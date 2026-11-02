#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <array>

// Analisi della voce e allungamento del suono per il riser.
namespace RiserDsp
{
    struct Span { int start = 0, end = 0; };

    //==========================================================================
    // Trova la PRIMA SILLABA della prima parola.
    //
    // Si guarda la voce fotogramma per fotogramma (5 ms) con due misure:
    //  - il volume,
    //  - quanto il suono e "intonato" (autocorrelazione): le vocali lo sono,
    //    respiri, fruscii e consonanti come s, f, c, p, t no.
    //
    // 1) Il nucleo della sillaba e la prima vocale vera: suono intonato, abbastanza forte,
    //    che dura almeno 25 ms. Cosi un respiro o un rumore prima della frase non conta.
    // 2) L'inizio si sposta indietro per includere la consonante d'attacco (c, s, t...),
    //    ma solo se e attaccata alla vocale (niente pause in mezzo).
    // 3) La fine e il primo di questi punti:
    //    - la voce smette di essere intonata (arriva una consonante come p, t, s, c),
    //    - il volume scende e poi risale (passaggio a una nuova sillaba, anche legata),
    //    - un silenzio.
    inline Span findFirstSyllable (const juce::AudioBuffer<float>& cap, int capLen, int capCh, double sr)
    {
        Span sp { 0, juce::jmin (capLen, (int) (0.3 * sr)) };
        const int analyseLen = juce::jmin (capLen, (int) (2.5 * sr));
        const int hop = juce::jmax (1, (int) std::round (0.005 * sr));
        const int win = juce::jmax (hop * 2, (int) std::round (0.030 * sr));
        if (analyseLen < win * 3) return sp;

        // voce mono, e una versione filtrata (60 Hz - 1 kHz) per misurare l'intonazione
        std::vector<float> x ((size_t) analyseLen), lp ((size_t) analyseLen);
        for (int i = 0; i < analyseLen; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < capCh; ++ch) s += cap.getSample (ch, i);
            x[(size_t) i] = s / (float) capCh;
        }
        {
            const float aLp = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 1000.0f / (float) sr);
            const float aHp = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 60.0f / (float) sr);
            float s1 = 0.0f, s2 = 0.0f, h = 0.0f;
            for (int i = 0; i < analyseLen; ++i)
            {
                s1 += aLp * (x[(size_t) i] - s1);
                s2 += aLp * (s1 - s2);
                h  += aHp * (s2 - h);
                lp[(size_t) i] = s2 - h;
            }
        }

        // l'intonazione si misura a ~12 kHz (sotto 1 kHz basta): 16 volte meno calcoli
        const int dec = juce::jmax (1, (int) std::round (sr / 12000.0));
        const double srD = sr / dec;
        std::vector<float> lpD ((size_t) (analyseLen / dec + 1));
        for (int i = 0; i * dec < analyseLen; ++i) lpD[(size_t) i] = lp[(size_t) (i * dec)];
        const int winD   = win / dec;
        const int minLag = juce::jmax (2, (int) (srD / 1000.0));   // fino a 1 kHz (voci acute)
        const int maxLag = juce::jmin (winD - 2, (int) (srD / 65.0));
        const int nFrames = (analyseLen - win) / hop;
        std::vector<float> db ((size_t) nFrames), voicing ((size_t) nFrames);

        for (int f = 0; f < nFrames; ++f)
        {
            const int p0 = f * hop;
            double e = 0.0;
            for (int i = 0; i < win; ++i) e += (double) x[(size_t) (p0 + i)] * x[(size_t) (p0 + i)];
            db[(size_t) f] = (float) (10.0 * std::log10 (e / win + 1.0e-12));

            // autocorrelazione normalizzata sulla parte filtrata
            const float* s = lpD.data() + p0 / dec;
            double best = 0.0;
            for (int lag = minLag; lag <= maxLag; ++lag)
            {
                const int n = winD - lag;
                double c = 0.0, ea = 0.0, eb = 0.0;
                for (int i = 0; i < n; ++i)
                {
                    c  += (double) s[i] * s[i + lag];
                    ea += (double) s[i] * s[i];
                    eb += (double) s[i + lag] * s[i + lag];
                }
                if (ea > 1.0e-12 && eb > 1.0e-12) best = std::max (best, c / std::sqrt (ea * eb));
            }
            voicing[(size_t) f] = (float) best;
        }

        // forma dello spettro (4 bande, in dB rispetto alla media): cambia quando cambia la vocale
        // o arriva una consonante, anche se il volume quasi non scende (canto legato)
        std::vector<std::array<float, 4>> shape ((size_t) nFrames);
        {
            int ord = 6;
            while ((1 << ord) < win && ord < 14) ++ord;
            const int nf = 1 << ord;
            juce::dsp::FFT sfft (ord);
            std::vector<float> fb ((size_t) nf * 2);
            const double edges[5] = { 80.0, 400.0, 900.0, 1800.0, 4000.0 };
            for (int f = 0; f < nFrames; ++f)
            {
                std::fill (fb.begin(), fb.end(), 0.0f);
                for (int i = 0; i < win && i < nf; ++i)
                    fb[(size_t) i] = x[(size_t) (f * hop + i)] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) win));
                sfft.performFrequencyOnlyForwardTransform (fb.data(), true);
                float mean = 0.0f;
                for (int b = 0; b < 4; ++b)
                {
                    const int k0 = juce::jmax (1, (int) (edges[b] * nf / sr)), k1 = juce::jmax (k0 + 1, (int) (edges[b + 1] * nf / sr));
                    double e = 0.0;
                    for (int k = k0; k < k1 && k <= nf / 2; ++k) e += (double) fb[(size_t) k] * fb[(size_t) k];
                    shape[(size_t) f][(size_t) b] = (float) (10.0 * std::log10 (e / (k1 - k0) + 1.0e-12));
                    mean += shape[(size_t) f][(size_t) b] * 0.25f;
                }
                for (auto& v : shape[(size_t) f]) v -= mean;
            }
        }
        const int spanD = juce::jmax (1, (int) std::round (0.02 * sr / hop));
        auto specChange = [&] (int f)          // quanto cambia il timbro tra 20 ms prima e 20 ms dopo
        {
            const int a = juce::jmax (0, f - spanD), b = juce::jmin (nFrames - 1, f + spanD);
            float d = 0.0f;
            for (int k = 0; k < 4; ++k) { const float q = shape[(size_t) b][(size_t) k] - shape[(size_t) a][(size_t) k]; d += q * q; }
            return std::sqrt (d / 4.0f);
        };

        // volume smussato (15 ms) per trovare valli e risalite
        std::vector<float> sm ((size_t) nFrames);
        for (int f = 0; f < nFrames; ++f)
        {
            float a = 0.0f; int k = 0;
            for (int d = -1; d <= 1; ++d)
                if (f + d >= 0 && f + d < nFrames) { a += db[(size_t) (f + d)]; ++k; }
            sm[(size_t) f] = a / (float) k;
        }

        std::vector<float> sorted (db);
        std::sort (sorted.begin(), sorted.end());
        const float floorDb = sorted[sorted.size() / 10];          // rumore di fondo
        const float peakDb  = sorted.back();
        const float voiceMin = std::max ({ floorDb + 12.0f, peakDb - 24.0f, -60.0f });

        auto isVoiced = [&] (int f) { return voicing[(size_t) f] > 0.6f && sm[(size_t) f] > voiceMin; };

        // 1) nucleo: prima vocale che dura almeno 25 ms
        const int needRun = juce::jmax (2, (int) std::round (0.025 * sr / hop));
        int n0 = -1;
        for (int f = 0, run = 0; f < nFrames; ++f)
        {
            run = isVoiced (f) ? run + 1 : 0;
            if (run >= needRun) { n0 = f - run + 1; break; }
        }
        if (n0 < 0)
        {
            // niente di intonato (sussurro, rumore): usa il primo tratto forte
            int f = 0;
            while (f < nFrames && sm[(size_t) f] < peakDb - 20.0f) ++f;
            sp.start = juce::jmin (capLen - 1, f * hop);
            sp.end   = juce::jmin (capLen, sp.start + (int) (0.25 * sr));
            return sp;
        }

        // 2) consonante d'attacco attaccata alla vocale (al massimo 180 ms indietro)
        int start = n0;
        {
            const float nucleusDb = *std::max_element (sm.begin() + n0, sm.begin() + std::min (nFrames, n0 + needRun * 2));
            const float consMin   = std::max (floorDb + 8.0f, nucleusDb - 32.0f);
            const int   maxBack   = (int) std::round (0.18 * sr / hop);
            int gap = 0;
            for (int f = n0 - 1; f >= 0 && n0 - f <= maxBack; --f)
            {
                if (db[(size_t) f] > consMin) { start = f; gap = 0; }
                else if (++gap > 2) break;                         // 10 ms di pausa: prima c'era altro (respiro)
            }
        }

        // 3) fine della sillaba
        const int minNucleus  = (int) std::round (0.045 * sr / hop);
        const int unvoicedEnd = juce::jmax (2, (int) std::round (0.015 * sr / hop));
        const int riseWindow  = (int) std::round (0.09 * sr / hop);
        int end = -1;
        float localMax = -200.0f;
        int unv = 0, firstDrop = -1;            // primo punto 6 dB sotto il massimo: la vocale sta finendo
        for (int f = n0; f < nFrames; ++f)
        {
            if (sm[(size_t) f] > localMax) localMax = sm[(size_t) f];
            if (sm[(size_t) f] > localMax - 3.0f) firstDrop = -1;                 // il volume e tornato su
            else if (firstDrop < 0 && sm[(size_t) f] < localMax - 6.0f) firstDrop = f;

            // silenzio
            if (sm[(size_t) f] < std::max (floorDb + 6.0f, localMax - 30.0f)) { end = f; break; }

            // la voce smette di essere intonata
            unv = isVoiced (f) ? 0 : unv + 1;
            if (unv >= unvoicedEnd && f - n0 >= needRun) { end = f - unv + 1; break; }

            // valle seguita da una risalita: nuova sillaba (anche cantata legata)
            //   - valle profonda (8 dB) seguita da una risalita
            //   - oppure valle piu leggera (5 dB) dove cambia anche il timbro
            if (f - n0 >= minNucleus && sm[(size_t) f] < localMax - 5.0f)
            {
                int m = f;
                while (m + 1 < nFrames && sm[(size_t) (m + 1)] <= sm[(size_t) m]) ++m;   // fondo della valle
                float rise = sm[(size_t) m];
                for (int k = m; k < std::min (nFrames, m + riseWindow); ++k) rise = std::max (rise, sm[(size_t) k]);
                float change = 0.0f;
                for (int k = juce::jmax (n0, m - spanD); k <= juce::jmin (nFrames - 1, m + spanD); ++k) change = std::max (change, specChange (k));
               #ifdef RISER_SYL_DEBUG
                std::printf ("  valle a %.0f ms: discesa %.1f dB, risalita %.1f dB, cambio timbro %.1f dB\n",
                             (m * hop + win / 2) * 1000.0 / sr, localMax - sm[(size_t) m], rise - sm[(size_t) m], change);
               #endif
                const bool deep = sm[(size_t) m] < localMax - 8.0f && rise > sm[(size_t) m] + 4.0f;
                const bool soft = rise > sm[(size_t) m] + 3.0f && change > 4.0f;
                if (deep || soft)
                {
                    // fine: dove la vocale e gia scesa di 6 dB (non in fondo alla valle)
                    int e2 = firstDrop >= 0 ? firstDrop : f;
                    while (e2 < m && sm[(size_t) e2] >= localMax - 6.0f) ++e2;
                    end = e2;
                    break;
                }
            }
        }
        if (end < 0) end = nFrames - 1;
       #ifdef RISER_SYL_DEBUG
        for (int f = std::max (0, start - 2); f <= std::min (nFrames - 1, end + 4); ++f)
            std::printf ("  %5.0f ms  db %6.1f sm %6.1f  voc %.2f %s\n", (f * hop + win / 2) * 1000.0 / sr, db[(size_t) f], sm[(size_t) f], voicing[(size_t) f], isVoiced (f) ? "V" : "-");
        std::printf ("  floor %.1f peak %.1f voiceMin %.1f n0 %d end %d\n", floorDb, peakDb, voiceMin, n0, end);
       #endif

        sp.start = juce::jmax (0, start * hop + win - (int) (0.008 * sr));   // il suono entra dal bordo destro del fotogramma
        sp.end   = juce::jmin (capLen, end * hop + win / 2 + (int) (0.006 * sr));
        const int minLen = (int) (0.07 * sr), maxLen = (int) (0.45 * sr);
        if (sp.end - sp.start < minLen) sp.end = juce::jmin (capLen, sp.start + minLen);
        if (sp.end - sp.start > maxLen) sp.end = sp.start + maxLen;
        return sp;
    }

    //==========================================================================
    // Allungamento "spettrale" (stile Paulstretch): ogni fotogramma tiene il timbro della
    // voce (le ampiezze delle frequenze) ma prende fasi casuali. Il risultato e un suono
    // continuo e morbido, senza la ripetizione dei grani che rende il suono metallico.
    // independentRight = true: il canale destro ha fasi proprie (piu ampiezza stereo).
    inline void spectralStretch (const juce::AudioBuffer<float>& src, int srcLen, juce::AudioBuffer<float>& dst,
                                 int outLen, double sr, bool independentRight)
    {
        const int numCh = src.getNumChannels();
        dst.setSize (numCh, outLen, false, false, true);
        dst.clear();
        if (srcLen < 8 || outLen < 8) return;

        int order = 8;
        while ((1 << (order + 1)) <= (int) (0.09 * sr) && order < 14) ++order;   // ~85 ms a 48 kHz: tiene il timbro della voce
        const int N = 1 << order, hopOut = N / 4;

        juce::dsp::FFT fft (order);
        std::vector<float> window ((size_t) N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) N);

        std::vector<float> buf ((size_t) N * 2);
        juce::Random rng (2024);
        std::vector<float> phases ((size_t) N / 2 + 1);

        const double inSpan  = (double) juce::jmax (0, srcLen - N / 2);
        const double outSpan = (double) juce::jmax (1, outLen);
        const float  norm    = 1.0f / 1.5f;      // finestre di Hann al quadrato con sovrapposizione 4x

        for (int o = -N + hopOut; o < outLen; o += hopOut)
        {
            const double prog = juce::jlimit (0.0, 1.0, (double) (o + N / 2) / outSpan);
            const int in0 = (int) (prog * inSpan) - N / 4;

            for (auto& ph : phases) ph = rng.nextFloat() * juce::MathConstants<float>::twoPi;

            for (int ch = 0; ch < numCh; ++ch)
            {
                if (ch == 1 && independentRight)
                    for (auto& ph : phases) ph = rng.nextFloat() * juce::MathConstants<float>::twoPi;

                const float* s = src.getReadPointer (ch);
                std::fill (buf.begin(), buf.end(), 0.0f);
                for (int i = 0; i < N; ++i)
                {
                    const int idx = in0 + i;
                    if (idx >= 0 && idx < srcLen) buf[(size_t) i] = s[idx] * window[(size_t) i];
                }
                fft.performRealOnlyForwardTransform (buf.data(), true);
                for (int k = 0; k <= N / 2; ++k)
                {
                    const float re = buf[(size_t) (2 * k)], im = buf[(size_t) (2 * k + 1)];
                    const float mag = std::sqrt (re * re + im * im);
                    buf[(size_t) (2 * k)]     = mag * std::cos (phases[(size_t) k]);
                    buf[(size_t) (2 * k + 1)] = mag * std::sin (phases[(size_t) k]);
                }
                for (int k = N / 2 + 1; k < N; ++k)     // simmetria per un segnale reale
                {
                    buf[(size_t) (2 * k)]     =  buf[(size_t) (2 * (N - k))];
                    buf[(size_t) (2 * k + 1)] = -buf[(size_t) (2 * (N - k) + 1)];
                }
                fft.performRealOnlyInverseTransform (buf.data());

                float* d = dst.getWritePointer (ch);
                for (int i = 0; i < N; ++i)
                {
                    const int oi = o + i;
                    if (oi >= 0 && oi < outLen) d[oi] += buf[(size_t) i] * window[(size_t) i] * norm;
                }
            }
        }
    }
}
