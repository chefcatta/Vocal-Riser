#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
// Palette "cucina": carta, inchiostro e ingredienti
namespace Ink
{
    const juce::Colour ink      { 0xff2a2320 };   // inchiostro (contorni e testi)
    const juce::Colour paper    { 0xfff4ead5 };   // fondo
    const juce::Colour cream    { 0xfffbf5e6 };   // targhette e pulsanti
    const juce::Colour mint     { 0xffcfe3d2 };   // pannello della frase
    const juce::Colour navy     { 0xff2f3a64 };   // pannello della pizza (forno)
    const juce::Colour butter   { 0xfff2db8e };   // pannello della ricetta
    const juce::Colour tomato   { 0xffd9452b };
    const juce::Colour basil    { 0xff4f8a3c };
    const juce::Colour wood     { 0xffe0b47c };
    const juce::Colour woodDark { 0xffbf8f55 };
    const juce::Colour crust    { 0xffe3a457 };
    const juce::Colour cheese   { 0xfffff0c4 };
}

namespace
{
    //==========================================================================
    // Font disegnati a mano (SIL OFL): Patrick Hand per le scritte, Gochi Hand per il titolo
    struct HandFonts
    {
        juce::Typeface::Ptr hand, title;
        HandFonts()
        {
            hand  = juce::Typeface::createSystemTypefaceFor (BinaryData::PatrickHandRegular_ttf, (size_t) BinaryData::PatrickHandRegular_ttfSize);
            title = juce::Typeface::createSystemTypefaceFor (BinaryData::GochiHandRegular_ttf,   (size_t) BinaryData::GochiHandRegular_ttfSize);
        }
        static HandFonts& get() { static HandFonts f; return f; }
    };

    juce::Font handFont (float h, float kerning = 0.04f)
    {
        return juce::Font (juce::FontOptions (HandFonts::get().hand).withHeight (h)).withExtraKerningFactor (kerning);
    }
    juce::Font titleFont (float h)
    {
        return juce::Font (juce::FontOptions (HandFonts::get().title).withHeight (h));
    }

    //==========================================================================
    // Tratto "a mano": il contorno viene ricampionato e spostato con un rumore morbido
    juce::Path wobble (const juce::Path& src, float amount, int seed, bool closed = true, float step = 5.0f)
    {
        juce::Path out;
        const float len = src.getLength();
        if (len <= 0.0f) return src;
        const int n = juce::jmax (6, (int) (len / step));
        const float s = (float) seed;
        for (int i = 0; i <= n; ++i)
        {
            if (closed && i == n) break;
            const float d  = len * (float) i / (float) n;
            const auto  p  = src.getPointAlongPath (d);
            const auto  p2 = src.getPointAlongPath (juce::jmin (len, d + 1.0f));
            auto t = p2 - p;
            const float tl = std::sqrt (t.x * t.x + t.y * t.y);
            const juce::Point<float> nrm = tl > 0.0f ? juce::Point<float> (-t.y / tl, t.x / tl) : juce::Point<float>();
            const float fi = (float) i;
            const float off = amount * (0.6f * std::sin (fi * 0.83f + s * 1.3f) + 0.4f * std::sin (fi * 0.29f + s * 2.7f));
            const auto q = p + nrm * off;
            if (i == 0) out.startNewSubPath (q); else out.lineTo (q);
        }
        if (closed) out.closeSubPath();
        return out;
    }

    void sketch (juce::Graphics& g, const juce::Path& p, int seed, float width = 2.2f,
                 juce::Colour c = Ink::ink, bool closed = true)
    {
        g.setColour (c);
        g.strokePath (wobble (p, 0.9f, seed, closed), juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withMultipliedAlpha (0.35f));
        g.strokePath (wobble (p, 1.3f, seed + 17, closed), juce::PathStrokeType (width * 0.45f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // riquadro disegnato: ombra piatta, riempimento, contorno a mano
    void sketchBox (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill, int seed,
                    float width = 2.2f, float shadow = 3.0f)
    {
        juce::Path p;
        p.addRoundedRectangle (r, radius);
        const auto w = wobble (p, 0.8f, seed);
        if (shadow > 0.0f)
        {
            g.setColour (Ink::ink.withAlpha (0.22f));
            g.fillPath (w, juce::AffineTransform::translation (shadow * 0.7f, shadow));
        }
        g.setColour (fill);
        g.fillPath (w);
        sketch (g, p, seed, width);
    }

    void handText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r,
                   juce::Justification j, juce::Colour c = Ink::ink)
    {
        g.setColour (c);
        g.drawText (t, r, j, false);
    }

    // viti del pannello: cerchietto con il taglio
    void screw (juce::Graphics& g, juce::Point<float> c, int seed)
    {
        const float r = 4.2f;
        g.setColour (Ink::cream);
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        juce::Path p; p.addEllipse (c.x - r, c.y - r, r * 2, r * 2);
        sketch (g, p, seed, 1.4f);
        const float a = 0.6f + (float) (seed % 5) * 0.4f;
        juce::Path l;
        l.startNewSubPath (c.x - std::cos (a) * r * 0.6f, c.y - std::sin (a) * r * 0.6f);
        l.lineTo (c.x + std::cos (a) * r * 0.6f, c.y + std::sin (a) * r * 0.6f);
        g.setColour (Ink::ink);
        g.strokePath (l, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // cappello da chef
    void chefHat (juce::Graphics& g, juce::Rectangle<float> r, int seed)
    {
        const float w = r.getWidth(), h = r.getHeight();
        const auto P = [&] (float fx, float fy) { return juce::Point<float> (r.getX() + fx * w, r.getY() + fy * h); };
        juce::Path hat;
        hat.startNewSubPath (P (0.22f, 0.70f));
        hat.lineTo (P (0.22f, 0.54f));
        hat.cubicTo (P (-0.02f, 0.52f), P (0.00f, 0.16f), P (0.27f, 0.24f));
        hat.cubicTo (P (0.30f, -0.02f), P (0.70f, -0.02f), P (0.73f, 0.24f));
        hat.cubicTo (P (1.00f, 0.16f), P (1.02f, 0.52f), P (0.78f, 0.54f));
        hat.lineTo (P (0.78f, 0.70f));
        hat.closeSubPath();
        juce::Path band;
        band.addRoundedRectangle (juce::Rectangle<float> (P (0.20f, 0.68f), P (0.80f, 0.94f)), h * 0.05f);
        g.setColour (Ink::cream);
        g.fillPath (hat); g.fillPath (band);
        sketch (g, hat, seed, juce::jmax (1.2f, w * 0.06f));
        sketch (g, band, seed + 3, juce::jmax (1.2f, w * 0.06f));
        juce::Path fold;
        fold.startNewSubPath (P (0.42f, 0.40f)); fold.quadraticTo (P (0.45f, 0.55f), P (0.42f, 0.66f));
        fold.startNewSubPath (P (0.60f, 0.38f)); fold.quadraticTo (P (0.57f, 0.52f), P (0.60f, 0.66f));
        g.setColour (Ink::ink.withAlpha (0.5f));
        g.strokePath (fold, juce::PathStrokeType (juce::jmax (0.8f, w * 0.03f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    //==========================================================================
    // Icone disegnate
    void drawIcon (juce::Graphics& g, const juce::String& icon, juce::Rectangle<float> r, juce::Colour c)
    {
        const auto stroke = juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path p;
        g.setColour (c);
        if (icon == "rec")
        {
            const auto e = r.reduced (r.getWidth() * 0.14f);
            g.setColour (Ink::tomato);
            g.fillEllipse (e);
            g.setColour (Ink::ink);
            g.drawEllipse (e, 1.5f);
        }
        else if (icon == "loop")
        {
            const auto ctr = r.getCentre();
            const float rad = r.getWidth() * 0.4f;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.6f, juce::MathConstants<float>::twoPi - 0.2f, true);
            g.strokePath (wobble (p, 0.4f, 3, false, 3.0f), stroke);
            const auto tip = ctr.getPointOnCircumference (rad, 0.6f);
            juce::Path head;
            head.addTriangle (tip.x - 3.0f, tip.y - 3.5f, tip.x + 3.5f, tip.y - 0.5f, tip.x - 1.5f, tip.y + 3.0f);
            g.fillPath (head);
        }
        else if (icon == "reset")
        {
            const auto ctr = r.getCentre();
            const float rad = r.getWidth() * 0.4f;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, -0.4f, -0.4f - 4.9f, true);
            g.strokePath (wobble (p, 0.4f, 9, false, 3.0f), stroke);
            const auto tip = ctr.getPointOnCircumference (rad, -0.4f);
            juce::Path head;
            head.addTriangle (tip.x - 0.5f, tip.y - 3.5f, tip.x + 4.0f, tip.y + 0.5f, tip.x - 3.0f, tip.y + 2.5f);
            g.fillPath (head);
        }
        else if (icon == "save")
        {
            p.startNewSubPath (r.getCentreX(), r.getY());
            p.lineTo (r.getCentreX(), r.getBottom() - 3.5f);
            p.startNewSubPath (r.getCentreX() - 4.0f, r.getBottom() - 7.5f);
            p.lineTo (r.getCentreX(), r.getBottom() - 3.5f);
            p.lineTo (r.getCentreX() + 4.0f, r.getBottom() - 7.5f);
            p.startNewSubPath (r.getX(), r.getBottom());
            p.lineTo (r.getRight(), r.getBottom());
            g.strokePath (p, stroke);
        }
        else if (icon == "chevronLeft" || icon == "chevronRight")
        {
            const bool left = icon == "chevronLeft";
            p.startNewSubPath (left ? r.getRight() - 3.0f : r.getX() + 3.0f, r.getY() + 1.0f);
            p.lineTo (left ? r.getX() + 3.0f : r.getRight() - 3.0f, r.getCentreY());
            p.lineTo (left ? r.getRight() - 3.0f : r.getX() + 3.0f, r.getBottom() - 1.0f);
            g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (icon == "chevron" || icon == "chevronDown")
        {
            const bool down = icon == "chevronDown";
            p.startNewSubPath (r.getX() + 2.0f, down ? r.getY() + 4.0f : r.getBottom() - 4.0f);
            p.lineTo (r.getCentreX(), down ? r.getBottom() - 4.0f : r.getY() + 4.0f);
            p.lineTo (r.getRight() - 2.0f, down ? r.getY() + 4.0f : r.getBottom() - 4.0f);
            g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (icon == "colander")          // FILTRO: scolapasta
        {
            juce::Path bowl;
            bowl.startNewSubPath (r.getX() + 1.0f, r.getY() + r.getHeight() * 0.35f);
            bowl.lineTo (r.getRight() - 1.0f, r.getY() + r.getHeight() * 0.35f);
            bowl.quadraticTo (r.getRight() - 2.0f, r.getBottom() - 1.0f, r.getCentreX(), r.getBottom() - 1.0f);
            bowl.quadraticTo (r.getX() + 2.0f, r.getBottom() - 1.0f, r.getX() + 1.0f, r.getY() + r.getHeight() * 0.35f);
            g.strokePath (bowl, stroke);
            for (int i = 0; i < 3; ++i)
                g.fillEllipse (r.getX() + r.getWidth() * (0.3f + 0.2f * (float) i) - 1.0f, r.getY() + r.getHeight() * 0.62f - 1.0f, 2.0f, 2.0f);
            juce::Path handles;
            handles.startNewSubPath (r.getX() - 2.0f, r.getY() + r.getHeight() * 0.3f); handles.lineTo (r.getX() + 1.0f, r.getY() + r.getHeight() * 0.35f);
            handles.startNewSubPath (r.getRight() + 2.0f, r.getY() + r.getHeight() * 0.3f); handles.lineTo (r.getRight() - 1.0f, r.getY() + r.getHeight() * 0.35f);
            g.strokePath (handles, stroke);
        }
        else if (icon == "knife")             // STUTTER: coltello che trita
        {
            juce::Path blade;
            blade.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.55f);
            blade.lineTo (r.getX() + r.getWidth() * 0.62f, r.getY() + r.getHeight() * 0.55f);
            blade.lineTo (r.getX() + r.getWidth() * 0.62f, r.getY() + r.getHeight() * 0.18f);
            blade.quadraticTo (r.getX() + r.getWidth() * 0.15f, r.getY() + r.getHeight() * 0.18f, r.getX(), r.getY() + r.getHeight() * 0.55f);
            g.strokePath (blade, stroke);
            g.fillRoundedRectangle (r.getX() + r.getWidth() * 0.62f, r.getY() + r.getHeight() * 0.3f, r.getWidth() * 0.38f, r.getHeight() * 0.18f, 1.5f);
            for (int i = 0; i < 3; ++i)
                g.fillRect (r.getX() + r.getWidth() * (0.05f + 0.22f * (float) i), r.getBottom() - 2.0f, r.getWidth() * 0.14f, 1.6f);
        }
        else if (icon == "phone")             // PHONE: cornetta
        {
            juce::Path h;
            h.startNewSubPath (r.getX() + 2.0f, r.getY() + r.getHeight() * 0.35f);
            h.quadraticTo (r.getCentreX(), r.getY() + 1.0f, r.getRight() - 2.0f, r.getY() + r.getHeight() * 0.35f);
            g.strokePath (h, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillRoundedRectangle (r.getX() + r.getWidth() * 0.2f, r.getY() + r.getHeight() * 0.52f, r.getWidth() * 0.6f, r.getHeight() * 0.42f, 2.5f);
        }
        else if (icon == "checkOff" || icon == "checkOn")
        {
            const auto box = r.reduced (0.5f);
            juce::Path b; b.addRoundedRectangle (box, 2.5f);
            g.setColour (icon == "checkOn" ? Ink::tomato : Ink::cream);
            g.fillPath (wobble (b, 0.4f, 21, true, 3.0f));
            sketch (g, b, 21, 1.6f);
            if (icon == "checkOn")
            {
                juce::Path tick;
                tick.startNewSubPath (box.getX() + box.getWidth() * 0.22f, box.getCentreY());
                tick.lineTo (box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.22f);
                tick.lineTo (box.getRight() - box.getWidth() * 0.18f, box.getY() + box.getHeight() * 0.2f);
                g.setColour (Ink::cream);
                g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
    }

    //==========================================================================
    // La pizza (disegnata una volta con lo spicchio mancante in alto: e l'indicatore)
    void drawPizza (juce::Graphics& g, juce::Point<float> c, float R)
    {
        using namespace juce;
        const float gap = 0.27f;                                    // meta dello spicchio mancante (rad)
        Path pie;
        pie.addPieSegment (c.x - R, c.y - R, R * 2, R * 2, gap, MathConstants<float>::twoPi - gap, 0.0f);
        const auto pieW = wobble (pie, 0.9f, 31, true, 4.0f);

        // ombra sul piatto
        g.setColour (Ink::ink.withAlpha (0.3f));
        g.fillPath (pieW, AffineTransform::translation (3.0f, 5.0f));

        {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (pieW);

        // cornicione
        g.setGradientFill (ColourGradient (Ink::crust.brighter (0.15f), c.x - R * 0.5f, c.y - R * 0.6f,
                                           Ink::crust.darker (0.15f), c.x + R * 0.6f, c.y + R * 0.7f, false));
        g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);
        Random rng (7);
        g.setColour (Colour (0xffb9772f).withAlpha (0.55f));
        for (int i = 0; i < 70; ++i)
        {
            const float a = rng.nextFloat() * MathConstants<float>::twoPi, rr = R * (0.86f + 0.11f * rng.nextFloat());
            const float s = 1.0f + 2.2f * rng.nextFloat();
            g.fillEllipse (c.x + std::sin (a) * rr - s * 0.5f, c.y - std::cos (a) * rr - s * 0.5f, s, s * 0.7f);
        }

        // salsa
        const float Rs = R * 0.82f;
        Path sauce; sauce.addEllipse (c.x - Rs, c.y - Rs, Rs * 2, Rs * 2);
        const auto sauceW = wobble (sauce, 2.4f, 44, true, 6.0f);
        g.setColour (Ink::tomato);
        g.fillPath (sauceW);
        g.setColour (Colour (0xffb83620).withAlpha (0.5f));
        for (int i = 0; i < 40; ++i)
        {
            const float a = rng.nextFloat() * MathConstants<float>::twoPi, rr = Rs * std::sqrt (rng.nextFloat());
            g.fillEllipse (c.x + std::sin (a) * rr, c.y - std::cos (a) * rr, 2.0f, 1.4f);
        }
        sketch (g, sauce, 44, 1.2f, Ink::ink.withAlpha (0.55f));

        // mozzarella a chiazze
        for (int i = 0; i < 9; ++i)
        {
            const float a = (float) i / 9.0f * MathConstants<float>::twoPi + rng.nextFloat() * 0.5f;
            const float rr = Rs * (i % 3 == 0 ? 0.2f : 0.55f + 0.18f * rng.nextFloat());
            const float s = R * (0.17f + 0.08f * rng.nextFloat());
            Path blob; blob.addEllipse (c.x + std::sin (a) * rr - s, c.y - std::cos (a) * rr - s * 0.8f, s * 2, s * 1.6f);
            const auto bw = wobble (blob, 1.8f, 60 + i, true, 4.0f);
            g.setColour (Ink::cheese);
            g.fillPath (bw);
            g.setColour (Ink::ink.withAlpha (0.45f));
            g.strokePath (bw, PathStrokeType (1.0f));
        }

        // salame piccante
        for (int i = 0; i < 6; ++i)
        {
            const float a = ((float) i + 0.5f) / 6.0f * MathConstants<float>::twoPi;
            const float rr = Rs * (i % 2 ? 0.62f : 0.36f);
            const float s = R * 0.12f;
            const auto pc = Point<float> (c.x + std::sin (a) * rr, c.y - std::cos (a) * rr);
            Path pep; pep.addEllipse (pc.x - s, pc.y - s, s * 2, s * 2);
            g.setColour (Colour (0xffa92c1c));
            g.fillPath (pep);
            g.setColour (Colour (0xff7d1f13).withAlpha (0.6f));
            for (int k = 0; k < 4; ++k)
                g.fillEllipse (pc.x + (rng.nextFloat() - 0.5f) * s, pc.y + (rng.nextFloat() - 0.5f) * s, 2.0f, 2.0f);
            g.setColour (Ink::ink.withAlpha (0.7f));
            g.strokePath (wobble (pep, 0.5f, 80 + i, true, 3.0f), PathStrokeType (1.2f));
        }

        // basilico
        for (int i = 0; i < 4; ++i)
        {
            const float a = (float) i / 4.0f * MathConstants<float>::twoPi + 0.9f;
            const float rr = Rs * 0.5f;
            const auto lc = Point<float> (c.x + std::sin (a) * rr, c.y - std::cos (a) * rr);
            Path leaf;
            const float L = R * 0.2f;
            leaf.startNewSubPath (-L, 0.0f);
            leaf.quadraticTo (0.0f, -L * 0.75f, L, 0.0f);
            leaf.quadraticTo (0.0f, L * 0.75f, -L, 0.0f);
            leaf.closeSubPath();
            const auto tr = AffineTransform::rotation (a * 1.7f).translated (lc);
            g.setColour (Ink::basil);
            g.fillPath (leaf, tr);
            g.setColour (Ink::ink.withAlpha (0.7f));
            g.strokePath (leaf, PathStrokeType (1.1f), tr);
            Path vein; vein.startNewSubPath (-L * 0.8f, 0.0f); vein.lineTo (L * 0.8f, 0.0f);
            g.setColour (Colour (0xff8fc27a));
            g.strokePath (vein, PathStrokeType (0.8f), tr);
        }

        // tagli degli spicchi
        g.setColour (Ink::ink.withAlpha (0.35f));
        for (int k = 1; k < 6; ++k)
        {
            const float a = gap + (MathConstants<float>::twoPi - 2.0f * gap) * (float) k / 6.0f;
            Path cut; cut.startNewSubPath (c); cut.lineTo (c.x + std::sin (a) * R, c.y - std::cos (a) * R);
            g.strokePath (wobble (cut, 0.4f, 90 + k, false, 4.0f), PathStrokeType (1.2f));
        }

        }
        sketch (g, pie, 31, 2.4f);
    }
}

//==============================================================================
RiserLookAndFeel::RiserLookAndFeel()
{
    setColour (juce::TooltipWindow::textColourId, Ink::ink);
}

void RiserLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    sketchBox (g, juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (2.0f), 5.0f, Ink::cream, 3, 1.6f, 0.0f);
    g.setFont (handFont (15.0f));
    handText (g, text, juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (6.0f, 2.0f), juce::Justification::centred);
}

void RiserLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    using namespace juce;
    const auto bounds = Rectangle<int> (x, y, w, h).toFloat();
    const auto c = bounds.getCentre();
    const float R = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 18.0f;

    // tacche intorno alla pizza: piene fino al valore
    const int ticks = 21;
    for (int i = 0; i < ticks; ++i)
    {
        const float t = (float) i / (float) (ticks - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const auto p = c.getPointOnCircumference (R + 12.0f, a);
        const bool lit = t <= pos + 0.001f;
        const float r = lit ? 3.4f : 2.6f;
        if (lit)
        {
            g.setColour (Ink::butter);
            g.fillEllipse (p.x - r, p.y - r, r * 2, r * 2);
            g.setColour (Ink::ink);
            g.drawEllipse (p.x - r, p.y - r, r * 2, r * 2, 1.0f);
        }
        else
        {
            g.setColour (Ink::cream.withAlpha (0.45f));
            g.drawEllipse (p.x - r, p.y - r, r * 2, r * 2, 1.2f);
        }
    }

    // pizza (in cache), ruotata: lo spicchio mancante indica il valore
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (pizzaCache.isNull() || std::abs (pizzaCacheRadius - R) > 0.5f || std::abs (pizzaCacheScale - scale) > 0.01f)
    {
        pizzaCacheRadius = R; pizzaCacheScale = scale;
        const float side = R * 2.0f + 20.0f;
        pizzaCache = Image (Image::ARGB, jmax (1, roundToInt (side * scale)), jmax (1, roundToInt (side * scale)), true);
        Graphics pg (pizzaCache);
        pg.addTransform (AffineTransform::scale (scale));
        drawPizza (pg, { side * 0.5f, side * 0.5f }, R);
    }
    const float side = R * 2.0f + 20.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    g.setOpacity (1.0f);              // drawImage usa l'opacita del colore corrente
    g.drawImageTransformed (pizzaCache,
        AffineTransform::scale (1.0f / pizzaCacheScale)
            .translated (-side * 0.5f, -side * 0.5f)
            .rotated (angle)
            .translated (c.x, c.y));

    // al passaggio del mouse un filo di farina intorno
    if (s.isMouseOverOrDragging())
    {
        Path ring; ring.addEllipse (c.x - R - 4.0f, c.y - R - 4.0f, (R + 4.0f) * 2, (R + 4.0f) * 2);
        g.setColour (Ink::cream.withAlpha (0.35f));
        g.strokePath (wobble (ring, 0.8f, 5), PathStrokeType (1.5f));
    }
}

void RiserLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    const auto style = b.getProperties()["style"].toString();
    const bool forced = (bool) b.getProperties()["forceOn"];
    const bool on = b.getToggleState() || forced;
    auto r = b.getLocalBounds().toFloat().reduced (3.0f, 3.0f);
    const int seed = (int) b.getButtonText().hashCode() & 63;

    if (style == "row" || style == "check")
        return;                                     // solo testo e spia

    if (down) r.translate (1.0f, 1.0f);
    juce::Colour fill = on ? (style == "loop" ? Ink::basil : Ink::tomato) : Ink::cream;
    if (highlighted && ! on) fill = fill.darker (0.06f);
    if (! b.isEnabled() && ! forced) fill = fill.withMultipliedAlpha (0.6f);
    sketchBox (g, r, 6.0f, fill, seed + 1, 2.0f, down ? 1.0f : 3.0f);
}

void RiserLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool down)
{
    const auto style = b.getProperties()["style"].toString();
    const bool forced = (bool) b.getProperties()["forceOn"];
    const bool on = b.getToggleState() || forced;
    auto r = b.getLocalBounds().toFloat();
    if (down) r.translate (1.0f, 1.0f);
    const float alpha = (b.isEnabled() || forced) ? 1.0f : 0.45f;

    if (style == "row")
    {
        // spia rotonda (pomodoro quando e acceso), scritta, icona a destra
        const float d = 13.0f;
        auto led = juce::Rectangle<float> (r.getX() + 4.0f, r.getCentreY() - d * 0.5f, d, d);
        juce::Path lp; lp.addEllipse (led);
        g.setColour (on ? Ink::tomato : Ink::cream);
        g.fillPath (lp);
        if (on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillEllipse (led.getX() + 3.0f, led.getY() + 2.5f, 3.5f, 2.5f);
        }
        sketch (g, lp, 7, 1.5f);

        g.setFont (handFont (19.0f, 0.06f));
        auto tr = r.withTrimmedLeft (d + 12.0f);
        handText (g, b.getButtonText(), tr, juce::Justification::centredLeft, Ink::ink.withMultipliedAlpha (highlighted ? 1.0f : 0.88f));

        const auto icon = b.getProperties()["icon"].toString();
        if (icon.isNotEmpty())
            drawIcon (g, icon, { r.getRight() - 24.0f, r.getCentreY() - 8.0f, 18.0f, 16.0f }, Ink::ink.withAlpha (0.8f));
        return;
    }

    if (style == "check")
    {
        const float d = 15.0f;
        drawIcon (g, on ? "checkOn" : "checkOff", { r.getX() + 4.0f, r.getCentreY() - d * 0.5f, d, d }, Ink::ink);
        g.setFont (handFont (17.0f, 0.06f));
        handText (g, b.getButtonText(), r.withTrimmedLeft (d + 12.0f), juce::Justification::centredLeft);
        return;
    }

    const juce::Colour col = (on ? Ink::cream : Ink::ink).withMultipliedAlpha (alpha);
    const auto icon = b.getProperties()["icon"].toString();
    if (style == "icon")
    {
        drawIcon (g, icon, r.withSizeKeepingCentre (14.0f, 12.0f), Ink::ink);
        return;
    }

    g.setFont (handFont ((float) b.getProperties().getWithDefault ("size", 18.0f), 0.06f));
    const auto text = b.getButtonText();
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text);
    const float iw = icon.isNotEmpty() ? 13.0f : 0.0f, gap = icon.isNotEmpty() ? 7.0f : 0.0f;
    float xx = r.getCentreX() - (iw + gap + tw) * 0.5f;
    if (icon.isNotEmpty())
    {
        drawIcon (g, icon, { xx, r.getCentreY() - iw * 0.5f - 0.5f, iw, iw }, col);
        xx += iw + gap;
    }
    handText (g, text, { xx, r.getY(), tw + 4.0f, r.getHeight() - 1.0f }, juce::Justification::centredLeft, col);
}

void RiserLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& s)
{
    // QUANTITA: un mattarello che scorre sulla pasta
    const float cy = (float) y + (float) h * 0.5f;
    const float x0 = (float) x + 8.0f, x1 = (float) (x + w) - 8.0f;
    juce::Path track; track.addRoundedRectangle (x0, cy - 4.0f, x1 - x0, 8.0f, 4.0f);
    g.setColour (Ink::cream);
    g.fillPath (track);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<float> (x0, cy - 6.0f, juce::jmax (0.0f, pos - x0), 12.0f).toNearestInt());
        g.setColour (Ink::tomato);
        g.fillPath (track);
    }
    sketch (g, track, 13, 1.5f);

    const float px = juce::jlimit (x0, x1, pos);
    juce::Path pin; pin.addRoundedRectangle (px - 10.0f, cy - 6.5f, 20.0f, 13.0f, 6.0f);
    g.setColour (Ink::ink.withAlpha (0.25f));
    g.fillPath (pin, juce::AffineTransform::translation (1.5f, 2.0f));
    g.setColour (s.isMouseOverOrDragging() ? Ink::wood.brighter (0.1f) : Ink::wood);
    g.fillPath (pin);
    sketch (g, pin, 14, 1.5f);
    g.setColour (Ink::ink.withAlpha (0.5f));
    g.drawLine (px - 4.0f, cy - 4.0f, px - 4.0f, cy + 4.0f, 1.0f);
    g.drawLine (px + 4.0f, cy - 4.0f, px + 4.0f, cy + 4.0f, 1.0f);
}

//==============================================================================
StutterPopup::StutterPopup (VocalRiserAudioProcessor& p)
    : timing (*p.apvts.getParameter ("stutRate"), "TIMING", { "1/4", "1/8", "1/16" })
{
    addAndMakeVisible (timing);

    depth.setSliderStyle (juce::Slider::LinearHorizontal);
    depth.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    depth.setDoubleClickReturnValue (true, 0.5);
    depth.onValueChange = [this] { repaint(); };
    addAndMakeVisible (depth);
    depthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "stutDepth", depth);

    pingPong.setClickingTogglesState (true);
    pingPong.getProperties().set ("style", "check");
    addAndMakeVisible (pingPong);
    ppAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "pingPong", pingPong);

    setSize (width, height);
}

void StutterPopup::paint (juce::Graphics& g)
{
    // bigliettino con la punta in basso verso STUTTER
    const auto box = getLocalBounds().toFloat().reduced (3.0f).withTrimmedBottom ((float) arrowH).withTrimmedRight (2.0f);
    juce::Path p;
    p.addRoundedRectangle (box, 7.0f);
    juce::Path tip;
    tip.addTriangle (arrowX - 9.0f, box.getBottom() - 1.0f, arrowX + 9.0f, box.getBottom() - 1.0f, arrowX, box.getBottom() + (float) arrowH);
    juce::Path all;
    all.addPath (p);
    const auto w = wobble (p, 0.7f, 71);
    g.setColour (Ink::ink.withAlpha (0.25f));
    g.fillPath (w, juce::AffineTransform::translation (2.0f, 3.0f));
    g.setColour (Ink::cream);
    g.fillPath (w);
    g.fillPath (tip);
    sketch (g, p, 71, 2.0f);
    juce::Path tipLines;
    tipLines.startNewSubPath (arrowX - 9.0f, box.getBottom());
    tipLines.lineTo (arrowX, box.getBottom() + (float) arrowH - 1.0f);
    tipLines.lineTo (arrowX + 9.0f, box.getBottom());
    g.setColour (Ink::cream);
    g.fillRect (arrowX - 8.0f, box.getBottom() - 3.0f, 16.0f, 4.0f);
    sketch (g, tipLines, 72, 2.0f, Ink::ink, false);

    g.setFont (handFont (16.0f));
    const auto d = depth.getBounds().toFloat();
    handText (g, juce::CharPointer_UTF8 ("QUANTIT\xc3\x80"), { d.getX() + 4.0f, d.getY() - 17.0f, 90.0f, 16.0f }, juce::Justification::centredLeft, Ink::ink.withAlpha (0.75f));
    handText (g, juce::String (juce::roundToInt (depth.getValue() * 100.0)) + " %",
              { d.getRight() - 54.0f, d.getY() - 17.0f, 50.0f, 16.0f }, juce::Justification::centredRight);
}

void StutterPopup::resized()
{
    auto r = getLocalBounds().withTrimmedBottom (arrowH).reduced (10, 7);
    timing.setBounds (r.removeFromTop (40).withSizeKeepingCentre (140, 40));
    r.removeFromTop (17);
    depth.setBounds (r.removeFromTop (20));
    pingPong.setBounds (r.withSizeKeepingCentre (126, 28));
}

//==============================================================================
// Prepara (una volta per ogni nuovo riser o dimensione) le immagini della forma d'onda
void RiserWaveView::rebuildLayers (float scale)
{
    layerScale = scale;
    layerGen   = proc.getRenderGeneration();
    layerSize  = getLocalBounds().getBottomRight();
    peaks      = proc.getPeaks();

    const int pw = juce::jmax (1, juce::roundToInt ((float) getWidth()  * scale));
    const int ph = juce::jmax (1, juce::roundToInt ((float) getHeight() * scale));
    for (auto* img : { &normalImg, &dimImg, &litImg, &hotImg })
        *img = juce::Image (juce::Image::ARGB, pw, ph, true);

    if (peaks == nullptr || peaks->empty()) return;

    auto inner = getLocalBounds().toFloat().reduced (6.0f, 10.0f);
    const float mid  = inner.getCentreY();
    const int   bars = juce::jmax (24, (int) (inner.getWidth() / 4.0f));
    const float bw   = inner.getWidth() / (float) bars;
    const float half = inner.getHeight() * 0.5f;
    const int   nb   = (int) peaks->size();

    // barrette "a pennarello": leggermente storte, come disegnate a mano
    juce::Path w;
    juce::Random rng (3);
    for (int i = 0; i < bars; ++i)
    {
        const int k0 = i * nb / bars, k1 = juce::jmax (k0 + 1, (i + 1) * nb / bars);
        float m = 0.0f;
        for (int k = k0; k < k1; ++k) m = juce::jmax (m, (*peaks)[(size_t) k]);
        const float hh = juce::jmax (1.2f, m * half);
        const float xx = inner.getX() + ((float) i + 0.5f) * bw + (rng.nextFloat() - 0.5f) * 0.6f;
        const float lean = (rng.nextFloat() - 0.5f) * 1.2f;
        w.startNewSubPath (xx - lean, mid - hh);
        w.lineTo (xx + lean, mid + hh);
    }
    const juce::PathStrokeType st (bw * 0.55f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    auto paintLayer = [&] (juce::Image& img, juce::Colour c)
    {
        juce::Graphics lg (img);
        lg.addTransform (juce::AffineTransform::scale (scale));
        lg.setColour (c);
        lg.strokePath (w, st);
    };
    paintLayer (normalImg, Ink::ink.withAlpha (0.85f));
    paintLayer (dimImg,    Ink::ink.withAlpha (0.25f));
    paintLayer (litImg,    Ink::tomato);
    paintLayer (hotImg,    Ink::tomato.darker (0.35f));
}

void RiserWaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto inner = r.reduced (6.0f, 10.0f);
    const float mid = inner.getCentreY();

    if (! proc.hasRender())
    {
        // tagliere vuoto: una linea tratteggiata a matita
        g.setColour (Ink::ink.withAlpha (0.3f));
        for (float xx = inner.getX(); xx < inner.getRight(); xx += 7.0f)
            g.fillRoundedRectangle (xx, mid - 0.8f, 4.0f, 1.6f, 0.8f);
        return;
    }

    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (normalImg.isNull() || layerGen != proc.getRenderGeneration() || std::abs (layerScale - scale) > 0.01f
         || layerSize != getLocalBounds().getBottomRight())
        rebuildLayers (scale);

    g.setOpacity (1.0f);
    if (! proc.isPreviewPlaying())
    {
        g.drawImage (normalImg, r);
        return;
    }

    const float px = inner.getX() + inner.getWidth() * proc.previewProgress.load();
    g.setOpacity (1.0f);
    g.drawImage (dimImg, r);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<float> (0.0f, 0.0f, px, r.getHeight()).toNearestInt());
        g.drawImage (litImg, r);
    }
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<float> (px - 14.0f, 0.0f, 14.0f, r.getHeight()).toNearestInt());
        g.drawImage (hotImg, r);
    }
    // testina: un tratto d'inchiostro con un pomodorino in cima
    juce::Path head;
    head.startNewSubPath (px, inner.getY() - 4.0f);
    head.lineTo (px + 0.5f, inner.getBottom() + 4.0f);
    g.setColour (Ink::ink);
    g.strokePath (head, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Ink::tomato);
    g.fillEllipse (px - 4.5f, inner.getY() - 9.0f, 9.0f, 9.0f);
    g.setColour (Ink::ink);
    g.drawEllipse (px - 4.5f, inner.getY() - 9.0f, 9.0f, 9.0f, 1.3f);
    g.setColour (Ink::basil);
    g.fillEllipse (px - 1.5f, inner.getY() - 11.0f, 3.0f, 3.0f);
}

void RiserWaveView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 6 || ! proc.hasRender())
        return;

    dragStarted = true;
    const auto file = proc.writeRiserFile();
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
ArrowSelector::ArrowSelector (juce::RangedAudioParameter& param, const juce::String& t, juce::StringArray v)
    : title (t), values (std::move (v)),
      attachment (param, [this] (float value) { index = juce::jlimit (0, values.size() - 1, juce::roundToInt (value)); repaint(); })
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void ArrowSelector::step (int delta)
{
    const int n = values.size();
    index = (index + delta + n) % n;
    attachment.setValueAsCompleteGesture ((float) index);
    repaint();
}

void ArrowSelector::mouseDown (const juce::MouseEvent& e)
{
    if (! isEnabled()) return;
    step (e.x < getWidth() / 2 ? -1 : 1);
}

void ArrowSelector::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (handFont (15.0f, 0.08f));
    handText (g, title, r.removeFromTop (15.0f), juce::Justification::centred, Ink::ink.withAlpha (0.7f));

    // targhetta col valore e le due frecce
    auto plate = r.reduced (2.0f, 2.0f);
    sketchBox (g, plate, 6.0f, Ink::cream, title.hashCode() & 63, 1.8f, 2.5f);
    g.setFont (handFont (19.0f, 0.06f));
    handText (g, values[index], plate.translated (0.0f, -0.5f), juce::Justification::centred);
    drawIcon (g, "chevronLeft",  { plate.getX() + 8.0f, plate.getCentreY() - 6.0f, 9.0f, 12.0f }, Ink::ink.withAlpha (0.65f));
    drawIcon (g, "chevronRight", { plate.getRight() - 17.0f, plate.getCentreY() - 6.0f, 9.0f, 12.0f }, Ink::ink.withAlpha (0.65f));
}

//==============================================================================
VocalRiserAudioProcessorEditor::VocalRiserAudioProcessorEditor (VocalRiserAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), wave (p),
      lengthSel (*p.apvts.getParameter ("length"), "DURATA", { "1/4 BAR", "1/2 BAR", "1 BAR", "2 BAR" }),
      stutPopup (p)
{
    setLookAndFeel (&lnf);
    auto& apvts = proc.apvts;

    addAndMakeVisible (wave);
    wave.setTooltip ("Trascina il riser sulla traccia");

    captureBtn.getProperties().set ("icon", "rec");
    loopBtn.getProperties().set ("icon", "loop");
    loopBtn.getProperties().set ("style", "loop");
    saveBtn.getProperties().set ("icon", "save");
    resetBtn.getProperties().set ("icon", "reset");
    for (auto* b : { &saveBtn, &resetBtn }) b->getProperties().set ("size", 16.0f);

    captureBtn.setTooltip ("Premi CATTURA, poi play: il plugin ascolta la prossima frase");
    captureBtn.onClick = [this] { proc.armCapture(); };

    loopBtn.setTooltip ("Con la DAW ferma suona il riser in loop");
    loopBtn.setClickingTogglesState (true);
    loopBtn.setToggleState (proc.getPreviewLoop(), juce::dontSendNotification);
    loopBtn.onClick = [this] { proc.setPreviewLoop (loopBtn.getToggleState()); };

    saveBtn.setTooltip ("Salva il WAV e apri la cartella");
    saveBtn.onClick = [this]
    {
        const auto f = proc.writeRiserFile();
        if (f.existsAsFile()) f.revealToUser();
    };

    resetBtn.setTooltip ("Cancella la frase catturata (i controlli restano come sono)");
    resetBtn.onClick = [this]
    {
        proc.clearCapture();      // via l'audio catturato; i controlli restano come sono
        wave.repaint();
    };

    for (auto* b : { &captureBtn, &loopBtn, &saveBtn, &resetBtn })
        addAndMakeVisible (*b);

    // La pizza (intensita)
    amountKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amountKnob.setDoubleClickReturnValue (true, 0.6);
    amountKnob.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (amountKnob);
    amountAtt = std::make_unique<SliderAtt> (apvts, "amount", amountKnob);
    amountKnob.onValueChange = [this] { repaint (intensityArea); };

    addAndMakeVisible (lengthSel);

    stereoBtn.setClickingTogglesState (true);
    stereoBtn.setTooltip ("Allarga una voce mono in stereo");
    addAndMakeVisible (stereoBtn);
    buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, "stereoOn", stereoBtn));

    // Effetti: righe con la spia
    const char* fxNames[] = { "FILTRO", "STUTTER", "PHONE" };
    const char* fxIds[]   = { "filterOn", "stutOn", "phoneOn" };
    const char* fxIcons[] = { "colander", "", "phone" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = fxBtns[(size_t) i];
        b.setButtonText (fxNames[i]);
        b.getProperties().set ("style", "row");
        b.getProperties().set ("icon", fxIcons[i]);
        b.setClickingTogglesState (true);
        b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (b);
        buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, fxIds[i], b));
    }
    // accendendo lo STUTTER si apre anche il suo menu
    fxBtns[1].onClick = [this] { showStutterMenu (fxBtns[1].getToggleState()); };

    // Freccia accanto a STUTTER: apre il menu con TIMING, QUANTITA e PING PONG
    stutMenuBtn.getProperties().set ("icon", "chevron");
    stutMenuBtn.getProperties().set ("style", "icon");
    stutMenuBtn.setTooltip ("Opzioni dello stutter");
    stutMenuBtn.onClick = [this] { showStutterMenu (! stutPopup.isVisible()); };
    addAndMakeVisible (stutMenuBtn);
    addChildComponent (stutPopup);
    addMouseListener (this, true);          // per chiudere il menu con un clic fuori

    setSize (720, 408);
    timerCallback();
    startTimerHz (30);
}

VocalRiserAudioProcessorEditor::~VocalRiserAudioProcessorEditor()
{
    removeMouseListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void VocalRiserAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Sfondo statico (pannelli, tagliere, scritte): disegnato una volta e riusato
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (bgCache.isNull() || std::abs (bgScale - scale) > 0.01f
         || bgCache.getWidth() != juce::roundToInt ((float) getWidth() * scale))
    {
        bgScale = scale;
        bgCache = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) getWidth() * scale)),
                               juce::jmax (1, juce::roundToInt ((float) getHeight() * scale)), true);
        juce::Graphics bg (bgCache);
        bg.addTransform (juce::AffineTransform::scale (scale));
        paintStatic (bg);
    }
    g.setOpacity (1.0f);
    g.drawImage (bgCache, getLocalBounds().toFloat());

    // percentuale sotto la pizza
    g.setFont (handFont (26.0f, 0.04f));
    handText (g, juce::String (juce::roundToInt (amountKnob.getValue() * 100.0)) + " %",
              intensityArea.toFloat(), juce::Justification::centred, Ink::cream);

    // titolo di IMMAGINE (cambia se la voce e gia stereo)
    g.setFont (handFont (15.0f, 0.08f));
    handText (g, proc.getSourceStereo() == 1 ? "VOCE STEREO" : "IMMAGINE", imageTitleArea.toFloat(),
              juce::Justification::centred, Ink::ink.withAlpha (0.7f));
}

void VocalRiserAudioProcessorEditor::paintStatic (juce::Graphics& g)
{
    using namespace juce;
    const auto b = getLocalBounds().toFloat();

    // carta con qualche puntino, come un foglio di ricette
    g.fillAll (Ink::paper);
    Random rng (11);
    for (int i = 0; i < 420; ++i)
    {
        g.setColour (Ink::ink.withAlpha (0.04f + 0.05f * rng.nextFloat()));
        const float s = 0.8f + 1.4f * rng.nextFloat();
        g.fillEllipse (rng.nextFloat() * b.getWidth(), rng.nextFloat() * b.getHeight(), s, s);
    }
    {
        Path frame; frame.addRoundedRectangle (b.reduced (5.0f), 10.0f);
        sketch (g, frame, 1, 2.6f);
    }

    // intestazione: cappello e titolo
    chefHat (g, { 22.0f, 12.0f, 40.0f, 40.0f }, 4);
    g.setFont (titleFont (38.0f));
    handText (g, "Chef Riser", { 68.0f, 10.0f, 220.0f, 44.0f }, Justification::centredLeft);
    g.setFont (handFont (17.0f));
    handText (g, "riser vocali fatti in casa", { 236.0f, 24.0f, 220.0f, 24.0f }, Justification::centredLeft, Ink::ink.withAlpha (0.6f));

    // pannelli
    auto panel = [&] (Rectangle<int> r, Colour c, int seed)
    {
        const auto rf = r.toFloat();
        sketchBox (g, rf, 9.0f, c, seed, 2.6f, 4.0f);
        screw (g, { rf.getX() + 10.0f, rf.getY() + 10.0f }, seed);
        screw (g, { rf.getRight() - 10.0f, rf.getY() + 10.0f }, seed + 1);
        screw (g, { rf.getX() + 10.0f, rf.getBottom() - 10.0f }, seed + 2);
        screw (g, { rf.getRight() - 10.0f, rf.getBottom() - 10.0f }, seed + 3);
    };
    auto plaque = [&] (Rectangle<int> panelR, const String& text, int seed)
    {
        auto pr = Rectangle<float> ((float) panelR.getCentreX() - 70.0f, (float) panelR.getY() + 12.0f, 140.0f, 30.0f);
        sketchBox (g, pr, 6.0f, Ink::cream, seed, 2.0f, 2.5f);
        g.setFont (handFont (21.0f, 0.06f));
        handText (g, text, pr.translated (0.0f, -1.0f), Justification::centred);
    };

    panel (leftPanel, Ink::mint, 20);
    panel (midPanel, Ink::navy, 30);
    panel (rightPanel, Ink::butter, 40);

    // farina sul pannello del forno
    {
        Random fl (5);
        const auto mp = midPanel.toFloat().reduced (14.0f);
        for (int i = 0; i < 90; ++i)
        {
            const float px = mp.getX() + fl.nextFloat() * mp.getWidth(), py = mp.getY() + fl.nextFloat() * mp.getHeight();
            const float s = 1.0f + 2.6f * fl.nextFloat();
            g.setColour (Ink::cream.withAlpha (0.2f + 0.45f * fl.nextFloat()));
            Path q; q.addRectangle (-s * 0.5f, -s * 0.5f, s, s);
            g.fillPath (q, AffineTransform::rotation (fl.nextFloat() * 3.0f).translated (px, py));
        }
    }

    plaque (leftPanel, "LA FRASE", 21);
    plaque (midPanel, juce::CharPointer_UTF8 ("INTENSIT\xc3\x80"), 31);
    plaque (rightPanel, "LA RICETTA", 41);

    // tagliere sotto la forma d'onda
    {
        const auto br = boardArea.toFloat();
        Path board;
        board.addRoundedRectangle (br, 14.0f);
        // manico a destra
        Path handle;
        handle.addRoundedRectangle (br.getRight() - 6.0f, br.getCentreY() - 13.0f, 24.0f, 26.0f, 9.0f);
        g.setColour (Ink::ink.withAlpha (0.22f));
        g.fillPath (wobble (board, 0.8f, 50), AffineTransform::translation (3.0f, 4.0f));
        g.fillPath (wobble (handle, 0.6f, 51), AffineTransform::translation (3.0f, 4.0f));
        g.setColour (Ink::wood);
        g.fillPath (wobble (handle, 0.6f, 51));
        sketch (g, handle, 51, 2.0f);
        g.setGradientFill (ColourGradient (Ink::wood.brighter (0.12f), br.getX(), br.getY(), Ink::wood, br.getX(), br.getBottom(), false));
        g.fillPath (wobble (board, 0.8f, 50));
        // venature del legno
        Random wv (9);
        for (int i = 0; i < 7; ++i)
        {
            const float yy = br.getY() + 14.0f + (br.getHeight() - 28.0f) * (float) i / 6.0f + (wv.nextFloat() - 0.5f) * 6.0f;
            Path vein;
            vein.startNewSubPath (br.getX() + 14.0f + wv.nextFloat() * 20.0f, yy);
            vein.cubicTo (br.getX() + br.getWidth() * 0.35f, yy - 4.0f + wv.nextFloat() * 8.0f,
                          br.getX() + br.getWidth() * 0.65f, yy - 4.0f + wv.nextFloat() * 8.0f,
                          br.getRight() - 14.0f - wv.nextFloat() * 20.0f, yy);
            g.setColour (Ink::woodDark.withAlpha (0.35f));
            g.strokePath (vein, PathStrokeType (1.0f));
        }
        sketch (g, board, 50, 2.4f);
        // foro del manico
        g.setColour (Ink::mint);
        g.fillEllipse (br.getRight() + 5.0f, br.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.setColour (Ink::ink);
        g.drawEllipse (br.getRight() + 5.0f, br.getCentreY() - 4.0f, 8.0f, 8.0f, 1.4f);
    }

    // separatore nella ricetta, sopra gli effetti
    {
        const float yy = (float) fxBtns[0].getY() - 6.0f;
        Path sep; sep.startNewSubPath ((float) rightPanel.getX() + 18.0f, yy); sep.lineTo ((float) rightPanel.getRight() - 18.0f, yy);
        sketch (g, sep, 61, 1.6f, Ink::ink.withAlpha (0.6f), false);
    }

    // firma
    chefHat (g, { 22.0f, b.getBottom() - 34.0f, 22.0f, 22.0f }, 8);
    g.setFont (handFont (17.0f, 0.14f));
    handText (g, "CHEF CATTA", { 50.0f, b.getBottom() - 33.0f, 120.0f, 22.0f }, Justification::centredLeft);
    g.setFont (handFont (17.0f, 0.04f));
    handText (g, "dsp", { 140.0f, b.getBottom() - 33.0f, 40.0f, 22.0f }, Justification::centredLeft, Ink::ink.withAlpha (0.6f));
   #ifdef JucePlugin_VersionString
    handText (g, String ("v") + JucePlugin_VersionString, { b.getRight() - 120.0f, b.getBottom() - 33.0f, 98.0f, 22.0f },
              Justification::centredRight, Ink::ink.withAlpha (0.5f));
   #endif
}

void VocalRiserAudioProcessorEditor::resized()
{
    bgCache = {};

    // intestazione: SALVA e RESET a destra
    saveBtn.setBounds (getWidth() - 214, 16, 96, 36);
    resetBtn.setBounds (getWidth() - 112, 16, 96, 36);

    // tre pannelli
    const int top = 64, bottom = getHeight() - 44;
    leftPanel  = { 16, top, 280, bottom - top };
    midPanel   = { 306, top, 222, bottom - top };
    rightPanel = { 538, top, 166, bottom - top };

    // LA FRASE: tagliere con la forma d'onda, poi CATTURA e LOOP
    boardArea = { leftPanel.getX() + 18, leftPanel.getY() + 60, leftPanel.getWidth() - 50, 168 };
    wave.setBounds (boardArea.reduced (14, 14));
    const int by = boardArea.getBottom() + 18;
    captureBtn.setBounds (leftPanel.getX() + 16, by, 122, 42);
    loopBtn.setBounds (leftPanel.getRight() - 138, by, 122, 42);

    // INTENSITA: la pizza
    const int ks = 196;
    amountKnob.setBounds (midPanel.getCentreX() - ks / 2, midPanel.getY() + 50, ks, ks);
    intensityArea = { midPanel.getCentreX() - 60, amountKnob.getBottom() + 8, 120, 32 };

    // LA RICETTA: durata, immagine, effetti
    auto rc = rightPanel.reduced (14, 0);
    lengthSel.setBounds (rc.getX(), rightPanel.getY() + 52, rc.getWidth(), 52);
    imageTitleArea = { rc.getX(), lengthSel.getBottom() + 6, rc.getWidth(), 15 };
    stereoBtn.setBounds (rc.getX(), imageTitleArea.getBottom(), rc.getWidth(), 38);

    int fy = stereoBtn.getBottom() + 16;
    for (auto& fb : fxBtns)
    {
        fb.setBounds (rc.getX(), fy, rc.getWidth(), 30);
        fy += 32;
    }
    // la freccia del menu stutter sta a destra della riga STUTTER
    const auto sb = fxBtns[1].getBounds();
    stutMenuBtn.setBounds (sb.getRight() - 26, sb.getY() + 3, 24, 24);

    if (stutPopup.isVisible()) showStutterMenu (true);
}

void VocalRiserAudioProcessorEditor::showStutterMenu (bool show)
{
    if (show)
    {
        const auto sb = fxBtns[1].getBounds();
        const int x = juce::jlimit (8, getWidth() - StutterPopup::width - 8, sb.getCentreX() - StutterPopup::width / 2);
        stutPopup.setTopLeftPosition (x, sb.getY() - StutterPopup::height + 4);
        stutPopup.setArrowX ((float) (sb.getX() + 50 - x));
        stutPopup.toFront (false);
    }
    stutPopup.setVisible (show);
    stutMenuBtn.getProperties().set ("icon", show ? "chevronDown" : "chevron");
    stutMenuBtn.repaint();
}

void VocalRiserAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! stutPopup.isVisible()) return;
    const auto pos = e.getEventRelativeTo (this).getPosition();
    if (! stutPopup.getBounds().contains (pos) && ! stutMenuBtn.getBounds().contains (pos)
         && ! fxBtns[1].getBounds().contains (pos))
        showStutterMenu (false);
}

void VocalRiserAudioProcessorEditor::timerCallback()
{
    proc.requestRenderIfNeeded();   // il calcolo avviene su un altro thread

    const int gen = proc.getRenderGeneration();
    const bool playingPreview = proc.isPreviewPlaying();
    if (gen != lastGeneration || playingPreview || playingPreview != lastPreviewPlaying)
        wave.repaint();
    lastGeneration = gen;
    lastPreviewPlaying = playingPreview;

    saveBtn.setEnabled (proc.hasRender());
    const bool armed = proc.getCaptureState() != VocalRiserAudioProcessor::stateIdle;
    captureBtn.setToggleState (armed, juce::dontSendNotification);
    const auto capText = proc.getCaptureState() == VocalRiserAudioProcessor::stateRecording ? "ASCOLTO..." : "CATTURA";
    if (captureBtn.getButtonText() != capText) captureBtn.setButtonText (capText);

    // Voce stereo: resta stereo, il selettore si blocca su STEREO
    const int src = proc.getSourceStereo();
    if (src != lastSourceStereo)
    {
        lastSourceStereo = src;
        if (src == 1)
        {
            stereoBtn.setEnabled (false);
            stereoBtn.setButtonText ("STEREO");
            stereoBtn.getProperties().set ("forceOn", true);
        }
        else
        {
            stereoBtn.setEnabled (true);
            stereoBtn.getProperties().remove ("forceOn");
        }
        repaint (imageTitleArea);
    }
    if (src != 1)
    {
        const auto t = stereoBtn.getToggleState() ? "STEREO" : "MONO";
        if (stereoBtn.getButtonText() != t) stereoBtn.setButtonText (t);
    }
}
