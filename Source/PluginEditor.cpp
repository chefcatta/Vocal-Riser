#include "PluginEditor.h"

namespace Colours
{
    const juce::Colour bg        { 0xff101113 };
    const juce::Colour bodyTop   { 0xff26272b };
    const juce::Colour bodyBot   { 0xff18191c };
    const juce::Colour displayTop{ 0xff0c0d0f };
    const juce::Colour displayBot{ 0xff141518 };
    const juce::Colour accent    { 0xff3a7bff };   // blu
    const juce::Colour accent2   { 0xff6fc6ff };   // azzurro
    const juce::Colour text      { 0xffd9dbe0 };
    const juce::Colour dim       { 0xff6e727b };
    const juce::Colour faint     { 0xff3a3c42 };
}

namespace
{
    juce::Font labelFont (float size, float kerning = 0.14f)
    {
        return juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kerning);
    }

    void engravedText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r,
                       juce::Justification j, juce::Colour c)
    {
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawText (t, r.translated (0.0f, 1.0f), j);
        g.setColour (c);
        g.drawText (t, r, j);
    }

    void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j)
    {
        g.setColour (Colours::accent.withAlpha (0.28f));
        for (auto d : { juce::Point<float> (0, 1), juce::Point<float> (0, -1), juce::Point<float> (1, 0), juce::Point<float> (-1, 0) })
            g.drawText (t, r.translated (d.x, d.y), j);
        g.setColour (Colours::accent2);
        g.drawText (t, r, j);
    }

    void drawIcon (juce::Graphics& g, const juce::String& icon, juce::Rectangle<float> r, juce::Colour c)
    {
        g.setColour (c);
        juce::Path p;
        const auto stroke = juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        if (icon == "rec")
        {
            g.fillEllipse (r.reduced (r.getWidth() * 0.22f));
        }
        else if (icon == "loop")
        {
            const auto ctr = r.getCentre();
            const float rad = r.getWidth() * 0.42f;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.6f, juce::MathConstants<float>::twoPi - 0.2f, true);
            g.strokePath (p, stroke);
            const auto tip = ctr.getPointOnCircumference (rad, 0.6f);
            juce::Path head;
            head.addTriangle (tip.x - 2.5f, tip.y - 3.0f, tip.x + 3.0f, tip.y - 0.5f, tip.x - 1.5f, tip.y + 2.5f);
            g.fillPath (head);
        }
        else if (icon == "reset")
        {
            const auto ctr = r.getCentre();
            const float rad = r.getWidth() * 0.42f;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, -0.4f, -0.4f - 4.9f, true);
            g.strokePath (p, stroke);
            const auto tip = ctr.getPointOnCircumference (rad, -0.4f);
            juce::Path head;
            head.addTriangle (tip.x - 0.5f, tip.y - 3.0f, tip.x + 3.5f, tip.y + 0.5f, tip.x - 2.5f, tip.y + 2.0f);
            g.fillPath (head);
        }
        else if (icon == "chevron")
        {
            p.startNewSubPath (r.getX() + 1.0f, r.getBottom() - 2.5f);
            p.lineTo (r.getCentreX(), r.getY() + 2.0f);
            p.lineTo (r.getRight() - 1.0f, r.getBottom() - 2.5f);
            g.strokePath (p, stroke);
        }
        else if (icon == "save")
        {
            p.startNewSubPath (r.getCentreX(), r.getY());
            p.lineTo (r.getCentreX(), r.getBottom() - 3.0f);
            p.startNewSubPath (r.getCentreX() - 3.5f, r.getBottom() - 6.5f);
            p.lineTo (r.getCentreX(), r.getBottom() - 3.0f);
            p.lineTo (r.getCentreX() + 3.5f, r.getBottom() - 6.5f);
            p.startNewSubPath (r.getX(), r.getBottom());
            p.lineTo (r.getRight(), r.getBottom());
            g.strokePath (p, stroke);
        }
        else if (icon == "checkOff" || icon == "checkOn")
        {
            // casella di spunta: quadratino incassato, acceso e con la spunta quando e selezionata
            const auto box = r.reduced (0.5f);
            if (icon == "checkOn")
            {
                juce::Path b; b.addRoundedRectangle (box, 2.5f);
                juce::DropShadow (Colours::accent.withAlpha (0.8f), 6, {}).drawForPath (g, b);
                g.setColour (Colours::accent);
                g.fillPath (b);
                juce::Path tick;
                tick.startNewSubPath (box.getX() + box.getWidth() * 0.24f, box.getCentreY() + 0.2f);
                tick.lineTo (box.getX() + box.getWidth() * 0.43f, box.getBottom() - box.getHeight() * 0.26f);
                tick.lineTo (box.getRight() - box.getWidth() * 0.22f, box.getY() + box.getHeight() * 0.27f);
                g.setColour (juce::Colours::white);
                g.strokePath (tick, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            else
            {
                g.setColour (juce::Colour (0xff121316));
                g.fillRoundedRectangle (box, 2.5f);
                g.setColour (c.withMultipliedAlpha (0.8f));
                g.drawRoundedRectangle (box, 2.5f, 1.0f);
            }
        }
    }
}

//==============================================================================
// Logo Chef Catta: cappello da chef stilizzato con un'onda sonora sulla fascia
static void drawChefLogo (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour line, juce::Colour wave)
{
    const float w = r.getWidth(), h = r.getHeight();
    const float x = r.getX(), y = r.getY();
    const auto P = [&] (float fx, float fy) { return juce::Point<float> (x + fx * w, y + fy * h); };

    // cupola: tre "sbuffi"
    juce::Path hat;
    hat.startNewSubPath (P (0.22f, 0.66f));
    hat.lineTo (P (0.22f, 0.52f));
    hat.cubicTo (P (0.00f, 0.50f), P (0.02f, 0.18f), P (0.26f, 0.24f));
    hat.cubicTo (P (0.30f, 0.00f), P (0.70f, 0.00f), P (0.74f, 0.24f));
    hat.cubicTo (P (0.98f, 0.18f), P (1.00f, 0.50f), P (0.78f, 0.52f));
    hat.lineTo (P (0.78f, 0.66f));

    // fascia
    juce::Path band;
    band.addRoundedRectangle (juce::Rectangle<float> (P (0.20f, 0.66f), P (0.80f, 0.92f)), h * 0.06f);

    const juce::PathStrokeType st (juce::jmax (1.0f, w * 0.075f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.strokePath (hat, st, juce::AffineTransform::translation (0.0f, 1.0f));
    g.strokePath (band, st, juce::AffineTransform::translation (0.0f, 1.0f));
    g.setColour (line);
    g.strokePath (hat, st);
    g.strokePath (band, st);

    // onda sonora dentro la fascia
    juce::Path sine;
    const float bx0 = x + 0.36f * w, bx1 = x + 0.64f * w, by = y + 0.79f * h, amp = 0.045f * h;
    sine.startNewSubPath (bx0, by);
    for (int i = 1; i <= 24; ++i)
    {
        const float t = (float) i / 24.0f;
        sine.lineTo (bx0 + t * (bx1 - bx0), by - amp * std::sin (t * juce::MathConstants<float>::twoPi * 1.5f) * (1.0f - std::abs (t - 0.5f)));
    }
    g.setColour (wave);
    g.strokePath (sine, juce::PathStrokeType (juce::jmax (0.8f, w * 0.05f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
RiserLookAndFeel::RiserLookAndFeel()
{
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void RiserLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const auto  bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c      = bounds.getCentre();
    const float angle  = startAngle + pos * (endAngle - startAngle);
    const float r      = radius * 0.78f;
    const float f      = r * 0.955f;

    // Scala a puntini sottili (economica: solo ellissi piene)
    constexpr int ticks = 49;
    for (int i = 0; i < ticks; ++i)
    {
        const float t  = (float) i / (float) (ticks - 1);
        const float a  = startAngle + t * (endAngle - startAngle);
        const bool  major = (i % 12) == 0;
        const auto  pt = c.getPointOnCircumference (radius - 4.0f, a);
        const float s  = major ? 1.6f : 1.0f;

        if (t <= pos + 1.0e-4f)
        {
            g.setColour (Colours::accent2.withAlpha (0.22f));
            g.fillEllipse (pt.x - s * 2.6f, pt.y - s * 2.6f, s * 5.2f, s * 5.2f);
            g.setColour (Colours::accent.interpolatedWith (Colours::accent2, t));
        }
        else
        {
            g.setColour (major ? Colours::dim : Colours::faint.brighter (0.2f));
        }
        g.fillEllipse (pt.x - s, pt.y - s, s * 2.0f, s * 2.0f);
    }

    // Corpo della manopola (ombra, bordo, faccia): disegnato una volta sola e tenuto in memoria
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int   side  = juce::roundToInt (radius * 2.0f);
    if (knobCache.isNull() || knobCacheSide != side || std::abs (knobCacheScale - scale) > 0.01f)
    {
        knobCacheSide  = side;
        knobCacheScale = scale;
        knobCache = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt ((float) side * scale)),
                                 juce::jmax (1, juce::roundToInt ((float) side * scale)), true);
        juce::Graphics kg (knobCache);
        kg.addTransform (juce::AffineTransform::scale (scale));
        const juce::Point<float> kc ((float) side * 0.5f, (float) side * 0.5f);

        juce::Path body;
        body.addEllipse (kc.x - r, kc.y - r, r * 2.0f, r * 2.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.9f), juce::roundToInt (r * 0.5f),
                          { 0, juce::roundToInt (r * 0.2f) }).drawForPath (kg, body);

        kg.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3f45), kc.x, kc.y - r,
                                                  juce::Colour (0xff0e0f11), kc.x, kc.y + r, false));
        kg.fillPath (body);

        kg.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1c20), kc.x, kc.y - f,
                                                  juce::Colour (0xff2a2b30), kc.x, kc.y + f, false));
        kg.fillEllipse (kc.x - f, kc.y - f, f * 2.0f, f * 2.0f);

        kg.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.035f), kc.x, kc.y - f,
                                                  juce::Colours::transparentWhite, kc.x, kc.y + f * 0.2f, false));
        kg.fillEllipse (kc.x - f, kc.y - f, f * 2.0f, f * 2.0f);
        juce::Path rim;
        rim.addCentredArc (kc.x, kc.y, f - 1.0f, f - 1.0f, 0.0f, juce::MathConstants<float>::pi * 0.65f,
                           juce::MathConstants<float>::pi * 1.35f, true);
        kg.setColour (juce::Colours::white.withAlpha (0.07f));
        kg.strokePath (rim, juce::PathStrokeType (1.2f));
    }
    g.drawImage (knobCache, juce::Rectangle<float> ((float) side, (float) side).withCentre (c));

    // Indicatore: tacca chiara
    const auto p1 = c.getPointOnCircumference (f * 0.62f, angle);
    const auto p2 = c.getPointOnCircumference (f * 0.86f, angle);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (juce::Line<float> (p1, p2).withShortenedStart (-1.0f).withShortenedEnd (-1.0f), 5.5f);
    g.setColour (Colours::text);
    g.drawLine ({ p1, p2 }, 3.0f);
}

// Tutti i tasti sono "a testo": niente cornici, si illuminano quando sono attivi
void RiserLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool)
{
    if (highlighted && b.isEnabled() && ! b.getToggleState())
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.fillRoundedRectangle (b.getLocalBounds().toFloat().reduced (2.0f), 6.0f);
    }
}

void RiserLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool down)
{
    const bool forced = (bool) b.getProperties()["forceOn"];
    const bool on   = b.getToggleState() || forced;
    auto icon = b.getProperties()["icon"].toString();
    if (b.getProperties()["check"]) icon = on ? "checkOn" : "checkOff";
    auto r = b.getLocalBounds().toFloat();
    if (down) r.translate (0.0f, 1.0f);

    const auto font = labelFont (b.getProperties().getWithDefault ("size", 10.5f), 0.16f);
    g.setFont (font);

    const auto text  = b.getButtonText();
    const float tw   = juce::GlyphArrangement::getStringWidth (font, text);
    const bool  isCheck = icon.startsWith ("check");
    const float iw   = icon.isNotEmpty() ? (isCheck ? 11.0f : 9.0f) : 0.0f;
    const float gap  = icon.isNotEmpty() ? (isCheck ? 8.0f : 6.0f) : 0.0f;
    float xx = r.getCentreX() - (iw + gap + tw) * 0.5f;
    const float alpha = (b.isEnabled() || forced) ? 1.0f : 0.45f;

    auto col = on ? Colours::accent2 : (highlighted ? Colours::text : Colours::dim);
    col = col.withMultipliedAlpha (alpha);

    if (icon.isNotEmpty())
    {
        drawIcon (g, icon, { xx, r.getCentreY() - iw * 0.5f, iw, iw }, col);
        xx += iw + gap;
    }

    const auto tr = juce::Rectangle<float> (xx, r.getY(), tw + 2.0f, r.getHeight());
    if (on && (b.isEnabled() || forced)) glowText (g, text, tr, juce::Justification::centredLeft);
    else                     engravedText (g, text, tr, juce::Justification::centredLeft, col);

    // Spia: piccola lineetta luminosa sotto al testo dei tasti attivi
    if (on && b.isEnabled() && b.getProperties()["led"])
    {
        auto bar = juce::Rectangle<float> (12.0f, 2.0f).withCentre ({ r.getCentreX(), r.getCentreY() + 10.0f });
        juce::Path p;
        p.addRoundedRectangle (bar, 1.0f);
        juce::DropShadow (Colours::accent.withAlpha (0.9f), 6, {}).drawForPath (g, p);
        g.setColour (Colours::accent2);
        g.fillPath (p);
    }
}

void RiserLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle, juce::Slider& s)
{
    const auto r = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float cy = r.getCentreY();
    const auto track = juce::Rectangle<float> (r.getX(), cy - 2.0f, r.getWidth(), 4.0f);

    // binario scavato
    g.setColour (juce::Colour (0xff0d0e10));
    g.fillRoundedRectangle (track, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine ((int) track.getBottom(), track.getX() + 2.0f, track.getRight() - 2.0f);

    // parte attiva, luminosa
    const float px = juce::jlimit (r.getX(), r.getRight(), pos);
    auto fill = track.withRight (px);
    juce::Path fp; fp.addRoundedRectangle (fill, 2.0f);
    juce::DropShadow (Colours::accent.withAlpha (0.7f), 6, {}).drawForPath (g, fp);
    g.setGradientFill (juce::ColourGradient (Colours::accent, track.getX(), 0.0f, Colours::accent2, px, 0.0f, false));
    g.fillPath (fp);

    // cursore in rilievo
    const float kr = 7.5f;
    juce::Path knob; knob.addEllipse (px - kr, cy - kr, kr * 2.0f, kr * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 5, { 0, 2 }).drawForPath (g, knob);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4c53), px, cy - kr, juce::Colour (0xff1c1d21), px, cy + kr, false));
    g.fillPath (knob);
    g.setColour (s.isMouseOverOrDragging() ? Colours::accent2 : juce::Colours::white.withAlpha (0.5f));
    g.fillEllipse (px - 1.5f, cy - 1.5f, 3.0f, 3.0f);
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
    pingPong.getProperties().set ("size", 9.5f);
    pingPong.getProperties().set ("check", true);      // semplice opzione con casella di spunta
    addAndMakeVisible (pingPong);
    ppAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "pingPong", pingPong);

    setSize (width, height);
}

void StutterPopup::paint (juce::Graphics& g)
{
    // riquadro con la punta in basso verso STUTTER
    const auto box = getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom ((float) arrowH);
    juce::Path p;
    p.addRoundedRectangle (box, 8.0f);
    juce::Path tip;
    tip.addTriangle (arrowX - 7.0f, box.getBottom() - 1.0f, arrowX + 7.0f, box.getBottom() - 1.0f, arrowX, box.getBottom() + (float) arrowH - 1.0f);
    p.addPath (tip);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2d32), 0.0f, box.getY(), juce::Colour (0xff1b1c20), 0.0f, box.getBottom(), false));
    g.fillPath (p);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (p, juce::PathStrokeType (1.0f));

    g.setFont (labelFont (8.0f, 0.2f));
    const auto d = depth.getBounds().toFloat();
    engravedText (g, juce::CharPointer_UTF8 ("QUANTIT\xc3\x80"), { d.getX(), d.getY() - 12.0f, 80.0f, 11.0f }, juce::Justification::centredLeft, Colours::dim);
    engravedText (g, juce::String (juce::roundToInt (depth.getValue() * 100.0)) + " %",
                  { d.getRight() - 50.0f, d.getY() - 12.0f, 50.0f, 11.0f }, juce::Justification::centredRight, Colours::text);
}

void StutterPopup::resized()
{
    auto r = getLocalBounds().withTrimmedBottom (arrowH).reduced (10, 6);
    timing.setBounds (r.removeFromTop (32).withSizeKeepingCentre (130, 32));
    r.removeFromTop (14);
    depth.setBounds (r.removeFromTop (18));
    pingPong.setBounds (r.removeFromTop (r.getHeight()).withSizeKeepingCentre (110, 26));
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

    auto inner = getLocalBounds().toFloat().reduced (6.0f, 16.0f);
    const float mid  = inner.getCentreY();
    const int   bars = juce::jmax (24, (int) (inner.getWidth() / 3.2f));
    const float bw   = inner.getWidth() / (float) bars;
    const float half = inner.getHeight() * 0.5f;
    const int   nb   = (int) peaks->size();

    juce::Path w;
    for (int i = 0; i < bars; ++i)
    {
        const int k0 = i * nb / bars, k1 = juce::jmax (k0 + 1, (i + 1) * nb / bars);
        float m = 0.0f;
        for (int k = k0; k < k1; ++k) m = juce::jmax (m, (*peaks)[(size_t) k]);
        const float hh = juce::jmax (0.8f, m * half);
        w.addRoundedRectangle (inner.getX() + (float) i * bw + bw * 0.25f, mid - hh, bw * 0.5f, hh * 2.0f, bw * 0.25f);
    }

    auto paintLayer = [&] (juce::Image& img, auto&& fn)
    {
        juce::Graphics lg (img);
        lg.addTransform (juce::AffineTransform::scale (scale));
        fn (lg);
    };

    paintLayer (normalImg, [&] (juce::Graphics& lg)
    {
        juce::DropShadow (Colours::accent.withAlpha (0.7f), 16, {}).drawForPath (lg, w);
        lg.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.35f), inner.getX(), 0.0f,
                                                  Colours::accent2, inner.getRight(), 0.0f, false));
        lg.fillPath (w);
    });
    paintLayer (dimImg, [&] (juce::Graphics& lg)
    {
        lg.setColour (Colours::accent.withAlpha (0.16f));
        lg.fillPath (w);
    });
    paintLayer (litImg, [&] (juce::Graphics& lg)
    {
        juce::DropShadow (Colours::accent.withAlpha (0.8f), 14, {}).drawForPath (lg, w);
        lg.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.55f), inner.getX(), 0.0f,
                                                  Colours::accent2, inner.getRight(), 0.0f, false));
        lg.fillPath (w);
    });
    paintLayer (hotImg, [&] (juce::Graphics& lg)
    {
        juce::DropShadow (Colours::accent2, 10, {}).drawForPath (lg, w);
        lg.setColour (juce::Colours::white.withAlpha (0.95f));
        lg.fillPath (w);
    });
}

void RiserWaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto inner = r.reduced (6.0f, 16.0f);
    const float mid = inner.getCentreY();

    if (! proc.hasRender())
    {
        g.setColour (Colours::faint);
        for (float xx = inner.getX(); xx < inner.getRight(); xx += 5.0f)
            g.fillRect (xx, mid - 0.5f, 2.0f, 1.0f);
        return;
    }

    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (normalImg.isNull() || layerGen != proc.getRenderGeneration() || std::abs (layerScale - scale) > 0.01f
         || layerSize != getLocalBounds().getBottomRight())
        rebuildLayers (scale);

    if (! proc.isPreviewPlaying())
    {
        g.drawImage (normalImg, r);
    }
    else
    {
        const float px = inner.getX() + inner.getWidth() * proc.previewProgress.load();

        // da suonare: spenta
        g.drawImage (dimImg, r);

        // gia suonata: accesa
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<float> (0.0f, 0.0f, px, r.getHeight()).toNearestInt());
            g.drawImage (litImg, r);
        }

        // barre appena toccate dalla testina: quasi bianche
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (juce::Rectangle<float> (px - 26.0f, 0.0f, 26.0f, r.getHeight()).toNearestInt());
            g.drawImage (hotImg, r);
        }

        // alone morbido dietro la testina
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::scale (1.0f, 0.55f, px, mid));
            g.setGradientFill (juce::ColourGradient (Colours::accent2.withAlpha (0.2f), px, mid,
                                                     Colours::accent2.withAlpha (0.0f), px - 80.0f, mid, true));
            g.fillEllipse (px - 80.0f, mid - 80.0f, 160.0f, 160.0f);
        }

        // linea di scansione
        g.setGradientFill (juce::ColourGradient (Colours::accent2.withAlpha (0.0f), inner.getX(), mid,
                                                 Colours::accent2.withAlpha (0.35f), px, mid, false));
        g.fillRect (inner.getX(), mid - 0.5f, juce::jmax (0.0f, px - inner.getX()), 1.0f);

        // testina: fascio verticale
        juce::ColourGradient beam (Colours::accent2.withAlpha (0.0f), px, inner.getY() - 12.0f,
                                   Colours::accent2.withAlpha (0.0f), px, inner.getBottom() + 12.0f, false);
        beam.addColour (0.5, Colours::accent2.withAlpha (0.45f));
        g.setGradientFill (beam);
        g.fillRect (px - 5.0f, inner.getY() - 12.0f, 10.0f, inner.getHeight() + 24.0f);
        beam = juce::ColourGradient (juce::Colours::white.withAlpha (0.0f), px, inner.getY() - 12.0f,
                                     juce::Colours::white.withAlpha (0.0f), px, inner.getBottom() + 12.0f, false);
        beam.addColour (0.5, juce::Colours::white);
        g.setGradientFill (beam);
        g.fillRect (px - 0.75f, inner.getY() - 12.0f, 1.5f, inner.getHeight() + 24.0f);
        g.setColour (Colours::accent2.withAlpha (0.35f));
        g.fillEllipse (px - 5.0f, mid - 5.0f, 10.0f, 10.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (px - 2.0f, mid - 2.0f, 4.0f, 4.0f);
    }
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
    step (e.x < getWidth() / 3 ? -1 : 1);
}

void ArrowSelector::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float alpha = isEnabled() ? 1.0f : 0.45f;

    g.setFont (labelFont (8.5f, 0.2f));
    engravedText (g, title, r.removeFromTop (12.0f), juce::Justification::centred, Colours::dim.withMultipliedAlpha (alpha));

    g.setFont (labelFont (11.5f, 0.14f));
    engravedText (g, values[index], r, juce::Justification::centred, Colours::text.withMultipliedAlpha (alpha));

    g.setFont (labelFont (13.0f, 0.0f));
    engravedText (g, juce::CharPointer_UTF8 ("\xe2\x80\xb9"), r.removeFromLeft (16.0f), juce::Justification::centred, Colours::dim.withMultipliedAlpha (alpha));
    engravedText (g, juce::CharPointer_UTF8 ("\xe2\x80\xba"), r.removeFromRight (16.0f), juce::Justification::centred, Colours::dim.withMultipliedAlpha (alpha));
}

//==============================================================================
// Griglia di forellini incisi nel pannello (decorazione)
static void drawDot (juce::Graphics& g, float x, float y)
{
    const float rad = 1.6f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (x - rad, y - rad + 0.9f, rad * 2.0f, rad * 2.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff666a73), x, y - rad,
                                             juce::Colour (0xff3a3d43), x, y + rad, false));
    g.fillEllipse (x - rad, y - rad, rad * 2.0f, rad * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.fillEllipse (x - rad * 0.4f, y - rad * 0.65f, rad * 0.7f, rad * 0.5f);
}

// Gruppo a triangolo (3-2-1) che punta verso l'angolo indicato da dx/dy (+1 o -1)
static void drawDotCorner (juce::Graphics& g, juce::Point<float> corner, float dx, float dy, float sp = 8.0f)
{
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3 - row; ++col)
            drawDot (g, corner.x + dx * (float) col * sp, corner.y + dy * (float) row * sp);
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

    // Comandi nel display
    captureBtn.getProperties().set ("icon", "rec");
    loopBtn.getProperties().set ("icon", "loop");
    saveBtn.getProperties().set ("icon", "save");

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

    resetBtn.getProperties().set ("icon", "reset");
    resetBtn.setTooltip ("Cancella la frase catturata (i controlli restano come sono)");
    resetBtn.onClick = [this]
    {
        proc.clearCapture();      // via l'audio catturato; i controlli restano come sono
        wave.repaint();
    };

    for (auto* b : { &captureBtn, &loopBtn, &saveBtn, &resetBtn })
        addAndMakeVisible (*b);

    // Manopola
    amountKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amountKnob.setDoubleClickReturnValue (true, 0.6);
    addAndMakeVisible (amountKnob);
    amountAtt = std::make_unique<SliderAtt> (apvts, "amount", amountKnob);
    amountKnob.onValueChange = [this] { repaint (intensityArea); };

    // Selettori
    addAndMakeVisible (lengthSel);
    // Freccia accanto a STUTTER: apre il menu con TIMING, QUANTITA e PING PONG
    stutMenuBtn.getProperties().set ("icon", "chevron");
    stutMenuBtn.setTooltip ("Opzioni dello stutter");
    stutMenuBtn.onClick = [this] { showStutterMenu (! stutPopup.isVisible()); };
    addAndMakeVisible (stutMenuBtn);
    addChildComponent (stutPopup);
    addMouseListener (this, true);          // per chiudere il menu con un clic fuori

    stereoBtn.setClickingTogglesState (true);
    stereoBtn.getProperties().set ("size", 11.5f);
    stereoBtn.setTooltip ("Allarga una voce mono in stereo");
    addAndMakeVisible (stereoBtn);
    buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, "stereoOn", stereoBtn));

    // Effetti
    const char* fxNames[] = { "FILTRO", "STUTTER", "PHONE" };
    const char* fxIds[]   = { "filterOn", "stutOn", "phoneOn" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = fxBtns[(size_t) i];
        b.setButtonText (fxNames[i]);
        b.getProperties().set ("led", true);
        b.setClickingTogglesState (true);
        addAndMakeVisible (b);
        buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, fxIds[i], b));
    }
    // accendendo lo STUTTER si apre anche il suo menu
    fxBtns[1].onClick = [this] { if (fxBtns[1].getToggleState()) showStutterMenu (true); else showStutterMenu (false); };

    setSize (460, 586);
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
    // Sfondo statico (ombre, display, puntini, logo): calcolato una volta e riusato
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
    g.drawImage (bgCache, getLocalBounds().toFloat());

    // Parti che cambiano
    auto lab = intensityArea.toFloat().withTrimmedTop (12.0f);
    g.setFont (labelFont (12.5f, 0.12f));
    engravedText (g, juce::String (juce::roundToInt (amountKnob.getValue() * 100.0)) + " %", lab,
                  juce::Justification::centred, Colours::text);

    const auto sb = stereoBtn.getBounds().toFloat();
    g.setFont (labelFont (8.5f, 0.2f));
    engravedText (g, proc.getSourceStereo() == 1 ? "SORGENTE STEREO" : "IMMAGINE",
                  { sb.getX() - 20.0f, sb.getY() - 12.0f, sb.getWidth() + 40.0f, 12.0f },
                  juce::Justification::centred, Colours::dim);
}

void VocalRiserAudioProcessorEditor::paintStatic (juce::Graphics& g)
{
    g.fillAll (Colours::bg);

    // Corpo
    auto shellR = getLocalBounds().reduced (8).toFloat();
    juce::Path shell;
    shell.addRoundedRectangle (shellR, 16.0f);
    juce::DropShadow (juce::Colours::black, 16, { 0, 6 }).drawForPath (g, shell);
    g.setGradientFill (juce::ColourGradient (Colours::bodyTop, 0.0f, shellR.getY(), Colours::bodyBot, 0.0f, shellR.getBottom(), false));
    g.fillPath (shell);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), 0.0f, shellR.getY(),
                                             juce::Colours::black.withAlpha (0.6f), 0.0f, shellR.getBottom(), false));
    g.strokePath (shell, juce::PathStrokeType (1.0f));


    // Display scuro in alto con l'incavo che abbraccia la manopola
    const auto d = displayArea.toFloat();
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (shell);
        juce::DropShadow (juce::Colours::black.withAlpha (0.85f), 18, { 0, 8 }).drawForPath (g, displayShape);
    }
    g.setGradientFill (juce::ColourGradient (Colours::displayTop, 0.0f, d.getY(), Colours::displayBot, 0.0f, d.getBottom(), false));
    g.fillPath (displayShape);
    g.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.07f), d.getCentreX(), d.getY() + 90.0f,
                                             juce::Colours::transparentBlack, d.getCentreX() + d.getWidth() * 0.45f, d.getY() + 90.0f, true));
    g.fillPath (displayShape);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.0f), 0.0f, d.getY(),
                                             juce::Colours::white.withAlpha (0.07f), 0.0f, d.getBottom(), false));
    g.strokePath (displayShape, juce::PathStrokeType (1.0f));

    // Titolo e stato
    const auto titleR = juce::Rectangle<float> (d.getX() + 30.0f, d.getY() + 14.0f, 200.0f, 16.0f);
    g.setColour (Colours::accent2.withAlpha (0.3f));
    g.fillEllipse (d.getX() + 16.0f, titleR.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour (Colours::accent2);
    g.fillEllipse (d.getX() + 18.0f, titleR.getCentreY() - 2.0f, 4.0f, 4.0f);
    g.setFont (labelFont (11.0f, 0.3f));
    engravedText (g, "CHEF RISER", titleR, juce::Justification::centredLeft, Colours::text);


    // Alloggiamento morbido della manopola, dentro l'incavo
    const auto kc = knobCentre;
    const float kr = (float) knobArea.getWidth() * 0.5f + 12.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.4f), kc.x, kc.y - kr,
                                             juce::Colours::white.withAlpha (0.03f), kc.x, kc.y + kr, false));
    g.fillEllipse (kc.x - kr, kc.y - kr, kr * 2.0f, kr * 2.0f);


    // Valore intensita
    // sotto l'alloggiamento della manopola, impaginato come gli altri selettori
    {
        auto lab = intensityArea.toFloat();
        g.setFont (labelFont (8.5f, 0.2f));
        engravedText (g, juce::CharPointer_UTF8 ("INTENSIT\xc3\x80"), lab.removeFromTop (12.0f),
                      juce::Justification::centred, Colours::dim);
    }


    // Separatore sottile sopra agli effetti
    const float sepY = (float) fxBtns[0].getY() - 8.0f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (shellR.getX() + 40.0f, sepY, shellR.getWidth() - 80.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (shellR.getX() + 40.0f, sepY + 1.0f, shellR.getWidth() - 80.0f, 1.0f);

    // Puntini decorativi agli angoli della sezione centrale
    {
        const float lx = shellR.getX() + 22.0f, rx = shellR.getRight() - 22.0f;
        const float topY = (float) displayArea.getBottom() + 22.0f;
        const float lowY = (float) intensityArea.getBottom();
        drawDotCorner (g, { lx, topY },  1.0f,  1.0f);
        drawDotCorner (g, { rx, topY }, -1.0f,  1.0f);
        drawDotCorner (g, { lx, lowY },  1.0f, -1.0f);
        drawDotCorner (g, { rx, lowY }, -1.0f, -1.0f);
    }

    // Firma dello sviluppatore
    {
        // marchio: "CHEF CATTA" maiuscolo spaziato + "dsp" minuscolo
        const auto nameFont = labelFont (9.0f, 0.3f);
        const auto dspFont  = juce::Font (juce::FontOptions (10.5f)).withExtraKerningFactor (0.06f);
        const juce::String name ("CHEF CATTA"), dsp ("dsp");
        const float nw = juce::GlyphArrangement::getStringWidth (nameFont, name) - 9.0f * 0.3f;
        const float dw = juce::GlyphArrangement::getStringWidth (dspFont, dsp);
        const float logo = 16.0f, gap = 7.0f, gap2 = 5.0f;
        const float total = logo + gap + nw + gap2 + dw;
        const float x0 = shellR.getCentreX() - total * 0.5f;
        const float cy = shellR.getBottom() - 15.0f;

        drawChefLogo (g, { x0, cy - logo * 0.5f - 1.0f, logo, logo }, Colours::dim, Colours::dim);
        g.setFont (nameFont);
        engravedText (g, name, { x0 + logo + gap, cy - 6.0f, nw + 6.0f, 12.0f }, juce::Justification::centredLeft, Colours::dim);
        g.setFont (dspFont);
        engravedText (g, dsp, { x0 + logo + gap + nw + gap2, cy - 7.0f, dw + 4.0f, 13.0f }, juce::Justification::centredLeft,
                      Colours::dim.brighter (0.25f));
    }
}

void VocalRiserAudioProcessorEditor::resized()
{
    bgCache = {};
    auto shell = getLocalBounds().reduced (8);

    // Display in alto
    displayArea = shell.withHeight (226).reduced (8, 8);
    const auto d = displayArea.toFloat();

    // Manopola: il suo centro sta sotto al display, l'incavo "morde" il bordo inferiore
    const int ks = 190;
    knobCentre  = { d.getCentreX(), d.getBottom() + 64.0f };
    knobArea    = juce::Rectangle<int> (ks, ks).withCentre (knobCentre.toInt());
    notchRadius = (float) ks * 0.5f + 24.0f;
    amountKnob.setBounds (knobArea);

    // Forma del display: rettangolo arrotondato con l'incavo circolare sul fondo
    {
        const float rc = 12.0f, fil = 8.0f;
        const float bottom = d.getBottom();
        const float dy = knobCentre.y - bottom;
        const float dx = std::sqrt (juce::jmax (0.0f, notchRadius * notchRadius - dy * dy));
        const float th = std::asin (dx / notchRadius);

        juce::Path p;
        p.startNewSubPath (d.getX() + rc, d.getY());
        p.lineTo (d.getRight() - rc, d.getY());
        p.quadraticTo (d.getRight(), d.getY(), d.getRight(), d.getY() + rc);
        p.lineTo (d.getRight(), bottom - rc);
        p.quadraticTo (d.getRight(), bottom, d.getRight() - rc, bottom);
        p.lineTo (knobCentre.x + dx + fil, bottom);
        p.quadraticTo (knobCentre.x + dx, bottom, knobCentre.x + dx - fil * 0.35f, bottom - fil * 0.6f);
        p.addCentredArc (knobCentre.x, knobCentre.y, notchRadius, notchRadius, 0.0f, th - 0.06f, -th + 0.06f, false);
        p.quadraticTo (knobCentre.x - dx, bottom, knobCentre.x - dx - fil, bottom);
        p.lineTo (d.getX() + rc, bottom);
        p.quadraticTo (d.getX(), bottom, d.getX(), bottom - rc);
        p.lineTo (d.getX(), d.getY() + rc);
        p.quadraticTo (d.getX(), d.getY(), d.getX() + rc, d.getY());
        p.closeSubPath();
        displayShape = p;
    }

    // Riga in alto: SALVA e RESET a destra
    auto top = displayArea.withHeight (44).reduced (10, 8);
    resetBtn.setBounds (top.removeFromRight (74));
    saveBtn.setBounds (top.removeFromRight (74));

    // Forma d'onda sopra l'incavo
    wave.setBounds (displayArea.getX() + 24, displayArea.getY() + 50, displayArea.getWidth() - 48, 100);

    // CATTURA e LOOP ai lati della manopola, nel fondo del display
    const int ctrlY = displayArea.getBottom() - 36;
    captureBtn.setBounds (displayArea.getX() + 12, ctrlY, 100, 28);
    loopBtn.setBounds (displayArea.getRight() - 100, ctrlY, 88, 28);

    // Sotto la manopola: selettori, effetti, firma
    // Intensita: sotto l'alloggiamento della manopola, con un po' di respiro
    const int recessBottom = juce::roundToInt (knobCentre.y + (float) ks * 0.5f + 12.0f);
    intensityArea = juce::Rectangle<int> (juce::roundToInt (knobCentre.x) - 70, recessBottom + 10, 140, 30);

    int y = intensityArea.getBottom() + 18;
    auto sel = juce::Rectangle<int> (shell.getX() + 18, y, shell.getWidth() - 36, 40);
    sel = sel.reduced (20, 0);
    sel = sel.reduced (30, 0);
    const int sw = sel.getWidth() / 2;
    lengthSel.setBounds (sel.removeFromLeft (sw).reduced (4, 0));
    stereoBtn.setBounds (sel.reduced (4, 0).withTrimmedTop (12));

    y += 40 + 18;
    auto fx = juce::Rectangle<int> (shell.getX() + 30, y, shell.getWidth() - 60, 32);
    fx = fx.reduced (30, 0);
    const int fw = fx.getWidth() / (int) fxBtns.size();
    for (auto& b : fxBtns) b.setBounds (fx.removeFromLeft (fw));

    // la freccia sta subito a destra della scritta STUTTER
    const auto sb = fxBtns[1].getBounds();
    stutMenuBtn.setBounds (sb.getRight() - 14, sb.getY() + 3, 18, 18);
}

void VocalRiserAudioProcessorEditor::showStutterMenu (bool show)
{
    if (show)
    {
        const auto sb = fxBtns[1].getBounds();
        const int x = juce::jlimit (12, getWidth() - StutterPopup::width - 12, sb.getCentreX() - StutterPopup::width / 2);
        stutPopup.setTopLeftPosition (x, sb.getY() - StutterPopup::height + 2);
        stutPopup.setArrowX ((float) (sb.getCentreX() - x));
        stutPopup.toFront (false);
    }
    stutPopup.setVisible (show);
    stutMenuBtn.setToggleState (show, juce::dontSendNotification);
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
    captureBtn.setToggleState (proc.getCaptureState() != VocalRiserAudioProcessor::stateIdle, juce::dontSendNotification);

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
        repaint();
    }
    if (src != 1)
    {
        const auto t = stereoBtn.getToggleState() ? "STEREO" : "MONO";
        if (stereoBtn.getButtonText() != t) stereoBtn.setButtonText (t);
    }
}
