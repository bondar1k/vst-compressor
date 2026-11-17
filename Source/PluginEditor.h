#pragma once
#include "PluginProcessor.h"
#include <array>

// ---------------------------------------------------------------- внешний вид
struct Skin
{
    const char* name;
    const char* sub;
    juce::uint32 bg1, bg2, accent, meterBg, meterInk, text, knobHi, knobLo;
};

class PlateLnF : public juce::LookAndFeel_V4
{
public:
    juce::Colour accent { 0xff6fb1ea }, text { 0xffd9dde3 }, knobHi { 0xff4a4f58 }, knobLo { 0xff17181c };

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

// ---------------------------------------------------------------- редактор
class LineageEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LineageEditor (LineageProcessor&);
    ~LineageEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void onModelChanged (int m);
    void applyPreset (int m);
    void drawMeter (juce::Graphics&);

    LineageProcessor& proc;
    PlateLnF lnf;

    struct Knob
    {
        juce::Slider s;
        juce::Label  l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a;
    };
    std::array<Knob, 9> knobs;
    std::array<juce::TextButton, NumModels> modelBtns;
    juce::ToggleButton autoBtn { "AUTO MAKEUP" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoAtt;
    juce::ParameterAttachment modelAtt;

    int   currentModel = 0;
    float grDisp = 0.f;
    juce::Rectangle<float> meterRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineageEditor)
};
