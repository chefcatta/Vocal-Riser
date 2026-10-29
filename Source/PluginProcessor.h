#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

//==============================================================================
// Pitch shifter granulare a due testine
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        int size = 1;
        while (size < (int) (0.2 * sampleRate)) size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
        phase = 0.0f;
        window = (float) (0.05 * sampleRate); // finestra da 50 ms
    }

    int getLatency() const { return (int) (window * 0.5f) + 2; }

    float process (float x, float ratio)
    {
        buffer[(size_t) writePos] = x;

        phase += (1.0f - ratio) / window;
        phase -= std::floor (phase);

        float phase2 = phase + 0.5f;
        if (phase2 >= 1.0f) phase2 -= 1.0f;

        const float s  = std::sin (juce::MathConstants<float>::pi * phase);
        const float g1 = s * s;
        const float y  = read (2.0f + phase  * window) * g1
                       + read (2.0f + phase2 * window) * (1.0f - g1);

        writePos = (writePos + 1) & mask;
        return y;
    }

private:
    float read (float delay) const
    {
        const float rp = (float) writePos - delay + (float) (mask + 1);
        const int   i0 = (int) rp;
        const float fr = rp - (float) i0;
        const float a  = buffer[(size_t) (i0 & mask)];
        const float b  = buffer[(size_t) ((i0 + 1) & mask)];
        return a + fr * (b - a);
    }

    std::vector<float> buffer;
    int mask = 0, writePos = 0;
    float phase = 0.0f, window = 1.0f;
};

//==============================================================================
class VocalRiserAudioProcessor : public juce::AudioProcessor,
                                 private juce::AudioProcessorValueTreeState::Listener
{
public:
    enum CaptureState { stateIdle = 0, stateArmed, stateRecording };

    VocalRiserAudioProcessor();
    ~VocalRiserAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Vocal Riser"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    //== Cattura (chiamate dal message thread) ==================================
    void armCapture()               { captureState.store (stateArmed); }
    int  getCaptureState() const    { return captureState.load(); }
    bool hasCapture() const         { return captureCounter.load() > 0; }

    //== Render (solo message thread) ===========================================
    bool updateRender();                 // true se ha appena renderizzato
    bool hasRender() const               { return renderReady; }
    const std::vector<float>& getPeaks() const { return peaks; }
    juce::String getRenderInfo() const   { return renderInfo; }
    juce::File writeRiserFile();         // scrive il WAV (una volta per render)

    //== Anteprima =============================================================
    void startPreview()  { previewRestart.store (true); previewOn.store (true); }
    void stopPreview()   { previewOn.store (false); }
    std::atomic<bool>  previewOn { false }, previewLoop { false };
    std::atomic<float> previewProgress { 0.0f };

private:
    void parameterChanged (const juce::String&, float) override { paramsDirty.store (true); }
    void renderRiser();
    void finishCapture (int& state);

    std::atomic<float>* pAmount = nullptr; std::atomic<float>* pLength = nullptr;
    std::atomic<float>* pPitchOn = nullptr; std::atomic<float>* pFilterOn = nullptr;
    std::atomic<float>* pStutOn = nullptr;  std::atomic<float>* pPhoneOn = nullptr;
    std::atomic<float>* pChorusOn = nullptr; std::atomic<float>* pRevDryOn = nullptr;
    std::atomic<float>* pAutoArm = nullptr;

    double sr = 44100.0;

    // Cattura: doppio buffer (l'audio thread scrive in uno, il render legge l'altro)
    std::array<juce::AudioBuffer<float>, 2> captures;
    std::array<int, 2>    captureLens  { 0, 0 };
    std::array<int, 2>    captureChans { 1, 1 };
    std::array<double, 2> captureBpms  { 120.0, 120.0 };
    int writeIdx = 0, recLen = 0, silenceCount = 0, captureMax = 0;
    std::atomic<int> publishedIdx { -1 };
    std::atomic<int> captureState { stateIdle };
    std::atomic<int> captureCounter { 0 };
    bool wasPlaying = false;

    // Pre-roll per non tagliare l'attacco
    std::array<std::vector<float>, 2> preRing;
    int ringPos = 0, ringMask = 0, prerollLen = 0;

    // Render
    juce::AudioBuffer<float> rendered;
    std::vector<float> peaks;
    juce::String renderInfo;
    juce::File renderedFile;
    bool renderReady = false;
    int lastRenderedCounter = 0;
    std::atomic<bool> paramsDirty { false };

    // Anteprima
    juce::SpinLock previewLock;
    std::unique_ptr<juce::AudioBuffer<float>> previewBuf;
    int previewPos = 0;
    std::atomic<bool> previewRestart { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalRiserAudioProcessor)
};
