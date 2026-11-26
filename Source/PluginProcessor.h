#pragma once
#include <JuceHeader.h>

class RadioStaticProcessor : public juce::AudioProcessor
{
public:
    RadioStaticProcessor();
    ~RadioStaticProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RadioStatic"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // --- Daten für die Anzeige (Audio-Thread schreibt, GUI liest) ---
    static constexpr int scopeSize = 1024;               // muss eine Zweierpotenz sein
    std::array<std::atomic<float>, scopeSize> scope;
    std::atomic<int>   scopeWritePos { 0 };
    std::atomic<float> needlePos { 0.5f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct OnePole
    {
        float a = 0.f, z = 0.f;
        float lp (float x) { z += a * (x - z); return z; }
        float hp (float x) { return x - lp (x); }
    };

    // Bandbegrenzung wie bei einem AM-Radio: 2x Hochpass + 2x Tiefpass
    struct Band
    {
        OnePole h1, h2, l1, l2;
        float process (float x) { return l2.lp (l1.lp (h2.hp (h1.hp (x)))); }
        void set (float fHP, float fLP, double sr)
        {
            auto coef = [sr] (float f) { return 1.f - std::exp (-juce::MathConstants<float>::twoPi * f / (float) sr); };
            h1.a = h2.a = coef (fHP);
            l1.a = l2.a = coef (fLP);
        }
    };

    // Heterodyn-Pfeifton: Sinus mit driftender Frequenz, der ein- und ausblendet
    struct Whistle
    {
        float phase = 0.f, freq = 1500.f, target = 1500.f, env = 0.f, envTarget = 0.f;
        int counter = 0;

        float next (float sr, bool kw, juce::Random& r)
        {
            if (--counter <= 0)
            {
                counter = (int) (sr * (kw ? 0.3f + 2.5f * r.nextFloat()
                                          : 1.0f + 5.0f * r.nextFloat()));
                target = kw ? 200.f + 3300.f * r.nextFloat()      // KW: wild gleitend
                            : 600.f + 2400.f * r.nextFloat();     // MW: eher stabiler Ton
                envTarget = r.nextFloat() < 0.6f ? 1.f : 0.f;
            }
            freq += (target - freq) * (kw ? 0.0004f : 0.00003f);
            env  += (envTarget - env) * 0.00005f;
            phase += freq / sr;
            if (phase >= 1.f) phase -= 1.f;
            return std::sin (juce::MathConstants<float>::twoPi * phase) * env;
        }
    };

    // --- Funk-Stimmen aus der eingebetteten Aufnahme (Resources/voice.wav) --------
    // Das Plugin schneidet zufällig 1-3 Sätze aus der Aufnahme und spielt sie als
    // "Durchsage" ab: bandbegrenzt, stark angezerrt, mit Bit-/Sample-Crunch,
    // offenem Träger (Rauschen) und Squelch-Klacken am Anfang und Ende.
    struct RadioVoice
    {
        const std::vector<float>* clip = nullptr;                          // Mono-Aufnahme
        const std::vector<std::pair<int, int>>* phrases = nullptr;         // Satz-Grenzen (Samples)
        double clipRate = 16000.0;

        int    idleCount = 0, holdCount = 0;
        bool   playing = false;
        double pos = 0.0, startPos = 0.0, endPos = 0.0, step = 1.0;
        float  txGate = 0.f, gateCoef = 0.001f, squelch = 0.f, squelchDecay = 0.99f, held = 0.f;
        Band   voiceBand;

        void prepare (float fs)
        {
            idleCount = (int) (fs * 2.f);                        // erste Durchsage nach ca. 2 s
            holdCount = 0; playing = false;
            txGate = 0.f; squelch = 0.f; held = 0.f;
            gateCoef = 1.f - std::exp (-1.f / (0.01f * fs));
            voiceBand = {};
            voiceBand.set (380.f, 2600.f, (double) fs);          // typische Sprechfunk-Bandbreite
        }

        // amt = Regler "Voices" (0..1): bestimmt Häufigkeit und Verzerrung
        float next (float fs, float amt, juce::Random& r)
        {
            const float noise = r.nextFloat() * 2.f - 1.f;
            const bool haveClip = clip != nullptr && clip->size() > 2 && phrases != nullptr && ! phrases->empty();

            // Pause -> neue Durchsage starten
            if (! playing && --idleCount <= 0)
            {
                if (amt > 0.01f && haveClip)
                {
                    const int n = (int) phrases->size();
                    const int first = r.nextInt (n);
                    int count = 1;
                    if (r.nextFloat() < 0.5f)  ++count;
                    if (r.nextFloat() < 0.25f) ++count;
                    const int last = juce::jmin (n - 1, first + count - 1);

                    startPos = pos = (double) (*phrases)[(size_t) first].first;
                    endPos   = (double) (*phrases)[(size_t) last].second;
                    step     = clipRate / (double) fs * (0.97 + 0.06 * r.nextFloat());   // leichte Tempo-/Tonhöhen-Streuung
                    playing  = true;
                    squelch  = 1.f;
                    squelchDecay = std::exp (-1.f / (0.012f * fs));                      // Klick am Anfang
                }
                else
                    idleCount = (int) fs;
            }

            // Aufnahme abspielen
            float v = 0.f;
            const bool transmitting = playing;
            if (playing)
            {
                const size_t i0 = (size_t) pos;
                const float frac = (float) (pos - (double) i0);
                const float a = (*clip)[i0];
                const float b = i0 + 1 < clip->size() ? (*clip)[i0 + 1] : a;
                v = a + (b - a) * frac;

                const double ramp = 0.005 * clipRate;                                    // 5 ms Ein-/Ausblendung
                v *= (float) juce::jlimit (0.0, 1.0, juce::jmin (pos - startPos, endPos - pos) / ramp);

                pos += step;
                if (pos >= endPos || pos >= (double) clip->size() - 1.0)
                {
                    playing = false;
                    squelch = 1.f;
                    squelchDecay = std::exp (-1.f / (0.09f * fs));                       // "kschhk" am Ende
                    idleCount = (int) (fs * (3.f + 14.f * r.nextFloat()) / (0.35f + 1.65f * amt));
                }
            }

            // --- Funk-Verzerrung ---
            v = voiceBand.process (v);                                                    // Telefon-/Funk-Bandbreite
            const float drive = 3.f + 9.f * amt;                                          // Übersteuerung
            v = std::tanh (drive * (v + 0.08f)) - std::tanh (drive * 0.08f);              // Offset = gerade Obertöne
            const int hold = 1 + (int) (fs / 48000.f * (1.f + 3.f * amt));               // Sample-Rate-Crunch
            if (++holdCount >= hold) { holdCount = 0; held = v; }
            v = held * 0.8f;

            txGate += ((transmitting ? 1.f : 0.f) - txGate) * gateCoef;
            const float carrier = noise * 0.06f * txGate;                                 // offener Träger rauscht
            const float sq = (r.nextFloat() * 2.f - 1.f) * squelch * 0.7f;
            squelch *= squelchDecay;

            return v + carrier + sq;
        }
    };

    RadioVoice chatter;

    // eingebettete Sprachaufnahme (wird beim Start einmal aus BinaryData dekodiert)
    void loadVoiceClip();
    std::vector<float> voiceClip;
    std::vector<std::pair<int, int>> voicePhrases;
    double voiceRate = 16000.0;

    double sr = 44100.0;

    Band sigBand[2], intBand[2];
    OnePole noiseLP[2];
    float crackEnv[2] { 0.f, 0.f };
    juce::Random rnd[2];
    juce::Random rndShared;

    Whistle whistle1, whistle2;
    float fadePhase[3] { 0.f, 0.3f, 0.7f };
    float burst = 0.f, burstTarget = 0.f;
    int burstCounter = 0;

    float scopePeak = 0.f;
    int scopeCount = 0, scopeIdx = 0;

    juce::SmoothedValue<float> mixSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioStaticProcessor)
};
