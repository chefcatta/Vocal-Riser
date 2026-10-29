#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const double      beatsTable[]  = { 1.0, 2.0, 4.0, 8.0 };            // 1/4, 1/2, 1, 2 battute (4/4)
    const char* const lengthNames[] = { "1-4bar", "1-2bar", "1bar", "2bar" };
    const char* const renderParams[] = { "amount", "length", "pitchOn", "filterOn",
                                         "stutOn", "phoneOn", "chorusOn", "revDryOn" };
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
}

//==============================================================================
VocalRiserAudioProcessor::VocalRiserAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pAmount   = apvts.getRawParameterValue ("amount");
    pLength   = apvts.getRawParameterValue ("length");
    pPitchOn  = apvts.getRawParameterValue ("pitchOn");
    pFilterOn = apvts.getRawParameterValue ("filterOn");
    pStutOn   = apvts.getRawParameterValue ("stutOn");
    pPhoneOn  = apvts.getRawParameterValue ("phoneOn");
    pChorusOn = apvts.getRawParameterValue ("chorusOn");
    pRevDryOn = apvts.getRawParameterValue ("revDryOn");
    pAutoArm  = apvts.getRawParameterValue ("autoArm");

    for (auto* id : renderParams)
        apvts.addParameterListener (id, this);
}

VocalRiserAudioProcessor::~VocalRiserAudioProcessor()
{
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
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "pitchOn", 1 },  "Pitch",   true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "filterOn", 1 }, "Filtro",  true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "stutOn", 1 },   "Stutter", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "phoneOn", 1 },  "Phone",   false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "chorusOn", 1 }, "Chorus",  false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "revDryOn", 1 }, "Reverse", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "autoArm", 1 },  "Auto Cattura", true));

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
    sr = sampleRate;
    captureMax = (int) (8.5 * sr); // basta per 2 battute anche a 60 BPM

    for (auto& c : captures) { c.setSize (2, captureMax); c.clear(); }

    int ringSize = 1;
    while (ringSize < (int) (0.05 * sr)) ringSize <<= 1;
    for (auto& r : preRing) r.assign ((size_t) ringSize, 0.0f);
    ringMask = ringSize - 1;
    ringPos = 0;
    prerollLen = (int) (0.008 * sr);
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

    // Auto-cattura: ogni volta che parte il play, cattura la prima frase che arriva
    if (playing && ! wasPlaying && pAutoArm->load() > 0.5f && state != stateRecording)
        state = stateArmed;

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

    // --- Anteprima del riser ----------------------------------------------
    if (previewOn.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (previewLock);
        if (lock.isLocked() && previewBuf != nullptr && previewBuf->getNumSamples() > 0)
        {
            if (previewRestart.exchange (false)) previewPos = 0;
            const int len = previewBuf->getNumSamples();

            for (int i = 0; i < n; ++i)
            {
                if (previewPos >= len)
                {
                    if (previewLoop.load()) previewPos = 0;
                    else { previewOn.store (false); break; }
                }
                for (int ch = 0; ch < numOut; ++ch)
                    buffer.addSample (ch, i, previewBuf->getSample (juce::jmin (ch, 1), previewPos));
                ++previewPos;
            }
            previewProgress.store ((float) previewPos / (float) len);
        }
    }
}

//==============================================================================
bool VocalRiserAudioProcessor::updateRender()
{
    const int counter = captureCounter.load();
    if (counter > 0 && counter != lastRenderedCounter)
    {
        lastRenderedCounter = counter;
        paramsDirty.store (false);
        renderRiser();
        return true;
    }

    if (paramsDirty.exchange (false) && counter > 0)
    {
        renderRiser();
        return true;
    }
    return false;
}

void VocalRiserAudioProcessor::renderRiser()
{
    using namespace juce;

    const int idx = publishedIdx.load();
    if (idx < 0) return;

    const auto&  cap    = captures[(size_t) idx];
    const int    capLen = captureLens[(size_t) idx];
    const int    capCh  = jmax (1, captureChans[(size_t) idx]);
    const double bpm    = captureBpms[(size_t) idx];
    const double rate   = sr;
    if (capLen <= 0) return;

    const float  amount     = pAmount->load();
    const int    lenIdx     = jlimit (0, 3, (int) pLength->load());
    const double secPerBeat = 60.0 / bpm;
    const int    Ls         = jmax (256, (int) std::round (beatsTable[lenIdx] * secPerBeat * rate));
    const int    wordLen    = findFirstWordLength (cap, capLen, capCh, rate);
    // Solo la prima parte della prima parola (l'attacco): 40% della parola, tra 80 e 250 ms
    const int    attackLen  = jlimit ((int) (0.08 * rate), (int) (0.25 * rate), (int) (wordLen * 0.4));
    const int    seg        = jmax (1, jmin (wordLen, attackLen, Ls));

    // 1) Inizio della frase, rovesciato
    AudioBuffer<float> rev (2, seg);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* src = cap.getReadPointer (jmin (ch, capCh - 1));
        float* dst = rev.getWritePointer (ch);
        for (int j = 0; j < seg; ++j) dst[j] = src[seg - 1 - j];
    }
    // la fine della parola (ora all'inizio) entra morbida
    const int wordFade = jmin (seg / 3, (int) (0.02 * rate));
    for (int ch = 0; ch < 2; ++ch) rev.applyGainRamp (ch, 0, wordFade, 0.0f, 1.0f);

    // 2) Eco a tempo + riverbero (solo wet) sulla voce rovesciata
    const int total = seg + Ls;
    AudioBuffer<float> wet (2, total);
    wet.clear();
    for (int ch = 0; ch < 2; ++ch) wet.copyFrom (ch, 0, rev, ch, 0, seg);

    const int   dly = jmax (1, (int) (0.75 * secPerBeat * rate));
    const float fb  = 0.30f + 0.30f * amount;
    for (int ch = 0; ch < 2; ++ch)
    {
        float* w = wet.getWritePointer (ch);
        const int offset = ch == 0 ? dly : dly + dly / 3; // leggero ping-pong
        for (int j = offset; j < total; ++j) w[j] += fb * w[j - offset];
    }

    Reverb rv;
    rv.setSampleRate (rate);
    Reverb::Parameters rp;
    rp.roomSize   = jmin (0.985f, 0.80f + 0.16f * amount + (lenIdx >= 2 ? 0.04f * (float) (lenIdx - 1) : 0.0f));
    rp.damping    = 0.35f;
    rp.wetLevel   = 0.33f;
    rp.dryLevel   = 0.0f;
    rp.width      = 1.0f;
    rp.freezeMode = 0.0f;
    rv.setParameters (rp);
    for (int pos = 0; pos < total; pos += 512)
        rv.processStereo (wet.getWritePointer (0) + pos, wet.getWritePointer (1) + pos, jmin (512, total - pos));

    // 3) Coda rovesciata: il riser finisce esattamente dove parte la voce
    AudioBuffer<float> out (2, Ls);
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* w = wet.getReadPointer (ch);
        float* o = out.getWritePointer (ch);
        for (int j = 0; j < Ls; ++j) o[j] = w[seg + Ls - 1 - j];
    }

    auto normalise = [] (AudioBuffer<float>& b, float targetPeak)
    {
        const float mag = b.getMagnitude (0, b.getNumSamples());
        if (mag > 1.0e-6f) b.applyGain (targetPeak / mag);
    };
    normalise (out, 0.5f);

    // 4) Voce rovesciata sotto al riverbero
    if (pRevDryOn->load() > 0.5f)
    {
        const float revMag = rev.getMagnitude (0, seg);
        const float g = revMag > 1.0e-6f ? 0.35f / revMag : 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            out.addFrom (ch, Ls - seg, rev, ch, 0, seg, g);
    }

    // 5) Pitch che sale (con compensazione della latenza)
    if (pPitchOn->load() > 0.5f)
    {
        PitchShifter ps[2];
        ps[0].prepare (rate); ps[1].prepare (rate);
        const int lat = ps[0].getLatency();
        const float semisMax = 5.0f + 7.0f * amount;

        AudioBuffer<float> shifted (2, Ls);
        for (int j = 0; j < Ls + lat; ++j)
        {
            const float p     = (float) jmin (j, Ls - 1) / (float) (Ls - 1);
            const float semis = semisMax * p;
            const float ratio = std::exp2 (semis / 12.0f);
            const float mix   = jlimit (0.0f, 1.0f, semis / 0.3f);
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = j < Ls ? out.getSample (ch, j) : 0.0f;
                const float y = ps[ch].process (x, ratio);
                const int o = j - lat;
                if (o >= 0)
                {
                    const float dry = out.getSample (ch, o);
                    shifted.setSample (ch, o, dry + mix * (y - dry));
                }
            }
        }
        out.makeCopyOf (shifted, true);
    }

    // 6) Filtro, phone, stutter, crescendo
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
    float gate = 1.0f;

    for (int j = 0; j < Ls; ++j)
    {
        const float p = (float) j / (float) (Ls - 1);

        if (filterOn && (j & 15) == 0)
            hp.setCutoffFrequency (20.0f * std::pow (fMax / 20.0f, p));

        float gateTarget = 1.0f;
        if (stutOn && p > 0.55f)
        {
            const float  q      = (p - 0.55f) / 0.45f;
            const int    k      = jmin (2, (int) (q * 3.0f));   // 1/8 -> 1/16 -> 1/32
            const double period = 0.5 / (double) (1 << k);
            double ph = (j / (secPerBeat * rate)) / period;
            ph -= std::floor (ph);
            gateTarget = ph < 0.6 ? 1.0f : 0.0f;
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

    // 7) Normalizza a -1 dBFS e micro-fade
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
    peaks.assign (bins, 0.0f);
    for (int b = 0; b < bins; ++b)
    {
        const int s0 = (int) ((int64) b * Ls / bins);
        const int s1 = jmax (s0 + 1, (int) ((int64) (b + 1) * Ls / bins));
        float m = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            m = jmax (m, out.getMagnitude (ch, s0, s1 - s0));
        peaks[(size_t) b] = m;
    }

    rendered.makeCopyOf (out);
    renderedFile = File();
    renderReady = true;
    renderInfo = String (roundToInt (bpm)) + " BPM  ·  " + String (Ls / rate, 2) + " s  ·  attacco "
                 + String (seg / rate, 2) + " s";

    // 9) Passa la copia all'anteprima
    auto fresh = std::make_unique<AudioBuffer<float>> (out);
    {
        const SpinLock::ScopedLockType lock (previewLock);
        std::swap (previewBuf, fresh);
        previewPos = 0;
    }
}

juce::File VocalRiserAudioProcessor::writeRiserFile()
{
    using namespace juce;
    if (! renderReady) return {};
    if (renderedFile.existsAsFile()) return renderedFile;

    auto folder = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Vocal Riser");
    if (! folder.createDirectory())
        folder = File::getSpecialLocation (File::tempDirectory);

    const int lenIdx = jlimit (0, 3, (int) pLength->load());
    const int idx = jmax (0, publishedIdx.load());
    auto file = folder.getNonexistentChildFile ("VocalRiser_" + String (roundToInt (captureBpms[(size_t) idx])) + "bpm_"
                                                    + lengthNames[lenIdx] + "_"
                                                    + Time::getCurrentTime().formatted ("%H%M%S"),
                                                ".wav", false);

    WavAudioFormat wav;
    std::unique_ptr<OutputStream> stream (file.createOutputStream());
    if (stream == nullptr) return {};

    std::unique_ptr<AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
    if (writer == nullptr) return {};
    stream.release(); // ora appartiene al writer

    writer->writeFromAudioSampleBuffer (rendered, 0, rendered.getNumSamples());
    writer.reset();

    renderedFile = file;
    return file;
}

//==============================================================================
void VocalRiserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalRiserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VocalRiserAudioProcessor::createEditor()
{
    return new VocalRiserAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalRiserAudioProcessor();
}
