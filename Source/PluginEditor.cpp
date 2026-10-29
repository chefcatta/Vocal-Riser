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
    }
}

//==============================================================================
// Logo chef catta dsp: cappello da chef stilizzato con un'onda sonora sulla fascia
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

    // Scala a puntini sottili
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

    // Disco opaco che proietta un'ombra morbida
    const float r = radius * 0.78f;
    juce::Path body;
    body.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.9f), juce::roundToInt (r * 0.5f),
                      { 0, juce::roundToInt (r * 0.2f) }).drawForPath (g, body);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3f45), c.x, c.y - r,
                                             juce::Colour (0xff0e0f11), c.x, c.y + r, false));
    g.fillPath (body);

    const float f = r * 0.955f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1c20), c.x, c.y - f,
                                             juce::Colour (0xff2a2b30), c.x, c.y + f, false));
    g.fillEllipse (c.x - f, c.y - f, f * 2.0f, f * 2.0f);

    // velatura opaca dall'alto + bordo luminoso in basso (faccia leggermente concava)
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.035f), c.x, c.y - f,
                                             juce::Colours::transparentWhite, c.x, c.y + f * 0.2f, false));
    g.fillEllipse (c.x - f, c.y - f, f * 2.0f, f * 2.0f);
    juce::Path rim;
    rim.addCentredArc (c.x, c.y, f - 1.0f, f - 1.0f, 0.0f, juce::MathConstants<float>::pi * 0.65f,
                       juce::MathConstants<float>::pi * 1.35f, true);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.strokePath (rim, juce::PathStrokeType (1.2f));

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
    const auto icon = b.getProperties()["icon"].toString();
    const bool forced = (bool) b.getProperties()["forceOn"];
    const bool on   = b.getToggleState() || forced;
    auto r = b.getLocalBounds().toFloat();
    if (down) r.translate (0.0f, 1.0f);

    const auto font = labelFont (b.getProperties().getWithDefault ("size", 10.5f), 0.16f);
    g.setFont (font);

    const auto text  = b.getButtonText();
    const float tw   = juce::GlyphArrangement::getStringWidth (font, text);
    const float iw   = icon.isNotEmpty() ? 9.0f : 0.0f;
    const float gap  = icon.isNotEmpty() ? 6.0f : 0.0f;
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

//==============================================================================
void RiserWaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto inner = r.reduced (6.0f, 16.0f);
    const auto& peaks = proc.getPeaks();
    const float mid = inner.getCentreY();

    if (proc.hasRender() && ! peaks.empty())
    {
        const int   bars = juce::jmax (24, (int) (inner.getWidth() / 3.2f));
        const float bw   = inner.getWidth() / (float) bars;
        const float half = inner.getHeight() * 0.5f;
        const int   nb   = (int) peaks.size();

        const bool  sweep = proc.previewPlaying.load();
        const float px    = inner.getX() + inner.getWidth() * proc.previewProgress.load();

        juce::Path played, ahead, hot;
        for (int i = 0; i < bars; ++i)
        {
            const int k0 = i * nb / bars, k1 = juce::jmax (k0 + 1, (i + 1) * nb / bars);
            float m = 0.0f;
            for (int k = k0; k < k1; ++k) m = juce::jmax (m, peaks[(size_t) k]);

            const float bx = inner.getX() + (float) i * bw + bw * 0.25f;
            float hh = juce::jmax (0.8f, m * half);

            if (! sweep) { played.addRoundedRectangle (bx, mid - hh, bw * 0.5f, hh * 2.0f, bw * 0.25f); continue; }

            // le barre vicino alla testina "pulsano" un po' piu alte
            const float dist = std::abs (bx - px);
            if (dist < 18.0f) hh = juce::jmin (half, hh * (1.0f + 0.25f * (1.0f - dist / 18.0f)));

            auto& dest = bx > px ? ahead : (px - bx < 26.0f ? hot : played);
            dest.addRoundedRectangle (bx, mid - hh, bw * 0.5f, hh * 2.0f, bw * 0.25f);
        }

        if (! sweep)
        {
            juce::DropShadow (Colours::accent.withAlpha (0.7f), 16, {}).drawForPath (g, played);
            g.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.35f), inner.getX(), 0.0f,
                                                     Colours::accent2, inner.getRight(), 0.0f, false));
            g.fillPath (played);
        }
        else
        {
            // parte ancora da suonare: spenta, quasi un'ombra
            g.setColour (Colours::accent.withAlpha (0.16f));
            g.fillPath (ahead);

            // parte gia suonata: accesa
            juce::DropShadow (Colours::accent.withAlpha (0.8f), 14, {}).drawForPath (g, played);
            g.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.5f), inner.getX(), 0.0f,
                                                     Colours::accent2, px, 0.0f, false));
            g.fillPath (played);

            // scia luminosa dietro la testina
            {
                juce::Graphics::ScopedSaveState ss (g);
                g.addTransform (juce::AffineTransform::scale (1.0f, 0.55f, px, mid));
                g.setGradientFill (juce::ColourGradient (Colours::accent2.withAlpha (0.2f), px, mid,
                                                         Colours::accent2.withAlpha (0.0f), px - 80.0f, mid, true));
                g.fillEllipse (px - 80.0f, mid - 80.0f, 160.0f, 160.0f);
            }

            // barre "calde" appena toccate: quasi bianche
            juce::DropShadow (Colours::accent2, 10, {}).drawForPath (g, hot);
            g.setGradientFill (juce::ColourGradient (Colours::accent2, px - 26.0f, 0.0f, juce::Colours::white, px, 0.0f, false));
            g.fillPath (hot);

            // linea di scansione orizzontale
            g.setGradientFill (juce::ColourGradient (Colours::accent2.withAlpha (0.0f), inner.getX(), mid,
                                                     Colours::accent2.withAlpha (0.35f), px, mid, false));
            g.fillRect (inner.getX(), mid - 0.5f, px - inner.getX(), 1.0f);

            // testina: fascio verticale con bagliore
            g.setGradientFill (juce::ColourGradient (Colours::accent2.withAlpha (0.0f), px, inner.getY() - 10.0f,
                                                     Colours::accent2.withAlpha (0.0f), px, inner.getBottom() + 10.0f, false));
            {
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
            }
            g.setColour (Colours::accent2.withAlpha (0.35f));
            g.fillEllipse (px - 5.0f, mid - 5.0f, 10.0f, 10.0f);
            g.setColour (juce::Colours::white);
            g.fillEllipse (px - 2.0f, mid - 2.0f, 4.0f, 4.0f);
        }

        g.setFont (labelFont (9.0f, 0.12f));
        engravedText (g, proc.getRenderInfo().toUpperCase(), r.removeFromBottom (12.0f), juce::Justification::centred, Colours::dim);
    }
    else
    {
        g.setColour (Colours::faint);
        for (float xx = inner.getX(); xx < inner.getRight(); xx += 5.0f)
            g.fillRect (xx, mid - 0.5f, 2.0f, 1.0f);
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
VocalRiserAudioProcessorEditor::VocalRiserAudioProcessorEditor (VocalRiserAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), wave (p),
      lengthSel (*p.apvts.getParameter ("length"), "DURATA", { "1/4 BAR", "1/2 BAR", "1 BAR", "2 BAR" }),
      reverbSel (*p.apvts.getParameter ("revType"), "AMBIENTE", { "HALL", "PLATE", "SPACE" }),
      timingSel (*p.apvts.getParameter ("stutRate"), "TIMING STUTTER", { "1/4", "1/8", "1/16" })
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
    loopBtn.setToggleState (proc.previewLoop.load(), juce::dontSendNotification);
    loopBtn.onClick = [this] { proc.previewLoop.store (loopBtn.getToggleState()); };

    saveBtn.setTooltip ("Salva il WAV e apri la cartella");
    saveBtn.onClick = [this]
    {
        const auto f = proc.writeRiserFile();
        if (f.existsAsFile()) f.revealToUser();
    };

    resetBtn.getProperties().set ("icon", "reset");
    resetBtn.setTooltip ("Riporta tutti i controlli ai valori iniziali");
    resetBtn.onClick = [this]
    {
        proc.resetParameters();
        loopBtn.setToggleState (false, juce::dontSendNotification);
        repaint();
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
    addAndMakeVisible (reverbSel);
    addAndMakeVisible (timingSel);

    stereoBtn.setClickingTogglesState (true);
    stereoBtn.getProperties().set ("size", 11.5f);
    stereoBtn.setTooltip ("Allarga una voce mono in stereo");
    addAndMakeVisible (stereoBtn);
    buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, "stereoOn", stereoBtn));

    // Effetti
    const char* fxNames[] = { "FILTRO", "STUTTER", "PHONE", "CHORUS" };
    const char* fxIds[]   = { "filterOn", "stutOn", "phoneOn", "chorusOn" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = fxBtns[(size_t) i];
        b.setButtonText (fxNames[i]);
        b.getProperties().set ("led", true);
        b.setClickingTogglesState (true);
        addAndMakeVisible (b);
        buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, fxIds[i], b));
    }

    setSize (460, 586);
    timerCallback();
    startTimerHz (60);
}

VocalRiserAudioProcessorEditor::~VocalRiserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

juce::String VocalRiserAudioProcessorEditor::statusText() const
{
    switch (proc.getCaptureState())
    {
        case VocalRiserAudioProcessor::stateArmed:     return "IN ASCOLTO";
        case VocalRiserAudioProcessor::stateRecording: return "CATTURA IN CORSO";
        default: break;
    }
    if (! proc.hasRender())            return "PREMI CATTURA, POI PLAY";
    if (proc.previewPlaying.load())    return "LOOP";
    return juce::CharPointer_UTF8 ("TRASCINA NELLA SESSIONE \xe2\x86\x97");
}

//==============================================================================
void VocalRiserAudioProcessorEditor::paint (juce::Graphics& g)
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
    engravedText (g, "VOCAL RISER", titleR, juce::Justification::centredLeft, Colours::text);

    g.setFont (labelFont (8.5f, 0.16f));
    engravedText (g, statusText(), { d.getCentreX() - 60.0f, d.getY() + 36.0f, 120.0f, 12.0f },
                  juce::Justification::centred, proc.previewPlaying.load() ? Colours::accent2 : Colours::dim);

    // Alloggiamento morbido della manopola, dentro l'incavo
    const auto kc = knobCentre;
    const float kr = (float) knobArea.getWidth() * 0.5f + 12.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.4f), kc.x, kc.y - kr,
                                             juce::Colours::white.withAlpha (0.03f), kc.x, kc.y + kr, false));
    g.fillEllipse (kc.x - kr, kc.y - kr, kr * 2.0f, kr * 2.0f);

    // Durante il loop, un sottile anello di avanzamento lungo il bordo dell'incavo
    if (proc.previewPlaying.load())
    {
        const float pr = notchRadius - 5.0f;
        const float a0 = -juce::MathConstants<float>::halfPi * 1.0f, span = juce::MathConstants<float>::pi;
        juce::Path ring;
        ring.addCentredArc (kc.x, kc.y, pr, pr, 0.0f, a0, a0 + span * proc.previewProgress.load(), true);
        g.setColour (Colours::accent.withAlpha (0.35f));
        g.strokePath (ring, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (Colours::accent2);
        g.strokePath (ring, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Valore intensita
    // sotto l'alloggiamento della manopola, impaginato come gli altri selettori
    {
        auto lab = intensityArea.toFloat();
        g.setFont (labelFont (8.5f, 0.2f));
        engravedText (g, juce::CharPointer_UTF8 ("INTENSIT\xc3\x80"), lab.removeFromTop (12.0f),
                      juce::Justification::centred, Colours::dim);
        g.setFont (labelFont (12.5f, 0.12f));
        engravedText (g, juce::String (juce::roundToInt (amountKnob.getValue() * 100.0)) + " %", lab,
                      juce::Justification::centred, Colours::text);
    }

    // Etichetta sopra STEREO, come gli altri selettori
    const auto sb = stereoBtn.getBounds().toFloat();
    g.setFont (labelFont (8.5f, 0.2f));
    const int src = proc.getSourceStereo();
    engravedText (g, src == 1 ? "SORGENTE STEREO" : "IMMAGINE", { sb.getX() - 20.0f, sb.getY() - 12.0f, sb.getWidth() + 40.0f, 12.0f },
                  juce::Justification::centred, Colours::dim);

    // Separatore sottile sopra agli effetti
    const float sepY = (float) fxBtns[0].getY() - 8.0f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (shellR.getX() + 40.0f, sepY, shellR.getWidth() - 80.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (shellR.getX() + 40.0f, sepY + 1.0f, shellR.getWidth() - 80.0f, 1.0f);

    // Firma dello sviluppatore
    {
        const auto sigFont = labelFont (8.0f, 0.35f);
        const juce::String sig ("CHEF CATTA DSP");
        const float tw = juce::GlyphArrangement::getStringWidth (sigFont, sig);
        const float logo = 16.0f, gap = 7.0f;
        const float x0 = shellR.getCentreX() - (logo + gap + tw) * 0.5f;
        const float cy = shellR.getBottom() - 15.0f;
        drawChefLogo (g, { x0, cy - logo * 0.5f - 1.0f, logo, logo }, Colours::dim, Colours::dim);
        g.setFont (sigFont);
        engravedText (g, sig, { x0 + logo + gap, cy - 6.0f, tw + 4.0f, 12.0f },
                      juce::Justification::centredLeft, Colours::dim);
    }
}

void VocalRiserAudioProcessorEditor::resized()
{
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
    const int sw = sel.getWidth() / 4;
    lengthSel.setBounds (sel.removeFromLeft (sw).reduced (2, 0));
    stereoBtn.setBounds (sel.removeFromLeft (sw).reduced (2, 0).withTrimmedTop (12));
    reverbSel.setBounds (sel.removeFromLeft (sw).reduced (2, 0));
    timingSel.setBounds (sel.reduced (2, 0));

    y += 40 + 18;
    auto fx = juce::Rectangle<int> (shell.getX() + 30, y, shell.getWidth() - 60, 32);
    const int fw = fx.getWidth() / 4;
    for (auto& b : fxBtns) b.setBounds (fx.removeFromLeft (fw));
}

void VocalRiserAudioProcessorEditor::timerCallback()
{
    if (proc.updateRender())
        wave.repaint();

    const bool playingPreview = proc.previewPlaying.load();
    if (playingPreview || playingPreview != lastPreviewPlaying)
        wave.repaint();

    const auto st = statusText();
    if (st != lastStatus) { lastStatus = st; repaint (displayArea.withHeight (52)); }

    saveBtn.setEnabled (proc.hasRender());
    timingSel.setEnabled (fxBtns[1].getToggleState());

    if (playingPreview || playingPreview != lastPreviewPlaying)
        repaint (knobArea.expanded (40));
    lastPreviewPlaying = playingPreview;
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
