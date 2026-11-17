#include "PluginEditor.h"

using namespace juce;

// Каждой модели — своя "лицевая панель"
static const Skin skins[NumModels] = {
    { "FET",     "Fast solid-state limiter. Blackface, ultra-fast attack, aggressive and punchy.",
      0xff1b1d22, 0xff0e0f12, 0xff6fb1ea, 0xffe6dcb8, 0xff1d1b16, 0xffd9dde3, 0xff4a4f58, 0xff17181c },
    { "OPTO",    "Electro-optical leveler. Program-dependent release with a slow, glowing tail.",
      0xffa9aeb3, 0xff7d838a, 0xfff4efe0, 0xfff0d98a, 0xff1c1a14, 0xff15171a, 0xff3b3d40, 0xff0f1011 },
    { "VCA BUS", "Feed-forward console bus compressor. Clean, tight, glues a mix together.",
      0xff3b4047, 0xff262a2f, 0xfff0892a, 0xffd8d4c8, 0xff1a1a1a, 0xffe4e6e8, 0xff5a5f66, 0xff1c1e21 },
    { "VARI-MU", "Variable-mu tube limiter. Soft knee, ratio grows with compression, transformer warmth.",
      0xff2f3d38, 0xff1a2420, 0xffe0b25a, 0xffefe3bd, 0xff211c10, 0xffe9e2cf, 0xff5b5648, 0xff1d1b15 },
    { "RMS",     "True-RMS detector, linear-in-dB release, over-easy knee. Smooth and transparent.",
      0xff131313, 0xff070707, 0xffee4b3f, 0xff1e1f21, 0xffe8e3da, 0xffd6d6d6, 0xff55585d, 0xff17181a }
};

static Font mkFont (float h, bool bold = false)
{
    return Font (FontOptions (h, bold ? Font::bold : Font::plain));
}

// ================================================================= LookAndFeel
void PlateLnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                 float a0, float a1, Slider&)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto  c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    const float ringR = r - 2.f;
    Path track; track.addCentredArc (c.x, c.y, ringR, ringR, 0.f, a0, a1, true);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.strokePath (track, PathStrokeType (3.f, PathStrokeType::curved, PathStrokeType::rounded));

    Path val; val.addCentredArc (c.x, c.y, ringR, ringR, 0.f, a0, ang, true);
    g.setColour (accent);
    g.strokePath (val, PathStrokeType (3.f, PathStrokeType::curved, PathStrokeType::rounded));

    const float kr = r - 9.f;
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 4.f, kr * 2.f, kr * 2.f);

    g.setGradientFill (ColourGradient (knobHi, c.x - kr * 0.5f, c.y - kr * 0.8f,
                                       knobLo, c.x + kr * 0.5f, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
    g.setColour (Colours::white.withAlpha (0.14f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f, 1.2f);

    // насечки по ободку
    for (int i = 0; i < 24; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / 24.f;
        g.setColour (Colours::black.withAlpha (0.18f));
        g.drawLine (c.x + std::sin (a) * (kr - 4.f), c.y - std::cos (a) * (kr - 4.f),
                    c.x + std::sin (a) * kr,         c.y - std::cos (a) * kr, 1.f);
    }

    Path p;
    p.addRoundedRectangle (-1.8f, -kr + 4.f, 3.6f, kr * 0.45f, 1.5f);
    p.applyTransform (AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (accent);
    g.fillPath (p);
}

void PlateLnF::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent : Colours::black.withAlpha (hl ? 0.45f : 0.32f));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (on ? accent.brighter (0.3f) : Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (r, 6.f, 1.f);
}

void PlateLnF::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setFont (mkFont (12.5f, true));
    g.setColour (b.getToggleState() ? Colours::black.withAlpha (0.85f) : Colour (0xffe6e6e6));
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

void PlateLnF::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const auto led = Rectangle<float> (12.f, 12.f).withCentre ({ r.getX() + 16.f, r.getCentreY() });
    if (b.getToggleState())
    {
        g.setColour (accent.withAlpha (0.35f)); g.fillEllipse (led.expanded (4.f));
        g.setColour (accent);                   g.fillEllipse (led);
    }
    else
    {
        g.setColour (Colours::black.withAlpha (0.55f)); g.fillEllipse (led);
    }
    g.setColour (Colours::white.withAlpha (0.25f)); g.drawEllipse (led, 1.f);
    g.setColour (text);
    g.setFont (mkFont (11.5f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (32.f), Justification::centredLeft);
}

// ================================================================= Editor
LineageEditor::LineageEditor (LineageProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      modelAtt (*p.apvts.getParameter (ids::model), [this] (float v) { onModelChanged (roundToInt (v)); })
{
    setLookAndFeel (&lnf);
    setSize (940, 540);

    static const char* paramIds[9] = { ids::input, ids::thresh, ids::ratio, ids::attack, ids::release,
                                       ids::output, ids::mix, ids::hpf, ids::character };
    static const char* names[9]    = { "INPUT", "THRESHOLD", "RATIO", "ATTACK", "RELEASE",
                                       "OUTPUT", "MIX", "SC HPF", "CHARACTER" };

    for (int i = 0; i < 9; ++i)
    {
        auto& k = knobs[(size_t) i];
        k.s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k.s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        k.s.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
        k.s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        k.s.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        addAndMakeVisible (k.s);

        k.l.setText (names[i], dontSendNotification);
        k.l.setJustificationType (Justification::centred);
        k.l.setFont (mkFont (11.5f, true));
        addAndMakeVisible (k.l);

        k.a = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramIds[i], k.s);
    }

    for (int i = 0; i < NumModels; ++i)
    {
        auto& b = modelBtns[(size_t) i];
        b.setButtonText (skins[i].name);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1);
        b.onClick = [this, i]
        {
            modelAtt.setValueAsCompleteGesture ((float) i);
            applyPreset (i);
        };
        addAndMakeVisible (b);
    }

    addAndMakeVisible (autoBtn);
    autoAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::autogain, autoBtn);

    modelAtt.sendInitialUpdate();
    startTimerHz (30);
}

LineageEditor::~LineageEditor()
{
    setLookAndFeel (nullptr);
}

void LineageEditor::onModelChanged (int m)
{
    currentModel = jlimit (0, (int) NumModels - 1, m);
    const auto& s = skins[currentModel];

    lnf.accent  = Colour (s.accent);
    lnf.text    = Colour (s.text);
    lnf.knobHi  = Colour (s.knobHi);
    lnf.knobLo  = Colour (s.knobLo);

    for (auto& k : knobs)
    {
        k.l.setColour (Label::textColourId, Colour (s.text).withAlpha (0.85f));
        k.s.setColour (Slider::textBoxTextColourId, Colour (s.text));
    }
    for (int i = 0; i < NumModels; ++i)
        modelBtns[(size_t) i].setToggleState (i == currentModel, dontSendNotification);

    repaint();
}

void LineageEditor::applyPreset (int m)
{
    const auto& pr = kPresets[m];
    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = proc.apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set (ids::thresh, pr.thresh);
    set (ids::ratio, pr.ratio);
    set (ids::attack, pr.attack);
    set (ids::release, pr.release);
    set (ids::character, pr.character);
}

void LineageEditor::resized()
{
    meterRect = { 30.f, 118.f, 310.f, 250.f };
    autoBtn.setBounds (370 + 4 * 108 + 4, 118 + 190 + 84, 100, 30);

    for (int i = 0; i < NumModels; ++i)
        modelBtns[(size_t) i].setBounds (378 + i * 108, 26, 100, 32);

    const int x0 = 370, y0 = 118, cw = 108, ch = 190;
    for (int k = 0; k < 9; ++k)
    {
        auto cell = Rectangle<int> (x0 + (k % 5) * cw, y0 + (k / 5) * ch, cw, ch);
        knobs[(size_t) k].l.setBounds (cell.removeFromTop (22));
        knobs[(size_t) k].s.setBounds (cell.reduced (4, 0));
    }
}

void LineageEditor::timerCallback()
{
    const float target = proc.grMeter.load();
    grDisp += (target - grDisp) * 0.4f;
    repaint (meterRect.toNearestInt());
}

// ---------------------------------------------------------------- рисование
static void panel (Graphics& g, Rectangle<float> r)
{
    g.setColour (Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (r, 12.f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (r, 12.f, 1.f);
}

static void screw (Graphics& g, float cx, float cy, uint32 ink)
{
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (cx - 5.5f, cy - 5.5f, 11.f, 11.f);
    g.setColour (Colour (ink).withAlpha (0.35f));
    g.fillEllipse (cx - 4.5f, cy - 4.5f, 9.f, 9.f);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawLine (cx - 3.f, cy + 2.f, cx + 3.f, cy - 2.f, 1.4f);
}

void LineageEditor::paint (Graphics& g)
{
    const auto& s = skins[currentModel];
    const auto  r = getLocalBounds().toFloat();

    g.setGradientFill (ColourGradient (Colour (s.bg1), 0.f, 0.f, Colour (s.bg2), 0.f, r.getBottom(), false));
    g.fillAll();

    // тонкая "шлифовка" металла
    g.setColour (Colours::white.withAlpha (0.018f));
    for (int y = 0; y < getHeight(); y += 3) g.drawHorizontalLine (y, 0.f, (float) getWidth());

    // шапка
    g.setColour (Colours::black.withAlpha (0.28f));
    g.fillRect (0, 0, getWidth(), 84);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (84, 0.f, (float) getWidth());

    g.setColour (Colour (s.text));
    g.setFont (mkFont (32.f, true));
    g.drawText ("LINEAGE", 32, 12, 280, 38, Justification::centredLeft);
    g.setFont (mkFont (10.5f, true));
    g.setColour (Colour (s.text).withAlpha (0.6f));
    g.drawText ("VINTAGE COMPRESSOR COLLECTION", 34, 50, 280, 16, Justification::centredLeft);

    // панели
    panel (g, { 20.f, 104.f, 330.f, 414.f });
    panel (g, { 358.f, 104.f, 562.f, 414.f });

    screw (g, 14.f, 14.f, s.text);  screw (g, r.getRight() - 14.f, 14.f, s.text);
    screw (g, 14.f, r.getBottom() - 14.f, s.text);
    screw (g, r.getRight() - 14.f, r.getBottom() - 14.f, s.text);

    drawMeter (g);

    // название модели в левой панели
    g.setColour (Colour (s.accent));
    g.setFont (mkFont (34.f, true));
    g.drawText (s.name, 30, 384, 310, 44, Justification::centred);
    g.setColour (Colour (s.text).withAlpha (0.75f));
    g.setFont (mkFont (12.f));
    g.drawFittedText (s.sub, 44, 430, 282, 62, Justification::centredTop, 4);
}

void LineageEditor::drawMeter (Graphics& g)
{
    const auto& s = skins[currentModel];
    auto r = meterRect;

    g.setColour (Colour (s.meterBg));
    g.fillRoundedRectangle (r, 10.f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.20f), r.getX(), r.getY(),
                                       Colours::transparentWhite, r.getX(), r.getCentreY(), false));
    g.fillRoundedRectangle (r, 10.f);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (r, 10.f, 2.f);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (r.toNearestInt());

    const float maxDb = 20.f;
    const float span  = MathConstants<float>::halfPi;                // ±45°
    const Point<float> pivot (r.getCentreX(), r.getBottom() - 36.f);
    const float rad = jmin (r.getHeight() - 70.f, (r.getWidth() * 0.5f - 24.f) / std::sin (span * 0.5f));
    const Colour ink (s.meterInk);
    const Colour red (0xffd0392b);

    auto ptAt = [&] (float db, float rr)
    {
        const float a = (db / maxDb - 0.5f) * span;
        return Point<float> (pivot.x + rr * std::sin (a), pivot.y - rr * std::cos (a));
    };

    g.setColour (ink.withAlpha (0.8f));
    g.setFont (mkFont (10.f, true));
    g.drawText ("GAIN REDUCTION  dB", r.withHeight (28.f).translated (0.f, 6.f), Justification::centred);

    Path arc;
    arc.addCentredArc (pivot.x, pivot.y, rad, rad, 0.f, -span * 0.5f, span * 0.5f, true);
    g.setColour (ink.withAlpha (0.7f));
    g.strokePath (arc, PathStrokeType (1.3f));

    for (int db = 0; db <= 20; ++db)
    {
        const bool major = (db % 4 == 0);
        const auto p1 = ptAt ((float) db, rad);
        const auto p2 = ptAt ((float) db, rad - (major ? 14.f : 8.f));
        g.setColour (db >= 16 ? red : ink);
        g.drawLine ({ p1, p2 }, major ? 2.f : 1.f);
        if (major)
        {
            g.setColour (db >= 16 ? red : ink);
            g.setFont (mkFont (11.f, true));
            g.drawText (String (db), Rectangle<float> (26.f, 14.f).withCentre (ptAt ((float) db, rad - 28.f)),
                        Justification::centred);
        }
    }

    // стрелка
    const float db = jlimit (0.f, maxDb + 1.f, grDisp);
    const auto tip = ptAt (db, rad - 4.f);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.drawLine (pivot.x + 2.f, pivot.y + 2.f, tip.x + 2.f, tip.y + 2.f, 2.6f);
    g.setColour (red);
    g.drawLine ({ pivot, tip }, 2.4f);
    g.setColour (ink);
    g.fillEllipse (Rectangle<float> (18.f, 18.f).withCentre (pivot));
    g.setColour (Colours::white.withAlpha (0.2f));
    g.fillEllipse (Rectangle<float> (7.f, 7.f).withCentre ({ pivot.x - 2.f, pivot.y - 2.f }));

    g.setColour (ink.withAlpha (0.85f));
    g.setFont (mkFont (12.f, true));
    g.drawText ("-" + String (grDisp, 1) + " dB", r.withTop (r.getBottom() - 22.f), Justification::centred);
}
