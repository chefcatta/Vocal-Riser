#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// Algoritmi di riverbero usati per creare la coda del riser.
// Tutti lavorano al 100% wet: in uscita non c'e mai segnale dry.
namespace RiserReverbs
{
    enum Type  { hall = 0, plate, space };
    enum Image { monoOut = 0, monoToStereo, keepStereo };

    //==========================================================================
    struct Delay
    {
        void init (int maxLen) { buf.assign ((size_t) juce::jmax (2, maxLen + 1), 0.0f); pos = 0; }
        float tap (int d) const
        {
            const int n = (int) buf.size();
            int i = pos - d;
            while (i < 0) i += n;
            return buf[(size_t) i];
        }
        void push (float x) { buf[(size_t) pos] = x; if (++pos >= (int) buf.size()) pos = 0; }
        std::vector<float> buf;
        int pos = 0;
    };

    struct Allpass
    {
        void init (int len, float coeff) { length = juce::jmax (1, len); g = coeff; line.init (length); }
        float process (float x)
        {
            const float d = line.tap (length);
            const float w = x + g * d;
            line.push (w);
            return d - g * w;
        }
        Delay line;
        int length = 1;
        float g = 0.5f;
    };

    //==========================================================================
    // PLATE: plate denso e brillante (topologia "figure-eight" di Dattorro)
    class Plate
    {
    public:
        void prepare (double sr, float amount)
        {
            const double k = sr / 29761.0;
            auto s = [k] (int n) { return juce::jmax (1, (int) std::round (n * k)); };

            inDiff[0].init (s (142), 0.75f);  inDiff[1].init (s (107), 0.75f);
            inDiff[2].init (s (379), 0.625f); inDiff[3].init (s (277), 0.625f);

            apA1.init (s (672), 0.7f);  dA1len = s (4453); dA1.init (dA1len);
            apA2.init (s (1800), 0.5f); dA2len = s (3720); dA2.init (dA2len);
            apB1.init (s (908), 0.7f);  dB1len = s (4217); dB1.init (dB1len);
            apB2.init (s (2656), 0.5f); dB2len = s (3163); dB2.init (dB2len);

            for (int i = 0; i < 7; ++i) { tapL[i] = s (tapLRaw[i]); tapR[i] = s (tapRRaw[i]); }

            decay = 0.55f + 0.4f * amount;       // fino a ~0.95
            damp  = 0.22f;
            bw    = 0.7f;
            lpIn = lpA = lpB = 0.0f;
            fbA = fbB = 0.0f;
        }

        void process (float in, float& outL, float& outR)
        {
            lpIn += bw * (in - lpIn);
            float x = lpIn;
            for (auto& a : inDiff) x = a.process (x);

            // meta A
            float a = apA1.process (x + fbB * decay);
            dA1.push (a);
            float aDel = dA1.tap (dA1len);
            lpA += (1.0f - damp) * (aDel - lpA);
            float a2 = apA2.process (lpA * decay);
            dA2.push (a2);
            fbA = dA2.tap (dA2len);

            // meta B
            float b = apB1.process (x + fbA * decay);
            dB1.push (b);
            float bDel = dB1.tap (dB1len);
            lpB += (1.0f - damp) * (bDel - lpB);
            float b2 = apB2.process (lpB * decay);
            dB2.push (b2);
            fbB = dB2.tap (dB2len);

            outL = 0.6f * ( dB1.tap (tapL[0]) + dB1.tap (tapL[1]) - apB2.line.tap (tapL[2]) + dB2.tap (tapL[3])
                          - dA1.tap (tapL[4]) - apA2.line.tap (tapL[5]) - dA2.tap (tapL[6]));
            outR = 0.6f * ( dA1.tap (tapR[0]) + dA1.tap (tapR[1]) - apA2.line.tap (tapR[2]) + dA2.tap (tapR[3])
                          - dB1.tap (tapR[4]) - apB2.line.tap (tapR[5]) - dB2.tap (tapR[6]));
        }

    private:
        static constexpr int tapLRaw[7] = { 266, 2974, 1913, 1996, 1990, 187, 1066 };
        static constexpr int tapRRaw[7] = { 353, 3627, 1228, 2673, 2111, 335, 121 };

        std::array<Allpass, 4> inDiff;
        Allpass apA1, apA2, apB1, apB2;
        Delay dA1, dA2, dB1, dB2;
        int dA1len = 1, dA2len = 1, dB1len = 1, dB2len = 1;
        int tapL[7] {}, tapR[7] {};
        float decay = 0.8f, damp = 0.2f, bw = 0.7f;
        float lpIn = 0.0f, lpA = 0.0f, lpB = 0.0f, fbA = 0.0f, fbB = 0.0f;
    };

    //==========================================================================
    // SPACE: rete di 8 linee di ritardo (FDN), coda enorme, scura e morbida
    class Space
    {
    public:
        void prepare (double sr, float amount)
        {
            static constexpr double ms[8] = { 47.3, 56.9, 67.1, 79.7, 91.3, 103.9, 117.7, 131.3 };
            const double t60 = 4.0 + 8.0 * amount; // secondi
            for (int i = 0; i < 8; ++i)
            {
                len[i] = juce::jmax (1, (int) (ms[i] * 0.001 * sr));
                lines[(size_t) i].init (len[i]);
                gain[i] = (float) std::pow (10.0, -3.0 * len[i] / (t60 * sr));
                lp[i] = 0.0f;
            }
            pre[0].init ((int) (0.0071 * sr), 0.7f);
            pre[1].init ((int) (0.0113 * sr), 0.7f);
            pre[2].init ((int) (0.0173 * sr), 0.6f);
            damp = 0.45f;
        }

        void process (float in, float& outL, float& outR)
        {
            float x = in;
            for (auto& a : pre) x = a.process (x);

            float y[8];
            for (int i = 0; i < 8; ++i)
            {
                const float d = lines[(size_t) i].tap (len[i]);
                lp[i] += (1.0f - damp) * (d - lp[i]);
                y[i] = lp[i] * gain[i];
            }

            // Matrice di Hadamard 8x8 (normalizzata): mescola l'energia tra le linee
            float h[8];
            for (int i = 0; i < 8; ++i) h[i] = y[i];
            for (int step = 1; step < 8; step <<= 1)
                for (int i = 0; i < 8; i += step * 2)
                    for (int j = i; j < i + step; ++j)
                    {
                        const float p = h[j], q = h[j + step];
                        h[j] = p + q; h[j + step] = p - q;
                    }
            constexpr float norm = 0.35355339f; // 1/sqrt(8)

            for (int i = 0; i < 8; ++i)
                lines[(size_t) i].push (h[i] * norm + (((i >> 1) & 1) ? -x : x) * 0.5f);

            outL = 0.5f * (y[0] + y[2] + y[4] + y[6]);
            outR = 0.5f * (y[1] + y[3] + y[5] + y[7]);
        }

    private:
        std::array<Delay, 8> lines;
        std::array<Allpass, 3> pre;
        int len[8] {};
        float gain[8] {}, lp[8] {};
        float damp = 0.4f;
    };

    //==========================================================================
    // Applica il riverbero scelto in place (risultato 100% wet) su un buffer stereo.
    inline void apply (juce::AudioBuffer<float>& buf, int numSamples, Type type, Image image, float amount, double sr)
    {
        float* L = buf.getWritePointer (0);
        float* R = buf.getWritePointer (1);

        if (type == hall)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize   = juce::jmin (0.985f, 0.80f + 0.16f * amount);
            rp.damping    = 0.35f;
            rp.wetLevel   = 0.33f;
            rp.dryLevel   = 0.0f;
            rp.width      = 1.0f;
            rp.freezeMode = 0.0f;

            if (image == keepStereo)
            {
                // un riverbero per canale: l'immagine stereo della sorgente resta intatta
                juce::Reverb rl, rr;
                for (auto* r : { &rl, &rr }) { r->setSampleRate (sr); r->setParameters (rp); }
                for (int pos = 0; pos < numSamples; pos += 512)
                {
                    const int n = juce::jmin (512, numSamples - pos);
                    rl.processMono (L + pos, n);
                    rr.processMono (R + pos, n);
                }
            }
            else
            {
                juce::Reverb rv;
                rv.setSampleRate (sr);
                rp.width = image == monoToStereo ? 1.0f : 0.0f;
                rv.setParameters (rp);
                for (int pos = 0; pos < numSamples; pos += 512)
                    rv.processStereo (L + pos, R + pos, juce::jmin (512, numSamples - pos));
            }
        }
        else
        {
            auto run = [&] (auto& left, auto& right)
            {
                left.prepare (sr, amount);
                right.prepare (sr, amount);
                for (int i = 0; i < numSamples; ++i)
                {
                    float l1, r1, l2, r2;
                    if (image == keepStereo)
                    {
                        left.process  (L[i], l1, r1);
                        right.process (R[i], l2, r2);
                        L[i] = l1; R[i] = r2;
                    }
                    else
                    {
                        left.process (0.5f * (L[i] + R[i]), l1, r1);
                        L[i] = l1; R[i] = r1;
                    }
                }
            };

            if (type == plate) { Plate a, b; run (a, b); }
            else               { Space a, b; run (a, b); }
        }

        if (image == monoOut)
            for (int i = 0; i < numSamples; ++i)
                L[i] = R[i] = 0.5f * (L[i] + R[i]);
    }
}
