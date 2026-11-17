#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace ids
{
    inline constexpr const char* model     = "model";
    inline constexpr const char* input     = "input";
    inline constexpr const char* thresh    = "thresh";
    inline constexpr const char* ratio     = "ratio";
    inline constexpr const char* attack    = "attack";
    inline constexpr const char* release   = "release";
    inline constexpr const char* output    = "output";
    inline constexpr const char* mix       = "mix";
    inline constexpr const char* hpf       = "hpf";
    inline constexpr const char* character = "character";
    inline constexpr const char* autogain  = "autogain";
}

enum Model { FET = 0, Opto, VCA, Mu, RMS, NumModels };

// Стартовые значения, которые подставляются при выборе модели
struct ModelPreset { float thresh, ratio, attack, release, character; };
inline constexpr ModelPreset kPresets[NumModels] = {
    { -18.f, 4.f,  0.2f,  120.f, 35.f },   // FET
    { -20.f, 3.f, 10.0f, 1200.f, 30.f },   // OPTO
    { -14.f, 4.f, 10.0f,  300.f, 15.f },   // VCA BUS
    { -22.f, 3.f,  0.5f,  600.f, 45.f },   // VARI-MU
    { -16.f, 3.f, 15.0f,  200.f, 10.f }    // RMS
};

class LineageProcessor : public juce::AudioProcessor
{
public:
    LineageProcessor();
    ~LineageProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Lineage"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grMeter { 0.f };   // текущее GR в дБ (>= 0) для стрелки

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double sampleRate = 44100.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inSm, outSm, mixSm, mkSm;

    float  hpX[2] { 0.f, 0.f }, hpY[2] { 0.f, 0.f };   // sidechain HPF
    double uPrev[2] { 0.0, 0.0 };                        // состояние ADAA-сатуратора
    float  rmsState = 0.f;
    float  gr = 0.f;          // сглаженное gain reduction, дБ
    float  memory = 0.f;      // "память" программы для оптики / vari-mu

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LineageProcessor)
};
