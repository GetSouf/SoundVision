#include "ui/SoundVisionLookAndFeel.h"

namespace sv
{

SoundVisionLookAndFeel::SoundVisionLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0f141a));
    setColour (juce::Label::textColourId, juce::Colour (0xffe8eef5));
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe8eef5));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1b2430));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff334155));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8eef5));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1b2430));
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1b2430));
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff334155));
    setColour (juce::TextEditor::textColourId, juce::Colour (0xffe8eef5));
    setColour (juce::ToggleButton::textColourId, juce::Colour (0xffe8eef5));
    setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff4ecdc4));
}

void SoundVisionLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (0xff243041));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (juce::Colour (0xff4ecdc4));
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto pointerLength = radius * 0.72f;
    const auto tip = centre.getPointOnCircumference (pointerLength, angle);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawLine (centre.x, centre.y, tip.x, tip.y, 2.0f);
}

void SoundVisionLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                           int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                           juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) width - 16.0f;
    const float cy = (float) height * 0.5f;
    arrow.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.fillPath (arrow);
}

} // namespace sv
