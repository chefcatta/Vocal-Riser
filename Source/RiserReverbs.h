#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

// Riverberi del riser. Tutti lavorano al 100% wet (nessun segnale dry in uscita).
// Obiettivo: suono naturale. Per questo ogni algoritmo ha
//  - pre-delay e prime riflessioni (la "stanza" prima della coda),
//  - linee di ritardo modulate lentamente (niente risonanze metalliche),
//  - smorzamento delle alte frequenze che cresce nel tempo, come in una sala vera,
//  - filtri in ingresso/uscita che tolgono rimbombo sui bassi e asprezza sugli alti.
namespace RiserReverbs
{
    enum Type  { hall = 0, plate, space };
    enum Image { monoOut = 0, monoToStereo, keepStereo };

    //==========================================================================
    // Linea di ritardo con lettura frazionaria (per la modulazione)
    struct Delay
    {
        void init (int maxLen)
        {
            int n = 1;
            while (n < maxLen + 4) n <<= 1;
            buf.assign ((size_t) n, 0.0f);
            mask = n - 1;
            pos = 0;
        }
        float tap (int d) const                 { return buf[(size_t) ((pos - d) & mask)]; }
        float tapFrac (float d) const
        {
            const int   i = (int) d;
            const float f = d - (float) i;
            const float a = buf[(size_t) ((pos - i) & mask)];
            const float b = buf[(size_t) ((pos - i - 1) & mask)];
            return a + f * (b - a);
        }
        void push (float x)                     { buf[(size_t) pos] = x; pos = (pos + 1) & mask; }

        std::vector<float> buf;
        int mask = 0, pos = 0;
    };

    struct Allpass
    {
        void init (int len, float coeff) { length = juce::jmax (1, len); g = coeff; line.init (length + 64); }
        float process (float x, float mod = 0.0f)
        {
            const float d = mod != 0.0f ? line.tapFrac ((float) length + mod) : line.tap (length);
            const float w = x + g * d;
            line.push (w);
            return d - g * w;
        }
        Delay line;
        int length = 1;
        float g = 0.5f;
    };

    struct OnePole
    {
        void setLowpass (float hz, double sr)  { c = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sr); }
        float lp (float x)                      { z += c * (x - z); return z; }
        float hp (float x)                      { z += c * (x - z); return x - z; }
        float c = 1.0f, z = 0.0f;
    };

    //==========================================================================
    // Rete a ritardi retroazionati (FDN 8x8) con prime riflessioni e modulazione.
    // Usata per HALL (sala naturale) e SPACE (ambiente enorme e scuro).
    class FDN
    {
    public:
        struct Settings
        {
            double sizeMs[8];       // lunghezze delle linee
            double t60;             // tempo di decadimento (s)
            float  hfDampHz;        // taglio dello smorzamento sugli alti
            float  modDepthMs;      // ampiezza della modulazione
            float  preDelayMs;
            float  erLevel;         // livello delle prime riflessioni
        };

        void prepare (double sampleRate, const Settings& s)
        {
            sr = sampleRate;
            for (int i = 0; i < 8; ++i)
            {
                len[i] = (float) (s.sizeMs[i] * 0.001 * sr);
                lines[(size_t) i].init ((int) len[i] + (int) (s.modDepthMs * 0.001 * sr) + 8);
                gain[i] = (float) std::pow (10.0, -3.0 * len[i] / (s.t60 * sr));
                damp[(size_t) i].setLowpass (s.hfDampHz, sr);
                damp[(size_t) i].z = 0.0f;
                lfoPhase[i] = (float) i * 0.785f;
                lfoInc[i]   = juce::MathConstants<float>::twoPi * (0.07f + 0.031f * (float) i) / (float) sr;
            }
            modDepth = s.modDepthMs * 0.001f * (float) sr;
            preDelay = juce::jmax (1, (int) (s.preDelayMs * 0.001 * sr));
            pre.init (preDelay + 8);
            er.init ((int) (0.09 * sr));
            erLevel = s.erLevel;
            for (int k = 0; k < numTaps; ++k)
                erTap[k] = juce::jmax (1, (int) (erMs[k] * 0.001 * sr));
            static constexpr double dMs[6] = { 4.7, 7.1, 11.3, 15.7, 22.3, 31.1 };
            for (int k = 0; k < 6; ++k)
                diff[(size_t) k].init ((int) (dMs[k] * 0.001 * sr), k < 4 ? 0.68f : 0.6f);
            static constexpr double oL[2] = { 5.3, 7.9 }, oR[2] = { 6.1, 8.7 };
            for (int k = 0; k < 2; ++k)
            {
                outDiffL[(size_t) k].init ((int) (oL[k] * 0.001 * sr), 0.5f);
                outDiffR[(size_t) k].init ((int) (oR[k] * 0.001 * sr), 0.5f);
            }
        }

        void process (float in, float& outL, float& outR)
        {
            pre.push (in);
            const float x0 = pre.tap (preDelay);

            // diffusione d'ingresso: il segnale diventa denso prima ancora di entrare nella coda
            float x = x0;
            for (auto& d : diff) x = d.process (x);

            // prime riflessioni, prese dal segnale gia diffuso (niente echi singoli udibili)
            er.push (x);
            float erL = 0.0f, erR = 0.0f;
            for (int k = 0; k < numTaps; ++k)
                (k & 1 ? erR : erL) += er.tap (erTap[k]) * erGain[k];

            float y[8];
            for (int i = 0; i < 8; ++i)
            {
                lfoPhase[i] += lfoInc[i];
                if (lfoPhase[i] > juce::MathConstants<float>::twoPi) lfoPhase[i] -= juce::MathConstants<float>::twoPi;
                const float d = len[i] + modDepth * (0.5f + 0.5f * std::sin (lfoPhase[i]));
                y[i] = damp[(size_t) i].lp (lines[(size_t) i].tapFrac (d)) * gain[i];
            }

            // matrice di Hadamard normalizzata: distribuisce l'energia senza colorare
            float h[8];
            for (int i = 0; i < 8; ++i) h[i] = y[i];
            for (int step = 1; step < 8; step <<= 1)
                for (int i = 0; i < 8; i += step * 2)
                    for (int j = i; j < i + step; ++j)
                    {
                        const float p = h[j], q = h[j + step];
                        h[j] = p + q; h[j + step] = p - q;
                    }
            constexpr float norm = 0.35355339f;
            for (int i = 0; i < 8; ++i)
                lines[(size_t) i].push (h[i] * norm + x * inSign[i] * 0.35f);

            float l = 0.45f * ( y[0] - y[2] + y[4] - y[6]) + erLevel * erL;
            float r = 0.45f * ( y[1] - y[3] + y[5] - y[7]) + erLevel * erR;
            for (auto& d : outDiffL) l = d.process (l);
            for (auto& d : outDiffR) r = d.process (r);
            outL = l; outR = r;
        }

    private:
        static constexpr int numTaps = 10;
        static constexpr float erMs[numTaps]   = { 3.1f, 5.3f, 8.9f, 12.7f, 16.3f, 21.1f, 26.9f, 33.7f, 41.3f, 52.1f };
        static constexpr float erGain[numTaps] = { 0.62f, 0.58f, 0.50f, 0.47f, 0.40f, 0.36f, 0.31f, 0.27f, 0.21f, 0.17f };
        static constexpr float inSign[8]       = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f };

        double sr = 44100.0;
        std::array<Delay, 8> lines;
        std::array<OnePole, 8> damp;
        std::array<Allpass, 6> diff;
        std::array<Allpass, 2> outDiffL, outDiffR;
        Delay pre, er;
        float len[8] {}, gain[8] {}, lfoPhase[8] {}, lfoInc[8] {};
        float modDepth = 0.0f, erLevel = 0.3f;
        int preDelay = 1, erTap[numTaps] {};
    };

    //==========================================================================
    // PLATE: lastra densa e brillante (topologia di Dattorro) con modulazione
    class Plate
    {
    public:
        void prepare (double sampleRate, float amount)
        {
            sr = sampleRate;
            const double k = sr / 29761.0;
            auto s = [k] (int n) { return juce::jmax (1, (int) std::round (n * k)); };

            pre.init (s (300)); preLen = s (300);   // ~10 ms di pre-delay
            inDiff[0].init (s (142), 0.75f);  inDiff[1].init (s (107), 0.75f);
            inDiff[2].init (s (379), 0.625f); inDiff[3].init (s (277), 0.625f);

            apA1.init (s (672), 0.7f);  dA1len = s (4453); dA1.init (dA1len);
            apA2.init (s (1800), 0.5f); dA2len = s (3720); dA2.init (dA2len);
            apB1.init (s (908), 0.7f);  dB1len = s (4217); dB1.init (dB1len);
            apB2.init (s (2656), 0.5f); dB2len = s (3163); dB2.init (dB2len);

            for (int i = 0; i < 7; ++i) { tapL[i] = s (tapLRaw[i]); tapR[i] = s (tapRRaw[i]); }

            excursion = (float) (12.0 * k);
            lfoInc = juce::MathConstants<float>::twoPi * 0.6f / (float) sr;
            lfo = 0.0f;
            decay = 0.45f + 0.35f * amount;        // coda piu corta e naturale di prima
            damp = 0.35f;
            bw = 0.75f;
            lpIn = lpA = lpB = fbA = fbB = 0.0f;
        }

        void process (float in, float& outL, float& outR)
        {
            pre.push (in);
            lpIn += bw * (pre.tap (preLen) - lpIn);
            float x = lpIn;
            for (auto& a : inDiff) x = a.process (x);

            lfo += lfoInc;
            if (lfo > juce::MathConstants<float>::twoPi) lfo -= juce::MathConstants<float>::twoPi;
            const float m1 = excursion * std::sin (lfo), m2 = excursion * std::cos (lfo);

            float a = apA1.process (x + fbB * decay, m1);
            dA1.push (a);
            lpA += (1.0f - damp) * (dA1.tap (dA1len) - lpA);
            dA2.push (apA2.process (lpA * decay));
            fbA = dA2.tap (dA2len);

            float b = apB1.process (x + fbA * decay, m2);
            dB1.push (b);
            lpB += (1.0f - damp) * (dB1.tap (dB1len) - lpB);
            dB2.push (apB2.process (lpB * decay));
            fbB = dB2.tap (dB2len);

            outL = 0.6f * ( dB1.tap (tapL[0]) + dB1.tap (tapL[1]) - apB2.line.tap (tapL[2]) + dB2.tap (tapL[3])
                          - dA1.tap (tapL[4]) - apA2.line.tap (tapL[5]) - dA2.tap (tapL[6]));
            outR = 0.6f * ( dA1.tap (tapR[0]) + dA1.tap (tapR[1]) - apA2.line.tap (tapR[2]) + dA2.tap (tapR[3])
                          - dB1.tap (tapR[4]) - apB2.line.tap (tapR[5]) - dB2.tap (tapR[6]));
        }

    private:
        static constexpr int tapLRaw[7] = { 266, 2974, 1913, 1996, 1990, 187, 1066 };
        static constexpr int tapRRaw[7] = { 353, 3627, 1228, 2673, 2111, 335, 121 };

        double sr = 44100.0;
        Delay pre; int preLen = 1;
        std::array<Allpass, 4> inDiff;
        Allpass apA1, apA2, apB1, apB2;
        Delay dA1, dA2, dB1, dB2;
        int dA1len = 1, dA2len = 1, dB1len = 1, dB2len = 1;
        int tapL[7] {}, tapR[7] {};
        float excursion = 0.0f, lfo = 0.0f, lfoInc = 0.0f;
        float decay = 0.8f, damp = 0.3f, bw = 0.75f;
        float lpIn = 0.0f, lpA = 0.0f, lpB = 0.0f, fbA = 0.0f, fbB = 0.0f;
    };

    //==========================================================================
    inline FDN::Settings hallSettings (float amount)
    {
        const double sc = 1.0 + 0.35 * amount;   // la sala si allarga con l'intensita
        return { { 31.7 * sc, 37.3 * sc, 41.9 * sc, 47.1 * sc, 53.9 * sc, 59.3 * sc, 67.1 * sc, 73.7 * sc },
                 1.6 + 1.8 * amount, 6500.0f, 0.9f, 18.0f, 0.42f };
    }

    inline FDN::Settings spaceSettings (float amount)
    {
        return { { 61.3, 71.9, 83.3, 97.1, 109.7, 127.3, 139.9, 157.1 },
                 4.5 + 5.0 * amount, 3800.0f, 2.2f, 35.0f, 0.18f };
    }

    //==========================================================================
    // Applica il riverbero scelto in place (risultato 100% wet) su un buffer stereo.
    // tailSeconds: quanto deve durare la coda. Per i riser lunghi la sala decade piu lentamente,
    // cosi la coda copre il riser senza doverla allungare artificialmente.
    inline void apply (juce::AudioBuffer<float>& buf, int numSamples, Type type, Image image, float amount, double sr,
                       double tailSeconds = 0.0)
    {
        float* L = buf.getWritePointer (0);
        float* R = buf.getWritePointer (1);

        // filtri in ingresso: niente rimbombo sotto i 150 Hz
        OnePole inHpL, inHpR;
        inHpL.setLowpass (150.0f, sr); inHpR.setLowpass (150.0f, sr);
        // filtri in uscita: alti morbidi, come in un ambiente reale
        const float outLpHz = type == plate ? 9500.0f : type == hall ? 7500.0f : 5200.0f;
        OnePole outL1, outR1, outL2, outR2, outHpL, outHpR;
        for (auto* f : { &outL1, &outR1, &outL2, &outR2 }) f->setLowpass (outLpHz, sr);
        outHpL.setLowpass (180.0f, sr); outHpR.setLowpass (180.0f, sr);

        auto run = [&] (auto& left, auto& right)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float inL = inHpL.hp (L[i]), inR = inHpR.hp (R[i]);
                float l1, r1, l2, r2;
                if (image == keepStereo)
                {
                    left.process  (inL, l1, r1);
                    right.process (inR, l2, r2);
                    // ogni canale tiene il proprio lato, con un po' dell'altro per naturalezza
                    l1 = 0.8f * l1 + 0.2f * r1;
                    r1 = 0.8f * r2 + 0.2f * l2;
                }
                else
                {
                    left.process (0.5f * (inL + inR), l1, r1);
                }
                L[i] = outHpL.hp (outL2.lp (outL1.lp (l1)));
                R[i] = outHpR.hp (outR2.lp (outR1.lp (r1)));
            }
        };

        if (type == plate)
        {
            Plate a, b; a.prepare (sr, amount); b.prepare (sr, amount); run (a, b);
        }
        else
        {
            auto s = type == hall ? hallSettings (amount) : spaceSettings (amount);
            s.t60 = std::max (s.t60, std::min (6.0, 0.6 * tailSeconds));
            FDN a, b; a.prepare (sr, s);
            auto s2 = s;                                        // il canale destro e leggermente diverso
            for (auto& ms : s2.sizeMs) ms *= 1.023;
            b.prepare (sr, s2);
            run (a, b);
        }

        if (image == monoOut)
            for (int i = 0; i < numSamples; ++i)
                L[i] = R[i] = 0.5f * (L[i] + R[i]);
    }
}
