#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RiserReverbs.h"

namespace
{
    const double      beatsTable[]  = { 1.0, 2.0, 4.0, 8.0 };            // 1/4, 1/2, 1, 2 battute (4/4)
    const char* const lengthNames[] = { "1-4bar", "1-2bar", "1bar", "2bar" };
    const char* const renderParams[] = { "amount", "length", "filterOn", "stutOn", "phoneOn", "chorusOn",
                                         "revType", "stereoOn", "stutRate" };
    constexpr float thresholdGain = 0.0056f; // ~ -45 dBFS

    // Trova dove finisce la prima parola: l'inviluppo scende sotto -12 dB
    // rispetto al picco della parola (o sotto la soglia) per almeno 40 ms.
    int findFirstWordLength (const juce::AudioBuffer<float>& cap, int capLen, int capCh, double sr)
    {
        const int hop      = juce::jmax (1, (int) (0.005 * sr));
        const int win      = hop * 2;
        const int minWord  = (int) (0.08 * sr);
        const int maxWord  = juce::jmin (capLen, (int) (1.2 * sr));
        const int dipHold  = (int) (0.04 * sr);
        const int release  = (int) (0.02 * sr);

        float peak = 0.0f;
        int dipStart = -1;

        for (int pos = 0; pos + win <= capLen; pos += hop)
        {
            double sum = 0.0;
            for (int ch = 0; ch < capCh; ++ch)
            {
                const float* d = cap.getReadPointer (ch, pos);
                for (int i = 0; i < win; ++i) sum += (double) d[i] * d[i];
            }
            const float env = (float) std::sqrt (sum / (double) (win * capCh));
            peak = juce::jmax (peak, env);

            const bool dip = pos > minWord && (env < peak * 0.25f || env < thresholdGain * 0.5f);
            if (dip)
            {
                if (dipStart < 0) dipStart = pos;
                if (pos - dipStart >= dipHold)
                    return juce::jmin (capLen, dipStart + release);
            }
            else
            {
                dipStart = -1;
            }

            if (pos >= maxWord) break;
        }
        return juce::jmax (1, maxWord); // nota lunga senza pause: tieni al massimo 1,2 s
    }

    // Time-stretch granulare (senza cambiare intonazione): allunga src fino a outLen campioni.
    // Le posizioni dei grani sono le stesse per tutti i canali, cosi l'immagine stereo resta stabile.
    void stretchGranular (const juce::AudioBuffer<float>& src, int srcLen, juce::AudioBuffer<float>& dst, int outLen, double sr)
    {
        dst.setSize (src.getNumChannels(), outLen, false, false, true);
        dst.clear();
        if (srcLen < 8 || outLen < 8) return;

        const int grain = juce::jlimit (64, juce::jmax (64, srcLen), (int) (0.07 * sr));
        const int hop   = juce::jmax (1, grain / 4);
        std::vector<float> win ((size_t) grain), norm ((size_t) outLen, 0.0f);
        for (int i = 0; i < grain; ++i)
            win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (grain - 1));

        juce::Random rng (12345);
        const double span = (double) juce::jmax (0, srcLen - grain);
        const double outSpan = (double) juce::jmax (1, outLen - grain);

        for (int o = 0; o < outLen; o += hop)
        {
            const double prog = juce::jmin (1.0, (double) o / outSpan);
            const int jitter  = (int) ((rng.nextFloat() - 0.5f) * 0.3f * (float) grain);
            const int in      = juce::jlimit (0, (int) span, (int) (prog * span) + jitter);
            const int len     = juce::jmin (grain, outLen - o, srcLen - in);

            for (int ch = 0; ch < src.getNumChannels(); ++ch)
            {
                const float* s = src.getReadPointer (ch, in);
                float* d = dst.getWritePointer (ch, o);
                for (int i = 0; i < len; ++i) d[i] += s[i] * win[(size_t) i];
            }
            for (int i = 0; i < len; ++i) norm[(size_t) (o + i)] += win[(size_t) i];
        }

        for (int ch = 0; ch < dst.getNumChannels(); ++ch)
        {
            float* d = dst.getWritePointer (ch);
            for (int i = 0; i < outLen; ++i) d[i] /= juce::jmax (0.5f, norm[(size_t) i]);
        }
    }
}

//==============================================================================
// Thread che calcola il riser: riceve le richieste e tiene sempre solo l'ultima
class RiserRenderThread : public juce::Thread
{
public:
    explicit RiserRenderThread (VocalRiserAudioProcessor& p) : juce::Thread ("VocalRiser render"), owner (p) {}

    void request() { pending.store (true); notify(); }

    void run() override
    {
        while (! threadShouldExit())
        {
            if (pending.exchange (false))
                owner.renderRiser();
            else
                wait (-1);
        }
    }

private:
    VocalRiserAudioProcessor& owner;
    std::atomic<bool> pending { false };
};

//==============================================================================
VocalRiserAudioProcessor::VocalRiserAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pAmount   = apvts.getRawParameterValue ("amount");
    pLength   = apvts.getRawParameterValue ("length");
    pFilterOn = apvts.getRawParameterValue ("filterOn");
    pStutOn   = apvts.getRawParameterValue ("stutOn");
    pPhoneOn  = apvts.getRawParameterValue ("phoneOn");
    pChorusOn = apvts.getRawParameterValue ("chorusOn");
    pRevType  = apvts.getRawParameterValue ("revType");
    pStereoOn = apvts.getRawParameterValue ("stereoOn");
    pStutRate = apvts.getRawParameterValue ("stutRate");

    for (auto* id : renderParams)
        apvts.addParameterListener (id, this);

    renderThread = std::make_unique<RiserRenderThread> (*this);
    renderThread->startThread (juce::Thread::Priority::low);
}

VocalRiserAudioProcessor::~VocalRiserAudioProcessor()
{
    renderThread->signalThreadShouldExit();
    renderThread->notify();
    renderThread->stopThread (4000);

    for (auto* id : renderParams)
        apvts.removeParameterListener (id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalRiserAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "amount", 1 }, "Intensita",
                                                        NormalisableRange<float> (0.0f, 1.0f), 0.6f,
                                                        AudioParameterFloatAttributes().withStringFromValueFunction (
                                                            [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "length", 1 }, "Durata",
                                                         StringArray { "1/4 BAR", "1/2 BAR", "1 BAR", "2 BAR" }, 1));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "filterOn", 1 }, "Filtro",  true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "stutOn", 1 },   "Stutter", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "phoneOn", 1 },  "Phone",   false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "chorusOn", 1 }, "Chorus",  false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "revType", 1 }, "Ambiente",
                                                         StringArray { "Hall", "Plate", "Space" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "stereoOn", 1 }, "Mono -> Stereo", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "stutRate", 1 }, "Timing Stutter",
                                                         StringArray { "1/4", "1/8", "1/16" }, 2));

    return { p.begin(), p.end() };
}

bool VocalRiserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void VocalRiserAudioProcessor::prepareToPlay (double sampleRate, int)
{
    {
        const juce::ScopedLock sl (captureLock);

        // Logic richiama prepareToPlay spesso (cambio buffer, bounce...): la cattura
        // si conserva, si riparte da zero solo se cambia la frequenza di campionamento.
        const bool srChanged = std::abs (sampleRate - sr) > 0.5 || captureMax == 0;
        if (srChanged)
        {
            sr = sampleRate;
            captureMax = (int) (8.5 * sr); // basta per 2 battute anche a 60 BPM
            for (auto& c : captures) { c.setSize (2, captureMax); c.clear(); }
            captureLens = { 0, 0 };
            writeIdx = 0;
            publishedIdx.store (-1);
            renderReady.store (false);
            sourceStereo.store (-1);
            {
                const juce::SpinLock::ScopedLockType lock (previewLock);
                previewBuf.reset();
                previewPos.store (0);
            }
        }

        int ringSize = 1;
        while (ringSize < (int) (0.05 * sr)) ringSize <<= 1;
        for (auto& r : preRing) r.assign ((size_t) ringSize, 0.0f);
        ringMask = ringSize - 1;
        ringPos = 0;
        prerollLen = (int) (0.008 * sr);
        recLen = 0;
        silenceCount = 0;
        wasPlaying = false;
        if (captureState.load() == stateRecording)
            captureState.store (stateArmed);

        applyRestoredCapture();
    }
    paramsDirty.store (true);
}

//==============================================================================
void VocalRiserAudioProcessor::finishCapture (int& state)
{
    if (recLen < (int) (0.15 * sr))
    {
        state = stateArmed; // troppo corto: resta in ascolto
        return;
    }

    captureLens[(size_t) writeIdx] = recLen;
    publishedIdx.store (writeIdx);
    writeIdx ^= 1;
    captureCounter.fetch_add (1);
    state = stateIdle;
}

void VocalRiserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    lastProcessMs.store (juce::Time::getMillisecondCounter());

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();
    for (int ch = numIn; ch < numOut; ++ch) buffer.clear (ch, 0, n);

    const int numCh = juce::jmin (2, numIn, buffer.getNumChannels());
    if (numCh == 0 || n == 0 || captureMax == 0) return;

    double bpm = 120.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
        }
    bpm = juce::jlimit (40.0, 300.0, bpm);

    int state = captureState.load();

    if (! playing && wasPlaying && state == stateRecording)
        finishCapture (state);

    wasPlaying = playing;

    // --- Cattura (l'audio passa inalterato) ---------------------------------
    auto& cap = captures[(size_t) writeIdx];
    const int silenceMax = (int) (0.8 * sr);

    for (int i = 0; i < n; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float x = buffer.getSample (ch, i);
            preRing[(size_t) ch][(size_t) ringPos] = x;
            peak = juce::jmax (peak, std::abs (x));
        }
        ringPos = (ringPos + 1) & ringMask;

        if (state == stateArmed && peak > thresholdGain)
        {
            recLen = 0;
            silenceCount = 0;
            for (int k = 0; k < prerollLen; ++k)
            {
                const int rp = (ringPos - prerollLen + k + ringMask + 1) & ringMask;
                for (int ch = 0; ch < numCh; ++ch)
                    cap.setSample (ch, recLen, preRing[(size_t) ch][(size_t) rp]);
                ++recLen;
            }
            captureChans[(size_t) writeIdx] = numCh;
            captureBpms[(size_t) writeIdx]  = bpm;
            state = stateRecording;
        }
        else if (state == stateRecording)
        {
            for (int ch = 0; ch < numCh; ++ch)
                cap.setSample (ch, recLen, buffer.getSample (ch, i));
            ++recLen;

            silenceCount = peak > thresholdGain ? 0 : silenceCount + 1;
            if (recLen >= captureMax || silenceCount > silenceMax)
                finishCapture (state);
        }
    }

    captureState.store (state);

    // --- Anteprima in loop: solo quando il trasporto della DAW e fermo -------
    bool playingPreview = false;
    if (previewLoop.load() && ! playing)
    {
        const juce::SpinLock::ScopedTryLockType lock (previewLock);
        if (lock.isLocked() && previewBuf != nullptr && previewBuf->getNumSamples() > 0)
        {
            const int len = previewBuf->getNumSamples();
            const int gap = (int) (60.0 / bpm * sr);          // un beat di pausa tra una ripetizione e l'altra
            const int cycle = len + gap;

            int pos = previewPos.load (std::memory_order_relaxed);
            for (int i = 0; i < n; ++i)
            {
                if (pos >= cycle) pos = 0;
                if (pos < len)
                    for (int ch = 0; ch < numOut; ++ch)
                        buffer.addSample (ch, i, previewBuf->getSample (juce::jmin (ch, 1), pos));
                ++pos;
            }
            previewPos.store (pos, std::memory_order_relaxed);
            playingPreview = true;
            previewProgress.store (juce::jmin (1.0f, (float) pos / (float) len));
        }
    }
    else
    {
        previewPos.store (0, std::memory_order_relaxed); // al prossimo stop riparte dall'inizio
    }
    previewPlaying.store (playingPreview);
}

//==============================================================================
void VocalRiserAudioProcessor::requestRenderIfNeeded()
{
    if (! hasCapture()) return;

    const int counter = captureCounter.load();
    const bool newCapture = counter != lastRequestedCounter;
    if (newCapture || paramsDirty.exchange (false))
    {
        lastRequestedCounter = counter;
        static_cast<RiserRenderThread*> (renderThread.get())->request();
    }
}

std::shared_ptr<const std::vector<float>> VocalRiserAudioProcessor::getPeaks() const
{
    const juce::ScopedLock sl (resultLock);
    return peaks;
}

juce::String VocalRiserAudioProcessor::getRenderInfo() const
{
    const juce::ScopedLock sl (resultLock);
    return renderInfo;
}

void VocalRiserAudioProcessor::renderRiser()
{
    using namespace juce;

    // Copia locale della cattura: da qui in poi il render non tocca piu i buffer dell'audio thread
    AudioBuffer<float> cap;
    int capLen = 0, capCh = 1;
    double bpm = 120.0, rate = 44100.0;
    {
        const ScopedLock sl (captureLock);
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            const int before = captureCounter.load();
            const int idx = publishedIdx.load();
            if (idx < 0) return;
            capLen = jmin (captureLens[(size_t) idx], captures[(size_t) idx].getNumSamples());
            capCh  = jlimit (1, 2, captureChans[(size_t) idx]);
            bpm    = captureBpms[(size_t) idx];
            rate   = sr;
            if (capLen <= 0) return;
            cap.setSize (2, capLen, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                cap.copyFrom (ch, 0, captures[(size_t) idx], jmin (ch, capCh - 1), 0, capLen);
            if (captureCounter.load() == before) break;   // nessuna nuova cattura durante la copia
        }
    }

    const float  amount     = pAmount->load();
    const int    lenIdx     = jlimit (0, 3, (int) pLength->load());
    const double secPerBeat = 60.0 / bpm;
    const int    Ls         = jmax (256, (int) std::round (beatsTable[lenIdx] * secPerBeat * rate));
    const int    wordLen    = findFirstWordLength (cap, capLen, capCh, rate);
    // Solo la prima parte della prima parola (l'attacco): 40% della parola, tra 80 e 250 ms
    const int    attackLen  = jlimit ((int) (0.08 * rate), (int) (0.25 * rate), (int) (wordLen * 0.4));
    int          seg        = jmax (1, jmin (wordLen, attackLen, Ls));

    // 1) Attacco della prima parola (saltando il silenzio iniziale, cosi lo stretch non lo allunga)
    int onset = 0;
    {
        const float pk = cap.getMagnitude (0, jmin (capLen, seg * 2));
        const float* d0 = cap.getReadPointer (0);
        while (onset < seg / 2 && std::abs (d0[onset]) < pk * 0.2f) ++onset;
        onset = jmax (0, onset - (int) (0.003 * rate));
    }
    seg = jmin (seg, capLen - onset);
    AudioBuffer<float> attack (2, seg);
    for (int ch = 0; ch < 2; ++ch)
        attack.copyFrom (ch, 0, cap, jmin (ch, capCh - 1), onset, seg);
    const int edge = jmin (seg / 3, (int) (0.02 * rate));
    for (int ch = 0; ch < 2; ++ch) attack.applyGainRamp (ch, seg - edge, edge, 1.0f, 0.0f);

    // Sorgente stereo o mono? (una traccia stereo con L = R conta come mono)
    bool srcStereo = false;
    if (capCh >= 2)
    {
        const float* l = cap.getReadPointer (0);
        const float* r = cap.getReadPointer (1);
        double sum = 0.0, diff = 0.0;
        for (int j = 0; j < capLen; ++j)
        {
            sum  += (double) (l[j] + r[j]) * (l[j] + r[j]);
            diff += (double) (l[j] - r[j]) * (l[j] - r[j]);
        }
        srcStereo = diff > sum * 0.001; // differenza oltre -30 dB
    }
    sourceStereo.store (srcStereo ? 1 : 0);

    const auto image = srcStereo                  ? RiserReverbs::keepStereo
                     : pStereoOn->load() > 0.5f   ? RiserReverbs::monoToStereo
                                                  : RiserReverbs::monoOut;
    const auto revType = (RiserReverbs::Type) jlimit (0, 2, (int) pRevType->load());

    // 2) Attacco rovesciato -> eco a tempo + riverbero 100% wet (nessun segnale dry)
    AudioBuffer<float> rev (2, seg);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* src = attack.getReadPointer (ch);
        float* dst = rev.getWritePointer (ch);
        for (int j = 0; j < seg; ++j) dst[j] = src[seg - 1 - j];
    }

    // Coda naturale: lunga quanto il riser ma al massimo ~1,6 s; il resto lo fa lo stretch
    const int tailLen = jmin (Ls, (int) ((0.8 + 0.8 * amount) * rate));
    const int search  = (int) (0.5 * rate);          // finestra in cui cercare il picco del riverbero
    const int total   = seg + search + tailLen;
    AudioBuffer<float> wet (2, total);
    wet.clear();
    for (int ch = 0; ch < 2; ++ch) wet.copyFrom (ch, 0, rev, ch, 0, seg);

    const int   dly = jmax (1, (int) (0.75 * secPerBeat * rate));
    const float fb  = 0.30f + 0.30f * amount;
    for (int ch = 0; ch < 2; ++ch)
    {
        float* w = wet.getWritePointer (ch);
        // ping-pong solo quando si allarga una voce mono
        const int offset = (ch == 1 && image == RiserReverbs::monoToStereo) ? dly + dly / 3 : dly;
        for (int j = offset; j < total; ++j) w[j] += fb * w[j - offset];
    }

    RiserReverbs::apply (wet, total, revType, image, amount, rate);

    // 3) Solo la coda, rovesciata. Parte dal punto in cui il riverbero e piu pieno
    //    (subito dopo la voce), cosi il riser arriva al massimo proprio sull'attacco.
    int tailStart = seg;
    {
        const int win = (int) (0.02 * rate);
        double best = -1.0;
        for (int pos = seg; pos + win <= seg + search; pos += win / 2)
        {
            double e = 0.0;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float* w = wet.getReadPointer (ch, pos);
                for (int i = 0; i < win; ++i) e += (double) w[i] * w[i];
            }
            if (e > best) { best = e; tailStart = pos; }
        }
    }
    AudioBuffer<float> tail (2, tailLen);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* w = wet.getReadPointer (ch);
        float* t = tail.getWritePointer (ch);
        for (int j = 0; j < tailLen; ++j) t[j] = w[tailStart + tailLen - 1 - j];
    }

    // 4) Stretch della coda fino alla durata del riser
    AudioBuffer<float> out;
    if (tailLen < Ls) stretchGranular (tail, tailLen, out, Ls, rate);
    else              out.makeCopyOf (tail);

    auto normalise = [] (AudioBuffer<float>& b, float targetPeak)
    {
        const float mag = b.getMagnitude (0, b.getNumSamples());
        if (mag > 1.0e-6f) b.applyGain (targetPeak / mag);
    };
    normalise (out, 0.5f);

    // 5) Filtro, phone, stutter, crescendo
    dsp::ProcessSpec spec { rate, 512, 2 };
    dsp::StateVariableTPTFilter<float> hp, phHp, phLp;
    hp.prepare (spec);   hp.setType (dsp::StateVariableTPTFilterType::highpass);  hp.setResonance (1.1f);
    phHp.prepare (spec); phHp.setType (dsp::StateVariableTPTFilterType::highpass); phHp.setCutoffFrequency (450.0f);
    phLp.prepare (spec); phLp.setType (dsp::StateVariableTPTFilterType::lowpass);  phLp.setCutoffFrequency (2800.0f);

    const bool  filterOn = pFilterOn->load() > 0.5f;
    const bool  phoneOn  = pPhoneOn->load()  > 0.5f;
    const bool  stutOn   = pStutOn->load()   > 0.5f;
    const float fMax     = 200.0f * std::pow (10.0f, amount);   // 200 Hz -> 2 kHz
    const float volRange = 4.0f + 14.0f * amount;
    const float gateCoeff = 1.0f - std::exp (-1.0f / (0.0015f * (float) rate));
    static constexpr double stutDiv[] = { 1.0, 0.5, 0.25 };   // 1/4, 1/8, 1/16
    const double stutPeriod = secPerBeat * rate * stutDiv[jlimit (0, 2, (int) pStutRate->load())];
    float gate = 1.0f;

    for (int j = 0; j < Ls; ++j)
    {
        const float p = (float) j / (float) (Ls - 1);

        if (filterOn && (j & 15) == 0)
            hp.setCutoffFrequency (20.0f * std::pow (fMax / 20.0f, p));

        // Stutter costante su tutto il riser, allineato alla fine (attacco della voce)
        float gateTarget = 1.0f;
        if (stutOn)
        {
            double ph = (double) (Ls - 1 - j) / stutPeriod;
            ph -= std::floor (ph);
            gateTarget = ph < 0.55 ? 1.0f : 0.0f;
        }
        gate += gateCoeff * (gateTarget - gate);

        const float vol = Decibels::decibelsToGain (-volRange * std::pow (1.0f - p, 1.2f));

        for (int ch = 0; ch < 2; ++ch)
        {
            float x = out.getSample (ch, j);
            if (filterOn) x = hp.processSample (ch, x);
            if (phoneOn)  x = std::tanh (2.5f * phLp.processSample (ch, phHp.processSample (ch, x)));
            out.setSample (ch, j, x * gate * vol);
        }
    }

    if (pChorusOn->load() > 0.5f)
    {
        dsp::Chorus<float> chorus;
        chorus.prepare (spec);
        chorus.setRate (0.8f);
        chorus.setDepth (0.35f);
        chorus.setCentreDelay (9.0f);
        chorus.setFeedback (0.1f);
        chorus.setMix (0.5f);
        for (int pos = 0; pos < Ls; pos += 512)
        {
            dsp::AudioBlock<float> blk (out);
            auto sub = blk.getSubBlock ((size_t) pos, (size_t) jmin (512, Ls - pos));
            chorus.process (dsp::ProcessContextReplacing<float> (sub));
        }
    }

    if (image == RiserReverbs::monoOut)
        out.copyFrom (1, 0, out, 0, 0, Ls);   // uscita mono: L e R identici

    // 5) Normalizza a -1 dBFS e micro-fade
    normalise (out, Decibels::decibelsToGain (-1.0f));
    const int fadeIn = jmin (Ls / 4, (int) (0.01 * rate));
    const int fadeOut = jmin (Ls / 4, (int) (0.002 * rate));
    for (int ch = 0; ch < 2; ++ch)
    {
        out.applyGainRamp (ch, 0, fadeIn, 0.0f, 1.0f);
        out.applyGainRamp (ch, Ls - fadeOut, fadeOut, 1.0f, 0.0f);
    }

    // 8) Forma d'onda per l'interfaccia
    constexpr int bins = 300;
    auto newPeaks = std::make_shared<std::vector<float>> (bins, 0.0f);
    for (int b = 0; b < bins; ++b)
    {
        const int s0 = (int) ((int64) b * Ls / bins);
        const int s1 = jmax (s0 + 1, (int) ((int64) (b + 1) * Ls / bins));
        float m = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            m = jmax (m, out.getMagnitude (ch, s0, s1 - s0));
        (*newPeaks)[(size_t) b] = m;
    }

    // 9) Pubblica il risultato
    {
        const ScopedLock sl (resultLock);
        rendered     = std::make_shared<const AudioBuffer<float>> (out);
        peaks        = newPeaks;
        renderedSr   = rate;
        renderedName = String (roundToInt (bpm)) + "bpm_" + lengthNames[lenIdx];
        renderInfo   = String (roundToInt (bpm)) + " BPM  ·  " + String (Ls / rate, 2) + " s  ·  "
                       + (srcStereo ? "voce stereo" : "voce mono");
        renderedFile = File();
    }
    renderReady.store (true);
    renderGeneration.fetch_add (1);

    // 10) Passa la copia all'anteprima senza farla ripartire da capo:
    //     se il loop sta suonando, continua dallo stesso punto (in proporzione)
    auto fresh = std::make_unique<AudioBuffer<float>> (out);
    {
        const SpinLock::ScopedLockType lock (previewLock);
        const int oldLen = previewBuf != nullptr ? previewBuf->getNumSamples() : 0;
        const int pos = previewPos.load (std::memory_order_relaxed);
        if (oldLen > 0 && pos < oldLen)
            previewPos.store ((int) ((int64) pos * Ls / oldLen), std::memory_order_relaxed);
        else if (oldLen == 0)
            previewPos.store (0, std::memory_order_relaxed);
        std::swap (previewBuf, fresh);
    }
    // il buffer vecchio viene liberato qui, fuori dal lock e fuori dall'audio thread
}

juce::File VocalRiserAudioProcessor::writeRiserFile()
{
    using namespace juce;
    std::shared_ptr<const AudioBuffer<float>> buf;
    double rate;
    String name;
    {
        const ScopedLock sl (resultLock);
        if (rendered == nullptr) return {};
        if (renderedFile.existsAsFile()) return renderedFile;
        buf = rendered; rate = renderedSr; name = renderedName;
    }

    auto folder = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Vocal Riser");
    if (! folder.createDirectory())
        folder = File::getSpecialLocation (File::tempDirectory);

    auto file = folder.getNonexistentChildFile ("VocalRiser_" + name + "_" + Time::getCurrentTime().formatted ("%H%M%S"),
                                                ".wav", false);

    WavAudioFormat wav;
    std::unique_ptr<OutputStream> stream (file.createOutputStream());
    if (stream == nullptr) return {};

    std::unique_ptr<AudioFormatWriter> writer (wav.createWriterFor (stream.get(), rate, 2, 24, {}, 0));
    if (writer == nullptr) return {};
    stream.release(); // ora appartiene al writer

    writer->writeFromAudioSampleBuffer (*buf, 0, buf->getNumSamples());
    writer.reset();

    {
        const ScopedLock sl (resultLock);
        if (rendered == buf) renderedFile = file;   // solo se nel frattempo non e cambiato
    }
    return file;
}

//==============================================================================
void VocalRiserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("CAPTURE"), nullptr);

    // Salva nella sessione anche l'inizio della frase catturata (max 1,5 s),
    // cosi riaprendo il progetto il riser c'e ancora
    {
        const juce::ScopedLock sl (captureLock);
        const int idx = publishedIdx.load();
        if (idx >= 0 && captureLens[(size_t) idx] > 0)
        {
            const int len = juce::jmin (captureLens[(size_t) idx], (int) (1.5 * sr));
            const int chans = juce::jlimit (1, 2, captureChans[(size_t) idx]);
            juce::MemoryBlock mb;
            for (int ch = 0; ch < chans; ++ch)
                mb.append (captures[(size_t) idx].getReadPointer (ch), sizeof (float) * (size_t) len);

            juce::ValueTree c ("CAPTURE");
            c.setProperty ("sr", sr, nullptr);
            c.setProperty ("bpm", captureBpms[(size_t) idx], nullptr);
            c.setProperty ("chans", chans, nullptr);
            c.setProperty ("len", len, nullptr);
            c.setProperty ("data", mb.toBase64Encoding(), nullptr);
            state.appendChild (c, nullptr);
        }
    }

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalRiserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    auto c = tree.getChildWithName ("CAPTURE");
    if (c.isValid())
    {
        juce::MemoryBlock mb;
        const int len = c["len"], chans = juce::jlimit (1, 2, (int) c["chans"]);
        if (len > 0 && mb.fromBase64Encoding (c["data"].toString())
             && mb.getSize() >= sizeof (float) * (size_t) (len * chans))
        {
            auto r = std::make_unique<RestoredCapture>();
            r->audio.setSize (chans, len);
            for (int ch = 0; ch < chans; ++ch)
                r->audio.copyFrom (ch, 0, static_cast<const float*> (mb.getData()) + (size_t) ch * (size_t) len, len);
            r->sr = c["sr"];
            r->bpm = c["bpm"];

            const juce::ScopedLock sl (captureLock);
            restored = std::move (r);
            applyRestoredCapture();
        }
        tree.removeChild (c, nullptr);
    }

    apvts.replaceState (tree);
    paramsDirty.store (true);
}

// Chiamata con captureLock preso
void VocalRiserAudioProcessor::applyRestoredCapture()
{
    if (restored == nullptr || captureMax == 0)
        return;

    auto& src = restored->audio;
    const int slot = writeIdx ^ 1;                      // mai il buffer in cui l'audio thread sta scrivendo
    auto& dst = captures[(size_t) slot];
    const double ratio = restored->sr > 0.0 ? restored->sr / sr : 1.0;
    const int outLen = juce::jmin (captureMax, (int) (src.getNumSamples() / ratio));

    dst.clear();
    for (int ch = 0; ch < src.getNumChannels(); ++ch)
    {
        if (std::abs (ratio - 1.0) < 1.0e-6)
            dst.copyFrom (ch, 0, src, ch, 0, outLen);
        else
        {
            juce::LagrangeInterpolator interp;
            interp.process (ratio, src.getReadPointer (ch), dst.getWritePointer (ch), outLen);
        }
    }
    captureLens[(size_t) slot]  = outLen;
    captureChans[(size_t) slot] = src.getNumChannels();
    captureBpms[(size_t) slot]  = restored->bpm > 0.0 ? restored->bpm : 120.0;
    publishedIdx.store (slot);
    captureCounter.fetch_add (1);
    restored.reset();
}

juce::AudioProcessorEditor* VocalRiserAudioProcessor::createEditor()
{
    return new VocalRiserAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalRiserAudioProcessor();
}
