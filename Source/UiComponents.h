#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Alle Komponenten zeichnen in "logischen" Koordinaten und skalieren sich selbst,
// damit das Fenster frei vergrößert/verkleinert werden kann.

namespace ui
{
    inline const juce::Colour amber     { 0xffffa63d };
    inline const juce::Colour amberHi   { 0xffffc766 };
    inline const juce::Colour parchment { 0xffd9bd98 };

    inline juce::Font font (const juce::String& name, float height,
                            int style = juce::Font::plain, float kerning = 0.f)
    {
        juce::Font f (juce::FontOptions (name, height, style));
        f.setExtraKerningFactor (kerning);
        return f;
    }

    // Leuchtende Linie (mehrere übereinandergelegte Striche = Glow)
    inline void glowLine (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b,
                          juce::Colour c, float w)
    {
        const juce::Line<float> l (a, b);
        g.setColour (c.withAlpha (0.10f)); g.drawLine (l, w * 5.f);
        g.setColour (c.withAlpha (0.18f)); g.drawLine (l, w * 3.f);
        g.setColour (c.withAlpha (0.35f)); g.drawLine (l, w * 1.8f);
        g.setColour (c.brighter (0.25f));  g.drawLine (l, w);
    }

    inline void screw (juce::Graphics& g, float cx, float cy, float r, float angleRad)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (cx - r, cy - r + 1.2f, 2 * r, 2 * r);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7d7d7d), cx - r * 0.4f, cy - r * 0.5f,
                                                 juce::Colour (0xff1c1c1c), cx + r, cy + r, true));
        g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (cx - r, cy - r, 2 * r, 2 * r, 1.f);

        juce::Path slot;
        slot.addRoundedRectangle (-r * 0.78f, -r * 0.14f, r * 1.56f, r * 0.28f, r * 0.1f);
        slot.applyTransform (juce::AffineTransform::rotation (angleRad).translated (cx, cy));
        g.setColour (juce::Colour (0xff0c0c0c));
        g.fillPath (slot);
    }

    inline void drawLed (juce::Graphics& g, float cx, float cy, bool on)
    {
        const float r = 7.5f;
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.55f), cx, cy,
                                                     amber.withAlpha (0.f), cx + 22.f, cy, true));
            g.fillEllipse (cx - 22.f, cy - 22.f, 44.f, 44.f);
        }
        g.setColour (juce::Colour (0xff0a0a0a));
        g.fillEllipse (cx - r - 2.5f, cy - r - 2.5f, 2.f * (r + 2.5f), 2.f * (r + 2.5f));
        g.setColour (juce::Colour (0xff3a3a3a));
        g.drawEllipse (cx - r - 2.5f, cy - r - 2.5f, 2.f * (r + 2.5f), 2.f * (r + 2.5f), 1.f);

        g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xffffe9a8), cx - 2.f, cy - 2.f,
                                                      juce::Colour (0xffff9d1c), cx + r, cy + r, true)
                              : juce::ColourGradient (juce::Colour (0xff5a3416), cx - 2.f, cy - 2.f,
                                                      juce::Colour (0xff24120a), cx + r, cy + r, true));
        g.fillEllipse (cx - r, cy - r, 2.f * r, 2.f * r);
    }

    inline void drawPlate (juce::Graphics& g, float w, float h)
    {
        const juce::Rectangle<float> plate (0.5f, 0.5f, w - 1.f, h - 1.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222222), 0.f, 0.f,
                                                 juce::Colour (0xff131313), 0.f, h, false));
        g.fillRoundedRectangle (plate, 9.f);
        g.setColour (juce::Colour (0xff0a0a0a));
        g.drawRoundedRectangle (plate, 9.f, 2.f);
        g.setColour (parchment.withAlpha (0.22f));
        g.drawRoundedRectangle (plate.reduced (3.f), 7.f, 1.f);
    }

    //==========================================================================
    // Metall-Drehregler mit LED-Skala
    class KnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                               float a0, float a1, juce::Slider&) override
        {
            const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
            const auto c = b.getCentre();
            const float u = juce::jmin (b.getWidth(), b.getHeight()) / 200.f;   // Radius 100 = 100u
            const float ang = a0 + pos * (a1 - a0);
            constexpr float pi = juce::MathConstants<float>::pi;

            auto P = [&] (float a, float r) { return juce::Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };
            auto circle = [&] (float r) { return juce::Rectangle<float> (c.x - r, c.y - r, 2 * r, 2 * r); };

            // --- LED-Skala ---
            constexpr int n = 31;
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (n - 1);
                const float a = a0 + t * (a1 - a0);
                const bool lit = pos > 0.f && t <= pos + 0.001f;
                const auto p1 = P (a, 86.f * u), p2 = P (a, 99.f * u);

                if (lit)
                    glowLine (g, p1, p2, amber, 2.2f * u);
                else
                {
                    g.setColour (juce::Colour (0xff9b8a70).withAlpha (0.55f));
                    g.drawLine (juce::Line<float> (p1, p2), 1.6f * u);
                }
            }

            // --- Sockel ---
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillEllipse (circle (80.f * u).translated (0.f, 3.f * u));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff232323), c.x, c.y - 80.f * u,
                                                     juce::Colour (0xff060606), c.x, c.y + 80.f * u, false));
            g.fillEllipse (circle (80.f * u));
            g.setColour (juce::Colour (0xff3c3c3c));
            g.drawEllipse (circle (80.f * u), 1.4f * u);
            g.setColour (amber.withAlpha (0.10f));
            g.drawEllipse (circle (74.f * u), 5.f * u);

            // --- Knopf-Körper (gebürstetes Metall) ---
            const auto body = circle (64.f * u);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillEllipse (circle (65.f * u).translated (0.f, 4.f * u));

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9c9c9), c.x - 20.f * u, c.y - 26.f * u,
                                                     juce::Colour (0xff4b4b4b), c.x + 50.f * u, c.y + 60.f * u, true));
            g.fillEllipse (body);

            auto pie = [&] (float from, float to, juce::Colour col)
            {
                juce::Path p;
                p.addPieSegment (body, from * pi, to * pi, 0.f);
                g.setColour (col);
                g.fillPath (p);
            };
            pie (0.15f, 0.45f, juce::Colours::white.withAlpha (0.16f));
            pie (1.15f, 1.45f, juce::Colours::white.withAlpha (0.16f));
            pie (0.65f, 0.95f, juce::Colours::black.withAlpha (0.14f));
            pie (1.65f, 1.95f, juce::Colours::black.withAlpha (0.14f));

            for (float r = 6.f; r < 64.f; r += 2.6f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.04f));
                g.drawEllipse (circle (r * u), 0.6f * u);
            }

            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), c.x, c.y - 64.f * u,
                                                     juce::Colours::black.withAlpha (0.65f), c.x, c.y + 64.f * u, false));
            g.drawEllipse (circle (63.f * u), 2.f * u);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawEllipse (circle (56.f * u), 1.f * u);

            // --- Markierung ---
            glowLine (g, P (ang, 33.f * u), P (ang, 52.f * u), amber, 2.6f * u);
        }
    };

    //==========================================================================
    // Drehregler + Wertanzeige + Beschriftung in einem Baustein
    class LabeledKnob : public juce::Component
    {
    public:
        LabeledKnob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                     const juce::String& labelText, float designWidth, bool isBig,
                     juce::LookAndFeel& lnf)
            : name (labelText), dw (designWidth), big (isBig),
              attachment (apvts, paramID, slider)
        {
            addAndMakeVisible (slider);
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                        juce::MathConstants<float>::pi * 2.75f, true);
            slider.setMouseDragSensitivity (250);
            slider.setLookAndFeel (&lnf);
            slider.onValueChange = [this] { repaint(); };

            if (auto* prm = apvts.getParameter (paramID))
                slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        }

        ~LabeledKnob() override { slider.setLookAndFeel (nullptr); }

        float designHeight() const { return side() + (big ? 58.f : 44.f); }

        void resized() override
        {
            const float s = (float) getWidth() / dw;
            slider.setBounds (juce::roundToInt (6.f * s), 0,
                              juce::roundToInt (side() * s), juce::roundToInt (side() * s));
        }

        void paint (juce::Graphics& g) override
        {
            if (getWidth() <= 0) return;
            g.addTransform (juce::AffineTransform::scale ((float) getWidth() / dw));

            const float boxH = big ? 30.f : 18.f;
            const float boxW = big ? 110.f : juce::jmin (64.f, dw - 16.f);
            const juce::Rectangle<float> box ((dw - boxW) * 0.5f, side() + 4.f, boxW, boxH);

            g.setColour (juce::Colour (0xff0a0705));
            g.fillRoundedRectangle (box, 5.f);
            g.setColour (juce::Colour (0xff4a4440));
            g.drawRoundedRectangle (box, 5.f, 1.2f);

            const auto txt = juce::String (juce::roundToInt ((float) slider.getValue() * 100.f)) + "%";
            g.setFont (font ("Bahnschrift", big ? 24.f : 13.5f, juce::Font::plain, 0.04f));
            if (big)
            {
                g.setColour (amber.withAlpha (0.20f));
                for (auto off : { juce::Point<float> (-1.5f, 0.f), { 1.5f, 0.f }, { 0.f, -1.5f }, { 0.f, 1.5f } })
                    g.drawText (txt, box.translated (off.x, off.y), juce::Justification::centred, false);
            }
            g.setColour (amberHi);
            g.drawText (txt, box, juce::Justification::centred, false);

            g.setColour (juce::Colour (0xffc9b79a));
            g.setFont (font ("Bahnschrift", big ? 15.f : 12.5f, juce::Font::plain, 0.14f));
            g.drawText (name, juce::Rectangle<float> (0.f, box.getBottom() + 4.f, dw, big ? 18.f : 15.f),
                        juce::Justification::centred, false);
        }

    private:
        float side() const { return dw - 12.f; }

        juce::String name;
        float dw;
        bool big;
        juce::Slider slider;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    };

    //==========================================================================
    // Die beleuchtete Skala mit MW/SW, Nadel und Oszilloskop-Linie
    class DialDisplay : public juce::Component, private juce::Timer
    {
    public:
        explicit DialDisplay (RadioStaticProcessor& p) : proc (p)
        {
            setInterceptsMouseClicks (false, false);
            startTimerHz (30);
        }

        void paint (juce::Graphics& g) override
        {
            if (getWidth() <= 0 || getHeight() <= 0) return;
            const float s = (float) getWidth() / W;
            const bool kw = proc.apvts.getRawParameterValue ("mode")->load() > 0.5f;

            if (! staticLayer.isValid() || staticLayer.getWidth() != getWidth()
                || staticLayer.getHeight() != getHeight() || kw != staticKw)
            {
                staticKw = kw;
                staticLayer = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
                juce::Graphics ig (staticLayer);
                ig.addTransform (juce::AffineTransform::scale (s));
                drawStatic (ig, kw);
            }

            g.drawImageAt (staticLayer, 0, 0);
            g.addTransform (juce::AffineTransform::scale (s));
            drawScope (g);
            drawNeedle (g);
        }

        static constexpr float W = 717.f, H = 177.f;

    private:
        void timerCallback() override
        {
            needle += (proc.needlePos.load (std::memory_order_relaxed) - needle) * 0.12f;
            repaint();
        }

        void drawScale (juce::Graphics& g, const std::vector<float>& xs, const std::vector<juce::String>& labels,
                        int sub, float lineY, float numY, float xa, float xb, float alpha)
        {
            const auto col = juce::Colour (0xfff0b565).withAlpha (alpha);
            const int n = (int) xs.size();

            g.setColour (col.withMultipliedAlpha (0.55f));
            g.drawLine (xa, lineY, xb, lineY, 1.f);
            g.setColour (col);

            auto tick = [&] (float x, bool major)
            {
                g.drawLine (x, lineY, x, lineY - (major ? 12.f : 5.5f), major ? 1.7f : 1.f);
            };

            for (int i = 0; i < n; ++i) tick (xs[(size_t) i], true);
            for (int i = 0; i < n - 1; ++i)
                for (int k = 1; k < sub; ++k)
                    tick (xs[(size_t) i] + (xs[(size_t) i + 1] - xs[(size_t) i]) * (float) k / (float) sub, false);

            float d = (xs[1] - xs[0]) / (float) sub;
            for (float x = xs[0] - d; x >= xa; x -= d) tick (x, false);
            d = (xs[(size_t) n - 1] - xs[(size_t) n - 2]) / (float) sub;
            for (float x = xs[(size_t) n - 1] + d; x <= xb; x += d) tick (x, false);

            g.setFont (font ("Bahnschrift", 16.5f, juce::Font::plain, 0.02f));
            for (int i = 0; i < n; ++i)
                g.drawText (labels[(size_t) i], juce::Rectangle<float> (xs[(size_t) i] - 26.f, numY - 10.f, 52.f, 20.f),
                            juce::Justification::centred, false);
        }

        void drawStatic (juce::Graphics& g, bool kw)
        {
            const juce::Rectangle<float> full (0.f, 0.f, W, H);

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff343434), 0.f, 0.f,
                                                     juce::Colour (0xff0c0c0c), 0.f, H, false));
            g.fillRoundedRectangle (full, 16.f);
            g.setColour (juce::Colour (0xff5a5a5a).withAlpha (0.55f));
            g.drawRoundedRectangle (full.reduced (0.8f), 16.f, 1.4f);

            const auto glass = full.reduced (13.f, 8.f);
            juce::Path gp;
            gp.addRoundedRectangle (glass, 14.f);

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a1c06), 0.f, glass.getY(),
                                                     juce::Colour (0xff160b03), 0.f, glass.getBottom(), false));
            g.fillPath (gp);

            g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.22f), glass.getCentreX(), glass.getCentreY(),
                                                     amber.withAlpha (0.f), glass.getX(), glass.getCentreY(), true));
            g.fillPath (gp);

            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), 0.f, glass.getY(),
                                                     juce::Colours::white.withAlpha (0.f), 0.f, glass.getY() + 60.f, false));
            g.fillPath (gp);

            for (int i = 0; i < 5; ++i)
            {
                g.setColour (amber.withAlpha (0.10f - (float) i * 0.018f));
                g.drawRoundedRectangle (glass.reduced ((float) i * 1.5f), 14.f - (float) i, 3.f);
            }
            g.setColour (amber.withAlpha (0.85f));
            g.drawRoundedRectangle (glass.reduced (1.5f), 13.f, 1.6f);

            const float aMW = kw ? 0.38f : 1.f;
            const float aSW = kw ? 1.f : 0.38f;

            drawScale (g, { 95.5f, 159.5f, 226.f, 292.5f, 379.f, 454.f, 528.f, 595.5f },
                          { "520", "600", "700", "800", "1000", "1200", "1400", "1600" },
                       5, 56.f, 35.f, 74.f, 645.f, aMW);

            drawScale (g, { 88.5f, 157.5f, 226.f, 296.5f, 352.f, 408.f, 481.5f, 550.f, 623.5f },
                          { "3", "5", "7", "9", "11", "13", "16", "19", "22" },
                       4, 105.f, 84.f, 74.f, 645.f, aSW);

            g.setColour (amber.withAlpha (0.12f));
            g.drawLine (22.f, 68.f, W - 22.f, 68.f, 1.f);

            g.setFont (font ("Bahnschrift", 19.f, juce::Font::bold, 0.08f));
            auto label = [&] (const juce::String& t, float cx, float cy, float a)
            {
                g.setColour (juce::Colour (0xfff0b565).withAlpha (a));
                g.drawText (t, juce::Rectangle<float> (cx - 30.f, cy - 12.f, 60.f, 24.f), juce::Justification::centred, false);
            };
            label ("MW", 50.f, 45.f, aMW);   label ("kHz", 670.f, 45.f, aMW);
            label ("SW", 50.f, 93.f, aSW);   label ("MHz", 670.f, 93.f, aSW);
        }

        void drawScope (juce::Graphics& g)
        {
            constexpr int N = RadioStaticProcessor::scopeSize;
            const int wp = proc.scopeWritePos.load (std::memory_order_relaxed);
            const float x0 = 20.f, x1 = W - 20.f, cy = 144.f;

            juce::Path path;
            for (int i = 0; i < N; ++i)
            {
                const float v = proc.scope[(size_t) ((wp + i) & (N - 1))].load (std::memory_order_relaxed);
                const float x = x0 + (x1 - x0) * (float) i / (float) (N - 1);
                const float y = cy - std::tanh (v * 16.f) * 24.f;
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }

            g.setColour (amber.withAlpha (0.12f));
            g.strokePath (path, juce::PathStrokeType (5.f));
            g.setColour (amberHi.withAlpha (0.95f));
            g.strokePath (path, juce::PathStrokeType (1.1f));
        }

        void drawNeedle (juce::Graphics& g)
        {
            const float x = 74.f + (645.f - 74.f) * needle;
            glowLine (g, { x, 16.f }, { x, 162.f }, amberHi, 1.6f);
        }

        RadioStaticProcessor& proc;
        juce::Image staticLayer;
        bool staticKw = false;
        float needle = 0.5f;
    };

    //==========================================================================
    // Schiebeschalter MEDIUM WAVE / SHORTWAVE mit LEDs
    class ModeSwitch : public juce::Component, private juce::Timer
    {
    public:
        explicit ModeSwitch (juce::AudioProcessorValueTreeState& apvts)
            : attachment (*apvts.getParameter ("mode"),
                          [this] (float v)
                          {
                              currentIdx = v > 0.5f ? 1 : 0;
                              target = (float) currentIdx;
                              startTimerHz (60);
                              repaint();
                          })
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            attachment.sendInitialUpdate();
            pos = target;
        }

        void paint (juce::Graphics& g) override
        {
            if (getWidth() <= 0) return;
            g.addTransform (juce::AffineTransform::scale ((float) getWidth() / W));

            drawPlate (g, W, H);

            screw (g, 13.f, 12.f, 6.f, 0.5f);     screw (g, W - 13.f, 12.f, 6.f, 2.1f);
            screw (g, 13.f, H - 12.f, 6.f, 1.2f); screw (g, W - 13.f, H - 12.f, 6.f, 0.2f);

            g.setFont (font ("Bahnschrift", 16.f, juce::Font::plain, 0.12f));
            g.setColour (currentIdx == 0 ? juce::Colour (0xffe0c9a0) : juce::Colour (0xff9a8a70));
            g.drawText ("MEDIUM WAVE", juce::Rectangle<float> (89.f - 70.f, 11.f, 140.f, 24.f), juce::Justification::centred, false);
            g.setColour (currentIdx == 1 ? juce::Colour (0xffe0c9a0) : juce::Colour (0xff9a8a70));
            g.drawText ("SHORTWAVE", juce::Rectangle<float> (221.f - 70.f, 11.f, 140.f, 24.f), juce::Justification::centred, false);

            drawLed (g, 89.f, 49.f, currentIdx == 0);
            drawLed (g, 221.f, 49.f, currentIdx == 1);

            g.setColour (parchment.withAlpha (0.55f));
            g.drawLine (89.f, 64.f, 89.f, 72.f, 1.2f);
            g.drawLine (221.f, 64.f, 221.f, 72.f, 1.2f);
            g.setColour (parchment.withAlpha (0.25f));
            g.drawLine (89.f, 72.f, 221.f, 72.f, 1.f);
            g.drawLine (155.f, 69.f, 155.f, 72.f, 1.f);

            const juce::Rectangle<float> slot (57.f, 78.f, 191.f, 47.f);
            g.setColour (juce::Colour (0xff050505));
            g.fillRoundedRectangle (slot, 14.f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.9f), 0.f, slot.getY(),
                                                     juce::Colours::black.withAlpha (0.f), 0.f, slot.getY() + 14.f, false));
            g.fillRoundedRectangle (slot, 14.f);
            g.setColour (juce::Colour (0xff343434));
            g.drawRoundedRectangle (slot, 14.f, 1.4f);

            const float hw = 73.f, hh = 44.f;
            const float hx = juce::jmap (pos, slot.getX() + 3.f + hw * 0.5f, slot.getRight() - 3.f - hw * 0.5f) - hw * 0.5f;
            const juce::Rectangle<float> hr (hx, slot.getCentreY() - hh * 0.5f, hw, hh);

            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (hr.translated (0.f, 3.f), 6.f);

            juce::ColourGradient cg (juce::Colour (0xffd7d7d7), 0.f, hr.getY(),
                                     juce::Colour (0xff3d3d3d), 0.f, hr.getBottom(), false);
            cg.addColour (0.5, juce::Colour (0xff8a8a8a));
            g.setGradientFill (cg);
            g.fillRoundedRectangle (hr, 6.f);
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.drawRoundedRectangle (hr.reduced (0.7f), 6.f, 1.f);

            for (float gx = hr.getX() + hw * 0.45f; gx < hr.getRight() - 7.f; gx += 3.6f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.55f));
                g.drawLine (gx, hr.getY() + 6.f, gx, hr.getBottom() - 6.f, 1.2f);
                g.setColour (juce::Colours::white.withAlpha (0.25f));
                g.drawLine (gx + 1.3f, hr.getY() + 6.f, gx + 1.3f, hr.getBottom() - 6.f, 0.8f);
            }
        }

        void mouseDown (const juce::MouseEvent& e) override { choose (e); }
        void mouseDrag (const juce::MouseEvent& e) override { choose (e); }

    private:
        static constexpr float W = 300.f, H = 157.f;

        void choose (const juce::MouseEvent& e)
        {
            const float lx = e.position.x / ((float) getWidth() / W);
            const int idx = lx > 152.5f ? 1 : 0;
            if (idx != currentIdx)
            {
                currentIdx = idx;
                target = (float) idx;
                startTimerHz (60);
                attachment.setValueAsCompleteGesture ((float) idx);
                repaint();
            }
        }

        void timerCallback() override
        {
            pos += (target - pos) * 0.35f;
            if (std::abs (target - pos) < 0.002f) { pos = target; stopTimer(); }
            repaint();
        }

        int currentIdx = 0;
        float target = 0.f, pos = 0.f;
        juce::ParameterAttachment attachment;
    };
}
