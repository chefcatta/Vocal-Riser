#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>
#include <algorithm>

//==============================================================================
class RiserRenderThread;

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

    const juce::String getName() const override { return "Chef Riser"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    // Logic smette di far lavorare un Audio Unit quando l'ingresso resta in silenzio piu a lungo
    // della coda dichiarata: con 0 s il LOOP (che suona a DAW ferma, con ingresso muto)
    // partiva una volta e poi si fermava. Per l'AU dichiariamo quindi una coda lunga.
    double getTailLengthSeconds() const override
    {
        return wrapperType == wrapperType_AudioUnit ? 3600.0 : 0.0;
    }

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
    // RESET: cancella l'audio catturato (per catturare una nuova sezione), i controlli restano come sono
    void clearCapture();
    int  getCaptureState() const    { return captureState.load(); }
    bool hasCapture() const         { return publishedIdx.load() >= 0; }
    int  getSourceStereo() const    { return sourceStereo.load(); } // -1 = nessuna cattura, 0 = mono, 1 = stereo

    //== Render ==================================================================
    // Il riser viene calcolato su un thread a parte: l'interfaccia non si blocca mai.
    void requestRenderIfNeeded();        // da chiamare dal timer dell'interfaccia
    int  getRenderGeneration() const     { return renderGeneration.load(); }
    bool hasRender() const               { return renderReady.load(); }
    std::shared_ptr<const std::vector<float>> getPeaks() const;
    juce::String getRenderInfo() const;
    juce::File writeRiserFile();         // scrive il WAV (una volta per render)

    //== Anteprima in loop (suona solo a DAW ferma) ============================
    // Con il LOOP acceso l'anteprima esce da un'uscita audio aperta dal plugin stesso
    // (l'uscita del Mac/PC), cosi funziona anche se la DAW, a trasporto fermo, non chiama
    // il plugin. Se l'uscita non si puo aprire si usa il vecchio metodo (audio della DAW).
    void setPreviewLoop (bool shouldLoop);          // solo dal message thread
    bool getPreviewLoop() const                     { return previewLoop.load(); }
    bool isUsingPreviewDevice() const               { return usingPreviewDevice.load(); }
    std::atomic<float> previewProgress { 0.0f };

    bool isHostProcessing() const
    {
        return juce::Time::getMillisecondCounter() - lastProcessMs.load() < 300;
    }
    bool isPreviewPlaying() const
    {
        if (usingPreviewDevice.load()) return devicePlaying.load();
        // vale solo se l'host sta davvero chiamando processBlock
        return previewPlaying.load() && isHostProcessing();
    }

    // Riempie un blocco dell'uscita audio dell'anteprima (chiamata dal thread audio del dispositivo; pubblica per i test)
    void renderPreviewToDevice (float* const* out, int numOut, int numSamples, double deviceSampleRate);

private:
    friend class RiserRenderThread;
    void parameterChanged (const juce::String&, float) override { paramsDirty.store (true); }
    void renderRiser();
    void finishCapture (int& state);
    void applyRestoredCapture();

    struct RestoredCapture { juce::AudioBuffer<float> audio; double sr = 0.0, bpm = 120.0; };
    std::unique_ptr<RestoredCapture> restored;      // cattura letta dalla sessione, in attesa di prepareToPlay

    std::atomic<bool>     previewPlaying { false };
    std::atomic<juce::uint32> lastProcessMs { 0 };
    std::atomic<bool>     previewLoop { false };
    std::atomic<bool>     hostPlaying { false };

    // uscita audio propria per l'anteprima
    void openPreviewDevice();
    void closePreviewDevice();
    struct PreviewDeviceCallback;
    std::unique_ptr<juce::AudioDeviceManager> previewDevice;
    std::unique_ptr<juce::AudioIODeviceCallback> previewCallback;
    std::atomic<bool>   usingPreviewDevice { false }, devicePlaying { false };
    std::atomic<double> previewSr { 44100.0 };
    std::atomic<int>    previewGap { 0 };
    double devicePos = 0.0;      // solo thread del dispositivo
    int    deviceLastLen = 0;    // solo thread del dispositivo

    std::atomic<bool> clearRequested { false };
    std::atomic<int>  captureEpoch { 0 };          // cambia a ogni RESET: i render vecchi non vengono pubblicati

    std::atomic<float>* pAmount = nullptr;  std::atomic<float>* pLength = nullptr;
    std::atomic<float>* pFilterOn = nullptr; std::atomic<float>* pStutOn = nullptr;
    std::atomic<float>* pPhoneOn = nullptr;  std::atomic<float>* pChorusOn = nullptr;
    std::atomic<float>* pRevType = nullptr;  std::atomic<float>* pStereoOn = nullptr;
    std::atomic<float>* pStutRate = nullptr; std::atomic<float>* pStutDepth = nullptr;
    std::atomic<float>* pPingPong = nullptr;

    double sr = 0.0;

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

    // Render (thread separato)
    juce::CriticalSection captureLock;     // protegge le catture da prepareToPlay mentre il render le legge
    juce::CriticalSection resultLock;      // protegge i risultati del render
    std::shared_ptr<const juce::AudioBuffer<float>> rendered;
    std::shared_ptr<const std::vector<float>> peaks;
    double renderedSr = 44100.0;
    juce::String renderInfo, renderedName;
    juce::File renderedFile;
    std::atomic<bool> renderReady { false };
    std::atomic<int>  renderGeneration { 0 };
    int lastRequestedCounter = 0;
    std::atomic<bool> paramsDirty { false };
    std::atomic<int>  sourceStereo { -1 };
    std::unique_ptr<juce::Thread> renderThread;

    // Anteprima
    juce::SpinLock previewLock;
    std::unique_ptr<juce::AudioBuffer<float>> previewBuf;
    std::atomic<int> previewPos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalRiserAudioProcessor)
};
