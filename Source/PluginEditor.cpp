#include "PluginEditor.h"

//==============================================================================
RadioStaticEditor::RadioStaticEditor (RadioStaticProcessor& p)
    : AudioProcessorEditor (p),
      proc (p),
      dial (p),
      modeSwitch (p.apvts),
      mixKnob     (p.apvts, "mix",     "MIX",     160.f, true,  knobLnf),
      noiseKnob   (p.apvts, "noise",   "NOISE",    74.f, false, knobLnf),
      crackleKnob (p.apvts, "crackle", "CRACKLE",  74.f, false, knobLnf),
      whistleKnob (p.apvts, "whistle", "WHISTLE",  74.f, false, knobLnf),
      fadingKnob  (p.apvts, "fading",  "FADING",   74.f, false, knobLnf),
      voicesKnob  (p.apvts, "chatter", "VOICES",   74.f, false, knobLnf)
{
    makeNoiseTile();

    for (juce::Component* c : { (juce::Component*) &dial, (juce::Component*) &modeSwitch,
                                (juce::Component*) &mixKnob,
                                (juce::Component*) &noiseKnob, (juce::Component*) &crackleKnob,
                                (juce::Component*) &whistleKnob, (juce::Component*) &fadingKnob,
                                (juce::Component*) &voicesKnob })
        addAndMakeVisible (c);

    setResizable (true, true);
    setResizeLimits (600, 360, 1800, 1080);
    getConstrainer()->setFixedAspectRatio ((double) (designW / designH));
    setSize ((int) designW, (int) designH);
}

//==============================================================================
void RadioStaticEditor::resized()
{
    scale = (float) getWidth() / designW;

    auto r = [this] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt (x * scale), juce::roundToInt (y * scale),
                                     juce::roundToInt (w * scale), juce::roundToInt (h * scale));
    };

    dial.setBounds (r (30.f, 72.f, 840.f, 207.f));

    // Schalter mittig im linken Feld (Feld: y 290..508)
    modeSwitch.setBounds (r (30.f, 290.f + (218.f - 137.f) * 0.5f, 262.f, 137.f));

    const float knobY = 290.f + (218.f - (74.f - 12.f + 44.f)) * 0.5f;
    noiseKnob.setBounds   (r (308.f, knobY, 74.f, noiseKnob.designHeight()));
    crackleKnob.setBounds (r (384.f, knobY, 74.f, crackleKnob.designHeight()));
    whistleKnob.setBounds (r (460.f, knobY, 74.f, whistleKnob.designHeight()));
    fadingKnob.setBounds  (r (536.f, knobY, 74.f, fadingKnob.designHeight()));
    voicesKnob.setBounds  (r (612.f, knobY, 74.f, voicesKnob.designHeight()));

    mixKnob.setBounds (r (706.f, 296.f, 160.f, mixKnob.designHeight()));

    renderBackground();
}

void RadioStaticEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (background, 0, 0);
}

//==============================================================================
void RadioStaticEditor::makeNoiseTile()
{
    noiseTile = juce::Image (juce::Image::ARGB, 128, 128, true);
    juce::Random rng (4711);
    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x)
        {
            const auto a = (juce::uint8) rng.nextInt (24);
            noiseTile.setPixelAt (x, y, rng.nextBool() ? juce::Colour::fromRGBA (255, 255, 255, a)
                                                       : juce::Colour::fromRGBA (0, 0, 0, a));
        }
}

void RadioStaticEditor::renderBackground()
{
    background = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    drawBackground (g);
}

void RadioStaticEditor::drawBackground (juce::Graphics& g)
{
    const juce::Rectangle<float> full (0.f, 0.f, designW, designH);

    // Grundfläche mit Vignette
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2e2c), designW * 0.5f, 240.f,
                                             juce::Colour (0xff0e0e0e), 0.f, 0.f, true));
    g.fillRect (full);

    // Fleckiges Metall
    juce::Random rng (77);
    for (int i = 0; i < 50; ++i)
    {
        const float cx = rng.nextFloat() * designW, cy = rng.nextFloat() * designH;
        const float rad = 20.f + rng.nextFloat() * 60.f;
        const auto col = rng.nextBool() ? juce::Colours::white : juce::Colours::black;
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.035f), cx, cy, col.withAlpha (0.f), cx + rad, cy, true));
        g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);
    }

    // Körnung
    g.setTiledImageFill (noiseTile, 0, 0, 1.f);
    g.fillRect (full);

    // Kratzer
    for (int i = 0; i < 160; ++i)
    {
        const float x = rng.nextFloat() * designW, y = rng.nextFloat() * designH;
        const float len = 5.f + rng.nextFloat() * 30.f;
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        g.setColour (juce::Colours::white.withAlpha (0.02f + rng.nextFloat() * 0.05f));
        g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.5f);
    }

    // Rahmenkante
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRect (full, 3.f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (full.reduced (6.f), 10.f, 1.2f);

    // Schrauben in den Ecken
    ui::screw (g, 25.f, 27.f, 11.f, 0.6f);
    ui::screw (g, designW - 25.f, 27.f, 11.f, 2.2f);
    ui::screw (g, 25.f, designH - 27.f, 11.f, 1.3f);
    ui::screw (g, designW - 25.f, designH - 27.f, 11.f, 0.1f);

    // Titel
    const juce::Rectangle<int> tr ((int) (designW * 0.5f) - 150, 12, 300, 54);
    g.setFont (ui::font ("Georgia", 42.f, juce::Font::bold, 0.06f));
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawFittedText ("RADIOSTATIC", tr.translated (2, 2), juce::Justification::centred, 1, 0.5f);
    g.setColour (ui::parchment.withAlpha (0.2f));
    g.drawFittedText ("RADIOSTATIC", tr.translated (-1, -1), juce::Justification::centred, 1, 0.5f);
    g.setColour (ui::parchment);
    g.drawFittedText ("RADIOSTATIC", tr, juce::Justification::centred, 1, 0.5f);

    g.setColour (ui::parchment.withAlpha (0.55f));
    g.drawLine (60.f, 36.f, 280.f, 36.f, 1.f);
    g.drawLine (100.f, 43.f, 280.f, 43.f, 1.f);
    g.drawLine (designW - 280.f, 36.f, designW - 60.f, 36.f, 1.f);
    g.drawLine (designW - 280.f, 43.f, designW - 100.f, 43.f, 1.f);

    // Vertiefte Felder hinter den Reglern
    auto panel = [&] (float x, float y, float w, float h)
    {
        const juce::Rectangle<float> r (x, y, w, h);
        g.setColour (juce::Colours::black.withAlpha (0.24f));
        g.fillRoundedRectangle (r, 10.f);
        g.setColour (ui::parchment.withAlpha (0.16f));
        g.drawRoundedRectangle (r, 10.f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (1.5f), 9.f, 1.f);
    };
    panel (302.f, 290.f, 388.f, 218.f);
    panel (700.f, 290.f, 172.f, 218.f);

    // Signatur ganz klein
    g.setColour (ui::parchment.withAlpha (0.45f));
    g.setFont (ui::font ("Bahnschrift", 10.f, juce::Font::plain, 0.12f));
    g.drawText ("Designed by Krystian Shek",
                juce::Rectangle<float> (designW * 0.5f - 100.f, designH - 24.f, 200.f, 14.f),
                juce::Justification::centred, false);
}
