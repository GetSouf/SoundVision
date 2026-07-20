#pragma once

#include <JuceHeader.h>

namespace sv
{

class SoundVisionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SoundVisionLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;
};

} // namespace sv
