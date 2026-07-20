#pragma once

#include "PluginProcessor.h"
#include "ui/SoundVisionLookAndFeel.h"
#include "ui/SourceLegend.h"
#include "ui/SpatialView.h"
#include <JuceHeader.h>

class SoundVisionAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit SoundVisionAudioProcessorEditor (SoundVisionAudioProcessor&);
    ~SoundVisionAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void syncModeVisibility();
    void refreshTitle();

    SoundVisionAudioProcessor& processorRef;
    sv::SoundVisionLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::ComboBox modeBox;
    juce::ComboBox colourBox;
    juce::TextEditor nameEditor;
    juce::ToggleButton bandOnlyButton { "Range Filter" };

    juce::Slider lowSlider;
    juce::Slider highSlider;
    juce::Slider particleSlider;

    juce::Label modeLabel;
    juce::Label colourLabel;
    juce::Label nameLabel;
    juce::Label lowLabel;
    juce::Label highLabel;
    juce::Label particleLabel;
    juce::Label statusLabel;

    sv::SpatialView spatialView;
    sv::SourceLegend sourceLegend;

    using BoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<BoxAttachment> modeAttachment;
    std::unique_ptr<BoxAttachment> colourAttachment;
    std::unique_ptr<SliderAttachment> lowAttachment;
    std::unique_ptr<SliderAttachment> highAttachment;
    std::unique_ptr<SliderAttachment> particleAttachment;
    std::unique_ptr<ButtonAttachment> bandOnlyAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundVisionAudioProcessorEditor)
};
