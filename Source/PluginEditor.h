#pragma once

#include "PluginProcessor.h"

class RiserLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RiserLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;

private:
    juce::Image knobCache;           // corpo della manopola gia disegnato
    int   knobCacheSide = 0;
    float knobCacheScale = 0.0f;
};

//==============================================================================
// Forma d'onda del riser: da qui lo trascini nella sessione
class RiserWaveView : public juce::Component
{
public:
    explicit RiserWaveView (VocalRiserAudioProcessor& p) : proc (p)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragStarted = false; }

private:
    void rebuildLayers (float scale);

    VocalRiserAudioProcessor& proc;
    bool dragStarted = false;

    // livelli gia disegnati della forma d'onda (si rifanno solo quando cambia il riser)
    juce::Image normalImg, dimImg, litImg, hotImg;
    std::shared_ptr<const std::vector<float>> peaks;
    int   layerGen = -1;
    float layerScale = 0.0f;
    juce::Point<int> layerSize;
};

//==============================================================================
// Selettore minimale "‹ VALORE ›" legato a un parametro a scelta
class ArrowSelector : public juce::Component
{
public:
    ArrowSelector (juce::RangedAudioParameter& param, const juce::String& title, juce::StringArray values);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void step (int delta);

    juce::String title;
    juce::StringArray values;
    int index = 0;
    juce::ParameterAttachment attachment;
};

//==============================================================================
class VocalRiserAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit VocalRiserAudioProcessorEditor (VocalRiserAudioProcessor&);
    ~VocalRiserAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::String statusText() const;
    void paintStatic (juce::Graphics&);

    juce::Image bgCache;
    float bgScale = 0.0f;
    int lastGeneration = -1;


    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    VocalRiserAudioProcessor& proc;
    RiserLookAndFeel lnf;

    RiserWaveView wave;
    juce::TextButton captureBtn { "CATTURA" }, loopBtn { "LOOP" }, saveBtn { "SALVA" }, resetBtn { "RESET" };

    juce::Slider amountKnob;
    ArrowSelector lengthSel, reverbSel, timingSel;
    juce::TextButton stereoBtn { "STEREO" };
    std::array<juce::TextButton, 4> fxBtns;

    std::unique_ptr<SliderAtt> amountAtt;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;

    juce::Rectangle<int> displayArea, knobArea, intensityArea;
    juce::Path displayShape;                 // display con l'incavo per la manopola
    juce::Point<float> knobCentre;
    float notchRadius = 0.0f;
    juce::String lastStatus;
    bool lastPreviewPlaying = false;
    int lastSourceStereo = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalRiserAudioProcessorEditor)
};
