#pragma once

#include "PluginProcessor.h"

class RiserLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RiserLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
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

    juce::String status;

private:
    VocalRiserAudioProcessor& proc;
    bool dragStarted = false;
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
    void updateStatus();

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    VocalRiserAudioProcessor& proc;
    RiserLookAndFeel lnf;

    RiserWaveView wave;
    juce::TextButton captureBtn { "CATTURA" }, loopBtn { "LOOP" }, saveBtn { "SALVA" };

    juce::Slider amountKnob;
    std::array<juce::TextButton, 4> lengthBtns;
    std::array<juce::TextButton, 4> fxBtns;

    std::unique_ptr<SliderAtt> amountAtt;
    std::vector<std::unique_ptr<ButtonAtt>> buttonAtts;

    juce::Rectangle<int> knobArea;
    int lastToggleMask = -1;
    bool lastPreviewPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalRiserAudioProcessorEditor)
};
