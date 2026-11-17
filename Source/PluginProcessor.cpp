#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

// ---------------------------------------------------------------- параметры
AudioProcessorValueTreeState::ParameterLayout LineageProcessor::createLayout()
{
    using NR = NormalisableRange<float>;
    using Attr = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    const String muS = String::fromUTF8 ("\xc2\xb5s");

    auto dbText = [] (float v, int) { return String (v, 1) + " dB"; };
    auto dbParse = [] (const String& t) { return t.getFloatValue(); };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ids::model, 1 }, "Model",
                 StringArray { "FET", "OPTO", "VCA BUS", "VARI-MU", "RMS" }, 0));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::input, 1 }, "Input",
                 NR (-24.f, 24.f, 0.1f), 0.f,
                 Attr().withStringFromValueFunction (dbText).withValueFromStringFunction (dbParse)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::thresh, 1 }, "Threshold",
                 NR (-40.f, 0.f, 0.1f), -18.f,
                 Attr().withStringFromValueFunction (dbText).withValueFromStringFunction (dbParse)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::ratio, 1 }, "Ratio",
                 NR (1.f, 20.f, 0.1f, 0.5f), 4.f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + ":1"; })
                       .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::attack, 1 }, "Attack",
                 NR (0.02f, 100.f, 0.f, 0.35f), 0.2f,
                 Attr().withStringFromValueFunction ([muS] (float v, int)
                        { return v < 1.f ? String (roundToInt (v * 1000.f)) + " " + muS : String (v, 1) + " ms"; })
                       .withValueFromStringFunction ([] (const String& t)
                        { auto v = t.getFloatValue();
                          if (t.endsWithIgnoreCase ("ms")) return v;
                          if (t.endsWithIgnoreCase ("s"))  return v / 1000.f;
                          return v; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::release, 1 }, "Release",
                 NR (5.f, 2000.f, 0.f, 0.4f), 120.f,
                 Attr().withStringFromValueFunction ([] (float v, int)
                        { return v >= 1000.f ? String (v / 1000.f, 2) + " s" : String (roundToInt (v)) + " ms"; })
                       .withValueFromStringFunction ([] (const String& t)
                        { auto v = t.getFloatValue();
                          if (t.endsWithIgnoreCase ("ms")) return v;
                          if (t.endsWithIgnoreCase ("s"))  return v * 1000.f;
                          return v; })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::output, 1 }, "Output",
                 NR (-24.f, 24.f, 0.1f), 0.f,
                 Attr().withStringFromValueFunction (dbText).withValueFromStringFunction (dbParse)));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::mix, 1 }, "Mix",
                 NR (0.f, 100.f, 1.f), 100.f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; })
                       .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::hpf, 1 }, "SC HPF",
                 NR (20.f, 400.f, 1.f, 0.5f), 20.f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })
                       .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ids::character, 1 }, "Character",
                 NR (0.f, 100.f, 1.f), 35.f,
                 Attr().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; })
                       .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::autogain, 1 }, "Auto Makeup", true));

    return { p.begin(), p.end() };
}

// ---------------------------------------------------------------- процессор
LineageProcessor::LineageProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool LineageProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void LineageProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    inSm.reset (sr, 0.03);  outSm.reset (sr, 0.03);
    mixSm.reset (sr, 0.03); mkSm.reset (sr, 0.05);

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    inSm.setCurrentAndTargetValue  (Decibels::decibelsToGain (P (ids::input)));
    outSm.setCurrentAndTargetValue (Decibels::decibelsToGain (P (ids::output)));
    mixSm.setCurrentAndTargetValue (P (ids::mix) * 0.01f);
    mkSm.setCurrentAndTargetValue  (1.f);

    for (int c = 0; c < 2; ++c) { hpX[c] = hpY[c] = 0.f; uPrev[c] = 0.0; }
    rmsState = gr = memory = 0.f;
}

// log(cosh(x)) без переполнения — первообразная tanh для ADAA
static inline double logCosh (double x)
{
    x = std::abs (x);
    return x + std::log1p (std::exp (-2.0 * x)) - 0.6931471805599453;
}

void LineageProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numCh = jmin (buffer.getNumChannels(), 2);
    const int n     = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);
    if (numCh == 0 || n == 0) return;

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const int   model     = jlimit (0, (int) NumModels - 1, (int) P (ids::model));
    const float thr       = P (ids::thresh);
    const float ratio     = jmax (1.f, P (ids::ratio));
    const float attackMs  = P (ids::attack);
    const float releaseMs = P (ids::release);
    const float hpfHz     = P (ids::hpf);
    const float character = P (ids::character) * 0.01f;
    const bool  autoMk    = P (ids::autogain) > 0.5f;

    inSm.setTargetValue  (Decibels::decibelsToGain (P (ids::input)));
    outSm.setTargetValue (Decibels::decibelsToGain (P (ids::output)));
    mixSm.setTargetValue (P (ids::mix) * 0.01f);
    const float makeupDb = autoMk ? -thr * (1.f - 1.f / ratio) * 0.4f : 0.f;
    mkSm.setTargetValue (Decibels::decibelsToGain (makeupDb));

    // ---- характеристики моделей
    //                              FET    OPTO   VCA   MU     RMS
    static constexpr float kKnee[]  { 2.f, 10.f,  3.f,  0.f,  12.f };   // дБ
    static constexpr float kDrive[] { 6.f,  3.f,  2.f,  5.f,  1.5f };   // макс. предусиление сатуратора
    static constexpr float kBias[]  { 0.25f, 0.35f, 0.f, 0.12f, 0.f };  // асимметрия -> чётные гармоники

    const float fs = (float) sampleRate;
    const float effAttack = (model == Opto) ? jmax (attackMs, 8.f) : attackMs;
    const float aCoef   = std::exp (-1.f / (fs * 0.001f * jmax (0.01f, effAttack)));
    const float rCoef   = std::exp (-1.f / (fs * 0.001f * releaseMs));
    const float dbRate  = 24000.f / releaseMs;                       // dbx: линейный спад, дБ/с
    const float hpfA    = 1.f / (1.f + MathConstants<float>::twoPi * hpfHz / fs);
    const float rmsCoef = 1.f - std::exp (-1.f / (fs * 0.010f));
    const float memCoef = 1.f - std::exp (-1.f / (fs * 1.5f));
    const bool  useRms  = (model == Opto || model == Mu || model == RMS);
    const float knee    = kKnee[model];

    const double drive = 1.0 + (double) character * kDrive[model];
    const double bias  = (double) kBias[model] * character;
    const double tb    = std::tanh (bias);
    const double norm  = 1.0 / (drive * (1.0 - tb * tb));

    auto shape = [&] (int c, float in) -> float
    {
        const double u  = drive * (double) in + bias;
        const double du = u - uPrev[c];
        double y;
        if (std::abs (du) < 1.0e-5) y = std::tanh (0.5 * (u + uPrev[c]));
        else                        y = (logCosh (u) - logCosh (uPrev[c])) / du;   // ADAA 1-го порядка
        uPrev[c] = u;
        return (float) ((y - tb) * norm);
    };

    float* d[2] = { buffer.getWritePointer (0), buffer.getWritePointer (numCh > 1 ? 1 : 0) };

    for (int i = 0; i < n; ++i)
    {
        const float gIn = inSm.getNextValue(), gOut = outSm.getNextValue();
        const float m   = mixSm.getNextValue(), mk = mkSm.getNextValue();

        float x[2] = { 0.f, 0.f };
        float det = 0.f;
        for (int c = 0; c < numCh; ++c)
        {
            x[c] = d[c][i] * gIn;
            const float hp = hpfA * (hpY[c] + x[c] - hpX[c]);
            hpX[c] = x[c]; hpY[c] = hp;
            det = jmax (det, std::abs (hp));       // stereo link
        }

        // ---- детектор
        float lev = det;
        if (useRms)
        {
            rmsState += rmsCoef * (det * det - rmsState);
            lev = std::sqrt (rmsState);
        }
        const float levDb = Decibels::gainToDecibels (lev, -100.f);

        // ---- статическая кривая (величина GR в дБ, >= 0)
        const float over = levDb - thr;
        float tgt = 0.f;
        if (model == Mu)
        {   // ratio растёт вместе со сжатием — характерно для variable-mu
            if (over > 0.f) tgt = (1.f - 1.f / ratio) * over * over / (over + 12.f);
        }
        else if (2.f * over > -knee && 2.f * over < knee)
        {
            const float t = over + knee * 0.5f;
            tgt = (1.f - 1.f / ratio) * t * t / (2.f * knee);
        }
        else if (over >= knee * 0.5f)
            tgt = (1.f - 1.f / ratio) * over;

        // ---- динамика (разная у каждой модели)
        switch (model)
        {
            case FET:
            case VCA:
                gr = tgt + (tgt > gr ? aCoef : rCoef) * (gr - tgt);
                break;

            case Opto:
            case Mu:
            {   // программно-зависимый релиз: чем дольше и глубже сжимали, тем медленнее отпускает
                memory += memCoef * (gr - memory);
                float relT;
                if (model == Opto)
                    relT = releaseMs * (0.12f + 0.88f * jlimit (0.f, 1.f, memory / 10.f))
                                     * (1.f + 1.5f / (1.f + 0.5f * gr));   // "хвост" у лампы
                else
                    relT = releaseMs * (0.3f + 0.7f * jlimit (0.f, 1.f, memory / 8.f));
                const float rc = std::exp (-1.f / (fs * 0.001f * jmax (1.f, relT)));
                gr = tgt + (tgt > gr ? aCoef : rc) * (gr - tgt);
                break;
            }

            case RMS:
                if (tgt > gr) gr = tgt + aCoef * (gr - tgt);
                else          gr = jmax (tgt, gr - dbRate / fs);          // линейный по дБ релиз
                break;
        }
        gr = jmax (0.f, gr);

        const float g = Decibels::decibelsToGain (-gr) * mk;

        for (int c = 0; c < numCh; ++c)
        {
            const float wet = shape (c, x[c] * g);
            d[c][i] = (x[c] * (1.f - m) + wet * m) * gOut;
        }
    }

    grMeter.store (gr);
}

// ---------------------------------------------------------------- состояние
void LineageProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void LineageProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* LineageProcessor::createEditor() { return new LineageEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LineageProcessor(); }
