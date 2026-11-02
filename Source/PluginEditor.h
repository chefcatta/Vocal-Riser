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
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

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
// Menu delle opzioni dello stutter: piccolo riquadro con la punta verso STUTTER.
// E' un normale componente figlio dell'editor (niente finestre modali): si apre con la
// freccia accanto a STUTTER e si chiude con un clic fuori o di nuovo sulla freccia.
class StutterPopup : public juce::Component
{
public:
    explicit StutterPopup (VocalRiserAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setArrowX (float x) { arrowX = x; repaint(); }

    static constexpr int width = 168, height = 112, arrowH = 7;

private:
    ArrowSelector timing;
    juce::Slider depth;
    juce::TextButton pingPong { "PING PONG" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ppAtt;
    float arrowX = width * 0.5f;
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
    void mouseDown (const juce::MouseEvent&) override;   // clic fuori dal menu dello stutter = chiudi

    void showStutterMenu (bool show);
    bool isStutterMenuVisible() const { return stutPopup.isVisible(); }

private:
    void timerCallback() override;
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
    ArrowSelector lengthSel, reverbSel;
    juce::TextButton stutMenuBtn;                       // freccia che apre il menu dello stutter
    juce::TextButton stereoBtn { "STEREO" };
    std::array<juce::TextButton, 4> fxBtns;
    StutterPopup stutPopup;                              // dichiarato dopo i pulsanti: si costruisce dopo e si distrugge prima

    std::unique_ptr<SliderAtt> amountAtt;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;

    juce::Rectangle<int> displayArea, knobArea, intensityArea;
    juce::Path displayShape;                 // display con l'incavo per la manopola
    juce::Point<float> knobCentre;
    float notchRadius = 0.0f;
    bool lastPreviewPlaying = false;
    int lastSourceStereo = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalRiserAudioProcessorEditor)
};
