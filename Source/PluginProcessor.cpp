#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring>

namespace
{
uint32_t makeInstanceId()
{
    static std::atomic<uint32_t> counter { 1 };
    const auto now = (uint32_t) juce::Time::getHighResolutionTicks();
    return (counter.fetch_add (1, std::memory_order_relaxed) << 16) ^ now;
}
} // namespace

SoundVisionAudioProcessor::SoundVisionAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "SoundVisionState", sv::PluginParameters::createParameterLayout()),
      instanceId (makeInstanceId())
{
}

SoundVisionAudioProcessor::~SoundVisionAudioProcessor()
{
    unregisterSender();
}

void SoundVisionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec {
        sampleRate,
        (juce::uint32) samplesPerBlock,
        (juce::uint32) juce::jmax (1, getTotalNumInputChannels())
    };

    bandFilter.prepare (spec);
    analyzer.prepare (sampleRate, samplesPerBlock);
    analysisBuffer.setSize (juce::jmax (2, getTotalNumInputChannels()), samplesPerBlock, false, false, true);

    if (getMode() == sv::PluginMode::sender)
        ensureSenderRegistration();
    else
        unregisterSender();
}

void SoundVisionAudioProcessor::releaseResources()
{
    bandFilter.reset();
    analyzer.reset();
}

bool SoundVisionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn = layouts.getMainInputChannelSet();

    if (mainOut != mainIn)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

sv::PluginMode SoundVisionAudioProcessor::getMode() const noexcept
{
    const auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (sv::ParamIDs::mode));
    if (param == nullptr)
        return sv::PluginMode::sender;

    return param->getIndex() == 1 ? sv::PluginMode::receiver : sv::PluginMode::sender;
}

float SoundVisionAudioProcessor::getCentreHz() const
{
    return apvts.getRawParameterValue (sv::ParamIDs::centreHz)->load();
}

float SoundVisionAudioProcessor::getBandwidthHz() const
{
    return apvts.getRawParameterValue (sv::ParamIDs::bandwidthHz)->load();
}

float SoundVisionAudioProcessor::getParticleRate() const
{
    return apvts.getRawParameterValue (sv::ParamIDs::particleRate)->load();
}

bool SoundVisionAudioProcessor::isBandOnly() const
{
    return apvts.getRawParameterValue (sv::ParamIDs::showOnlyBand)->load() > 0.5f;
}

juce::Colour SoundVisionAudioProcessor::getSourceColour() const
{
    const auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (sv::ParamIDs::sourceColour));
    const int index = param != nullptr ? param->getIndex() : 0;
    return sv::PluginParameters::colourFromIndex (index);
}

juce::String SoundVisionAudioProcessor::getSourceDisplayName() const
{
    const juce::SpinLock::ScopedLockType lock (nameLock);
    return sourceName;
}

void SoundVisionAudioProcessor::setSourceDisplayName (const juce::String& name)
{
    const juce::SpinLock::ScopedLockType lock (nameLock);
    sourceName = name.isNotEmpty() ? name.substring (0, sv::kMaxNameChars - 1) : "Source";
}

std::vector<sv::SourceSnapshot> SoundVisionAudioProcessor::getReceiverSnapshots() const
{
    return hub->collectActiveSources();
}

sv::SourceSnapshot SoundVisionAudioProcessor::makeLocalSnapshot() const
{
    const auto analysis = getLocalAnalysis();

    sv::SourceSnapshot snap;
    snap.sourceId = instanceId;
    snap.colourARGB = getSourceColour().getARGB();
    snap.leftEnergy = analysis.leftEnergy;
    snap.rightEnergy = analysis.rightEnergy;
    snap.midEnergy = analysis.midEnergy;
    snap.sideEnergy = analysis.sideEnergy;
    snap.energy = analysis.energy;
    snap.bandEnergy = isBandOnly() ? analysis.bandEnergy : analysis.energy;
    snap.spectralFocus = analysis.spectralFocus;
    snap.samplePosition = sampleCounter.load (std::memory_order_relaxed);
    snap.active = true;

    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        const auto* raw = sourceName.toRawUTF8();
        const size_t bytes = juce::jmin ((size_t) sv::kMaxNameChars - 1, std::strlen (raw));
        std::memcpy (snap.name, raw, bytes);
        snap.name[bytes] = '\0';
    }

    return snap;
}

void SoundVisionAudioProcessor::ensureSenderRegistration()
{
    if (hubSlot.load (std::memory_order_acquire) >= 0)
        return;

    if (auto slot = hub->registerSource (instanceId))
        hubSlot.store (*slot, std::memory_order_release);
}

void SoundVisionAudioProcessor::unregisterSender()
{
    const int slot = hubSlot.exchange (-1, std::memory_order_acq_rel);
    if (slot >= 0)
        hub->unregisterSource (slot);
}

void SoundVisionAudioProcessor::publishSenderSnapshot (const sv::AnalysisResult& result)
{
    juce::ignoreUnused (result);
    const int slot = hubSlot.load (std::memory_order_acquire);
    if (slot < 0)
        return;

    hub->publish (slot, makeLocalSnapshot());
}

void SoundVisionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const auto mode = getMode();

    if (mode == sv::PluginMode::sender)
        ensureSenderRegistration();
    else
        unregisterSender();

    analysisBuffer.makeCopyOf (buffer, true);

    bandFilter.setBand (getCentreHz(), getBandwidthHz());
    bandFilter.setEnabled (isBandOnly());

    if (isBandOnly())
        bandFilter.process (analysisBuffer);

    const auto result = analyzer.analyse (buffer, analysisBuffer, getCentreHz(), getBandwidthHz());
    lastAnalysis.store (result);
    sampleCounter.fetch_add ((uint64_t) buffer.getNumSamples(), std::memory_order_relaxed);

    if (mode == sv::PluginMode::sender)
        publishSenderSnapshot (result);
}

void SoundVisionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("sourceName", getSourceDisplayName(), nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SoundVisionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        apvts.replaceState (tree);
        setSourceDisplayName (tree.getProperty ("sourceName", "Source").toString());
    }
}

juce::AudioProcessorEditor* SoundVisionAudioProcessor::createEditor()
{
    return new SoundVisionAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoundVisionAudioProcessor();
}
