#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"     // enthält die eingebettete Datei Resources/voice.wav

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout RadioStaticProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = [] (float v, int) { return String (roundToInt (v * 100.f)) + " %"; };
    auto knob = [&] (const char* id, const char* name, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                   NormalisableRange<float> (0.f, 1.f, 0.001f), def,
                   AudioParameterFloatAttributes().withStringFromValueFunction (pct));
    };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                     StringArray { "Medium Wave", "Shortwave" }, 0));
    p.push_back (knob ("mix",     "Mix",     0.35f));
    p.push_back (knob ("noise",   "Noise",   0.50f));
    p.push_back (knob ("crackle", "Crackle", 0.50f));
    p.push_back (knob ("whistle", "Whistle", 0.30f));
    p.push_back (knob ("fading",  "Fading",  0.40f));
    p.push_back (knob ("chatter", "Voices",  0.25f));


    return { p.begin(), p.end() };
}

RadioStaticProcessor::RadioStaticProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& s : scope) s.store (0.f);
    rnd[0].setSeed (1234);
    rnd[1].setSeed (5678);

    loadVoiceClip();
}

//==============================================================================
// Eingebettete Aufnahme laden, normalisieren und in Sätze zerlegen
void RadioStaticProcessor::loadVoiceClip()
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (
        fm.createReaderFor (std::make_unique<juce::MemoryInputStream> (BinaryData::voice_wav,
                                                                      (size_t) BinaryData::voice_wavSize,
                                                                      false)));
    if (reader == nullptr || reader->lengthInSamples < 100)
        return;

    const int n = (int) reader->lengthInSamples;
    const int numCh = (int) reader->numChannels;
    juce::AudioBuffer<float> tmp (numCh, n);
    reader->read (&tmp, 0, n, 0, true, true);

    voiceRate = reader->sampleRate;
    voiceClip.assign ((size_t) n, 0.f);
    float peak = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int c = 0; c < numCh; ++c) s += tmp.getSample (c, i);
        s /= (float) numCh;
        voiceClip[(size_t) i] = s;
        peak = juce::jmax (peak, std::abs (s));
    }
    if (peak > 0.f)
        for (auto& s : voiceClip) s *= 0.9f / peak;                   // auf 0,9 normalisieren

    // Sätze finden: Stille (>= 0,2 s) trennt die Abschnitte
    const int win = juce::jmax (1, (int) (0.01 * voiceRate));         // 10-ms-Fenster
    const int nWin = n / win;
    const float threshold = 0.02f;
    const int minSilenceWin = 20;
    const int minPhraseSamples = (int) (0.4 * voiceRate);

    bool inSpeech = false;
    int startW = 0, lastLoud = 0, silRun = 0;
    auto addPhrase = [&] (int a, int b)
    {
        b = juce::jmin (b, n);
        if (b - a >= minPhraseSamples) voicePhrases.emplace_back (a, b);
    };

    for (int w = 0; w < nWin; ++w)
    {
        double e = 0.0;
        for (int i = 0; i < win; ++i) { const float s = voiceClip[(size_t) (w * win + i)]; e += (double) s * s; }
        const bool loud = std::sqrt (e / win) > threshold;

        if (loud)
        {
            if (! inSpeech) { inSpeech = true; startW = w; }
            lastLoud = w;
            silRun = 0;
        }
        else if (inSpeech && ++silRun >= minSilenceWin)
        {
            addPhrase (startW * win, (lastLoud + 6) * win);
            inSpeech = false;
        }
    }
    if (inSpeech) addPhrase (startW * win, n);
    if (voicePhrases.empty()) voicePhrases.emplace_back (0, n);

    chatter.clip = &voiceClip;
    chatter.phrases = &voicePhrases;
    chatter.clipRate = voiceRate;
}

juce::AudioProcessorEditor* RadioStaticProcessor::createEditor()
{
    return new RadioStaticEditor (*this);
}

bool RadioStaticProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void RadioStaticProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    mixSm.reset (sr, 0.03);
    mixSm.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load());

    for (int c = 0; c < 2; ++c)
    {
        sigBand[c] = {}; intBand[c] = {}; noiseLP[c] = {}; crackEnv[c] = 0.f;
    }
    chatter.prepare ((float) sr);
}

//==============================================================================
void RadioStaticProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    for (int c = numCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    const bool  kw       = apvts.getRawParameterValue ("mode")->load()    > 0.5f;
    const float noiseAmt = apvts.getRawParameterValue ("noise")->load();
    const float crackAmt = apvts.getRawParameterValue ("crackle")->load();
    const float whistAmt = apvts.getRawParameterValue ("whistle")->load();
    const float fadeAmt  = apvts.getRawParameterValue ("fading")->load();
    const float chatAmt  = apvts.getRawParameterValue ("chatter")->load();
    mixSm.setTargetValue (apvts.getRawParameterValue ("mix")->load());

    // Bandbreite je nach Modus
    const float fHP = kw ? 250.f : 150.f;
    const float fLP = kw ? 2800.f : 3800.f;
    for (int c = 0; c < 2; ++c)
    {
        sigBand[c].set (fHP, fLP, sr);
        intBand[c].set (fHP, fLP, sr);
    }

    const float fs    = (float) sr;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float fadeRate[3] = { kw ? 0.13f : 0.05f, kw ? 0.41f : 0.11f, kw ? 0.97f : 0.23f };
    const float crackRate   = kw ? 10.f : 30.f;                                    // Knackser/Sek. bei 100 %
    const float crackDecay  = std::exp (-1.f / ((kw ? 0.0006f : 0.0012f) * fs));   // Länge der Knackser

    float* ch[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- gemeinsame Modulatoren ----
        for (int k = 0; k < 3; ++k)
        {
            fadePhase[k] += fadeRate[k] / fs;
            if (fadePhase[k] >= 1.f) fadePhase[k] -= 1.f;
        }
        const float fadeLfo  = (std::sin (twoPi * fadePhase[0]) + 0.7f * std::sin (twoPi * fadePhase[1])
                                + 0.5f * std::sin (twoPi * fadePhase[2])) / 2.2f;
        const float fadeNorm = 0.5f + 0.5f * fadeLfo;
        const float fadeGain = 1.f - fadeAmt * 0.9f * (1.f - fadeNorm);

        const float noiseCut    = kw ? 800.f + 5000.f * fadeNorm : 4500.f;
        const float noiseA      = 1.f - std::exp (-twoPi * noiseCut / fs);
        const float noiseMakeup = 0.5f / std::sqrt (noiseA);

        if (--burstCounter <= 0)
        {
            burstCounter = (int) (fs * (0.5f + 3.5f * rndShared.nextFloat()));
            burstTarget  = rndShared.nextFloat() < 0.3f ? 1.f : 0.f;
        }
        burst += (burstTarget - burst) * 0.0003f;
        const float crackProb = crackAmt * crackRate * (1.f + 8.f * burst) / fs;

        const float whistle = whistle1.next (fs, kw, rndShared)
                            + 0.6f * whistle2.next (fs, kw, rndShared);

        // Funk-Stimmen aus der eingebetteten Aufnahme. Pegel hier einstellbar:
        // 0.15 = Grundlautstärke, 0.45 = zusätzlich bei Regler auf 100 %
        const float chatLevel = chatAmt > 0.01f ? 0.15f + 0.45f * chatAmt : 0.f;
        const float chat = chatter.next (fs, chatAmt, rndShared) * chatLevel * fadeGain;

        const float mix = mixSm.getNextValue();

        // ---- pro Kanal ----
        for (int c = 0; c < numCh; ++c)
        {
            const float dry = ch[c][i];

            // Rauschen
            noiseLP[c].a = noiseA;
            const float white = rnd[c].nextFloat() * 2.f - 1.f;
            const float noise = noiseLP[c].lp (white) * noiseMakeup * noiseAmt * noiseAmt * 0.3f;

            // Knackser
            if (rnd[c].nextFloat() < crackProb)
            {
                const float r = rnd[c].nextFloat();
                crackEnv[c] = 0.1f + 1.5f * r * r * r;
            }
            const float crack = (rnd[c].nextFloat() * 2.f - 1.f) * crackEnv[c] * 0.8f;
            crackEnv[c] *= crackDecay;

            // Störungen zusammen, bandbegrenzt wie ein Radio
            float interf = noise + crack + whistle * whistAmt * 0.12f + chat;
            interf = intBand[c].process (interf);

            // Nutzsignal: Radio-Klang (bandbegrenzt) + Fading
            const float signal = sigBand[c].process (dry) * fadeGain;

            // leichte AM-/Röhren-Sättigung
            const float wet = std::tanh (1.2f * (signal + interf)) / 1.2f;

            ch[c][i] = dry * (1.f - mix) + wet * mix;

            // Daten für die Anzeige (nur linker Kanal, alle 16 Samples ein Spitzenwert)
            if (c == 0)
            {
                const float v = interf * mix;
                if (std::abs (v) > std::abs (scopePeak)) scopePeak = v;
                if (++scopeCount >= 16)
                {
                    scope[(size_t) scopeIdx].store (scopePeak, std::memory_order_relaxed);
                    scopeIdx = (scopeIdx + 1) & (scopeSize - 1);
                    scopePeak = 0.f;
                    scopeCount = 0;
                }
            }
        }
    }

    scopeWritePos.store (scopeIdx, std::memory_order_relaxed);

    // Skalennadel folgt dem Pfeifton (wirkt wie ein Sendersuchlauf)
    const float lo = kw ? 200.f : 600.f;
    const float hi = kw ? 3500.f : 3000.f;
    needlePos.store (juce::jlimit (0.f, 1.f, (whistle1.freq - lo) / (hi - lo)), std::memory_order_relaxed);
}

//==============================================================================
void RadioStaticProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RadioStaticProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RadioStaticProcessor();
}
