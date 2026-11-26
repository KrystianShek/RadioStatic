#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UiComponents.h"

class RadioStaticEditor : public juce::AudioProcessorEditor
{
public:
    explicit RadioStaticEditor (RadioStaticProcessor&);
    ~RadioStaticEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void makeNoiseTile();
    void renderBackground();
    void drawBackground (juce::Graphics&);

    static constexpr float designW = 900.f, designH = 540.f;

    RadioStaticProcessor& proc;

    // Reihenfolge wichtig: LookAndFeel muss vor den Reglern existieren
    ui::KnobLookAndFeel knobLnf;
    ui::DialDisplay dial;
    ui::ModeSwitch modeSwitch;
    ui::LabeledKnob mixKnob, noiseKnob, crackleKnob, whistleKnob, fadingKnob, voicesKnob;

    juce::Image background, noiseTile;
    float scale = 1.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioStaticEditor)
};
