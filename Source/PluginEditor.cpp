#include "PluginEditor.h"

namespace
{
void styleSlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    label.setText (text, juce::dontSendNotification);
    label.attachToComponent (&slider, false);
    label.setJustificationType (juce::Justification::centred);
}
} // namespace

SoundVisionAudioProcessorEditor::SoundVisionAudioProcessorEditor (SoundVisionAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (920, 600);
    setResizable (true, true);
    setResizeLimits (780, 520, 1500, 960);

    titleLabel.setText ("SoundVision", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff4ecdc4));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Spatial bus visualiser", juce::dontSendNotification);
    subtitleLabel.setFont (juce::FontOptions (14.0f));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9aa7b5));
    addAndMakeVisible (subtitleLabel);

    modeLabel.setText ("Mode", juce::dontSendNotification);
    colourLabel.setText ("Colour", juce::dontSendNotification);
    nameLabel.setText ("Source Name", juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9aa7b5));

    addAndMakeVisible (modeLabel);
    addAndMakeVisible (colourLabel);
    addAndMakeVisible (nameLabel);
    addAndMakeVisible (statusLabel);

    modeBox.addItemList ({ "Sender", "Receiver" }, 1);
    colourBox.addItemList (sv::PluginParameters::colourChoices(), 1);
    addAndMakeVisible (modeBox);
    addAndMakeVisible (colourBox);

    nameEditor.setText (processorRef.getSourceDisplayName(), juce::dontSendNotification);
    nameEditor.setTextToShowWhenEmpty ("Vocals / Guitar / Drums...", juce::Colour (0xff6b7785));
    nameEditor.onTextChange = [this]
    {
        processorRef.setSourceDisplayName (nameEditor.getText());
    };
    addAndMakeVisible (nameEditor);

    styleSlider (centreSlider, centreLabel, "Centre Hz");
    styleSlider (bandwidthSlider, bandwidthLabel, "Bandwidth");
    styleSlider (particleSlider, particleLabel, "Particles");

    centreSlider.setNumDecimalPlacesToDisplay (0);
    bandwidthSlider.setNumDecimalPlacesToDisplay (0);
    particleSlider.setNumDecimalPlacesToDisplay (2);

    addAndMakeVisible (centreSlider);
    addAndMakeVisible (bandwidthSlider);
    addAndMakeVisible (particleSlider);
    addAndMakeVisible (bandOnlyButton);
    addAndMakeVisible (spatialView);
    addAndMakeVisible (sourceLegend);

    auto& apvts = processorRef.getAPVTS();
    modeAttachment = std::make_unique<BoxAttachment> (apvts, sv::ParamIDs::mode, modeBox);
    colourAttachment = std::make_unique<BoxAttachment> (apvts, sv::ParamIDs::sourceColour, colourBox);
    centreAttachment = std::make_unique<SliderAttachment> (apvts, sv::ParamIDs::centreHz, centreSlider);
    bandwidthAttachment = std::make_unique<SliderAttachment> (apvts, sv::ParamIDs::bandwidthHz, bandwidthSlider);
    particleAttachment = std::make_unique<SliderAttachment> (apvts, sv::ParamIDs::particleRate, particleSlider);
    bandOnlyAttachment = std::make_unique<ButtonAttachment> (apvts, sv::ParamIDs::showOnlyBand, bandOnlyButton);

    spatialView.setSnapshotProvider ([this]() -> std::vector<sv::SourceSnapshot>
    {
        if (processorRef.getMode() == sv::PluginMode::receiver)
            return processorRef.getReceiverSnapshots();

        return { processorRef.makeLocalSnapshot() };
    });

    modeBox.onChange = [this] { syncModeVisibility(); };
    syncModeVisibility();
    startTimerHz (20);
}

SoundVisionAudioProcessorEditor::~SoundVisionAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SoundVisionAudioProcessorEditor::syncModeVisibility()
{
    const bool sender = processorRef.getMode() == sv::PluginMode::sender;
    colourBox.setEnabled (sender);
    nameEditor.setEnabled (sender);
    refreshTitle();
}

void SoundVisionAudioProcessorEditor::refreshTitle()
{
    if (processorRef.getMode() == sv::PluginMode::receiver)
        subtitleLabel.setText ("Receiver - decode the stereo field around the head", juce::dontSendNotification);
    else
        subtitleLabel.setText ("Sender - publish this track's L/R/Mid imaging", juce::dontSendNotification);
}

void SoundVisionAudioProcessorEditor::timerCallback()
{
    spatialView.setEmissionScale (processorRef.getParticleRate());
    spatialView.setBandLabel (processorRef.getCentreHz(), processorRef.getBandwidthHz());
    sourceLegend.setSources (spatialView.getLatestSnapshots());

    const auto analysis = processorRef.getLocalAnalysis();
    statusLabel.setText (
        "L " + juce::String (analysis.leftEnergy, 2)
            + "  M " + juce::String (analysis.midEnergy, 2)
            + "  R " + juce::String (analysis.rightEnergy, 2)
            + "  S " + juce::String (analysis.sideEnergy, 2),
        juce::dontSendNotification);

    syncModeVisibility();
}

void SoundVisionAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0f141a));

    auto header = getLocalBounds().removeFromTop (78).toFloat().reduced (12.0f, 8.0f);
    juce::ColourGradient grad (juce::Colour (0xff13202b), header.getTopLeft(),
                               juce::Colour (0xff0f141a), header.getBottomRight(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (header, 12.0f);
}

void SoundVisionAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (62);
    titleLabel.setBounds (header.removeFromLeft (220).removeFromTop (34));
    subtitleLabel.setBounds (header.removeFromTop (28));

    area.removeFromTop (8);
    auto controls = area.removeFromLeft (280);
    auto legend = area.removeFromBottom (130);
    sourceLegend.setBounds (legend.reduced (8, 4));
    spatialView.setBounds (area.reduced (8, 0));

    auto row = [&controls] (int h) -> juce::Rectangle<int>
    {
        auto r = controls.removeFromTop (h);
        controls.removeFromTop (8);
        return r;
    };

    auto placeLabeled = [] (juce::Rectangle<int> r, juce::Label& label, juce::Component& comp)
    {
        label.setBounds (r.removeFromTop (18));
        comp.setBounds (r);
    };

    placeLabeled (row (54), modeLabel, modeBox);
    placeLabeled (row (54), colourLabel, colourBox);
    placeLabeled (row (54), nameLabel, nameEditor);

    auto knobs = row (130);
    const int knobW = knobs.getWidth() / 3;
    centreSlider.setBounds (knobs.removeFromLeft (knobW).reduced (4, 12));
    bandwidthSlider.setBounds (knobs.removeFromLeft (knobW).reduced (4, 12));
    particleSlider.setBounds (knobs.reduced (4, 12));

    bandOnlyButton.setBounds (row (28));
    statusLabel.setBounds (controls.removeFromBottom (48));
}
