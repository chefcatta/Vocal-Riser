#include "PluginEditor.h"

namespace Colours
{
    const juce::Colour bg      { 0xff141518 };
    const juce::Colour panelTop{ 0xff2b2d33 };
    const juce::Colour panelBot{ 0xff1b1c20 };
    const juce::Colour key     { 0xff26282e };
    const juce::Colour outline { 0xff34363d };
    const juce::Colour accent  { 0xff2a6bff };   // blu
    const juce::Colour accent2 { 0xff63c8ff };   // azzurro
    const juce::Colour text    { 0xffe6e8ee };
    const juce::Colour dim     { 0xff80858f };
}

namespace
{
    // Doppia ombra "soft UI": scura in basso a destra, luce tenue in alto a sinistra
    void softRaise (juce::Graphics& g, const juce::Path& shape, float depth)
    {
        juce::DropShadow (juce::Colours::black.withAlpha (0.75f), juce::roundToInt (depth * 2.4f),
                          { juce::roundToInt (depth * 0.5f), juce::roundToInt (depth) }).drawForPath (g, shape);
        juce::DropShadow (juce::Colours::white.withAlpha (0.045f), juce::roundToInt (depth * 1.6f),
                          { -juce::roundToInt (depth * 0.4f), -juce::roundToInt (depth * 0.6f) }).drawForPath (g, shape);
    }

    // Area scavata: ombra interna in alto, riflesso tenue in basso
    void softInset (juce::Graphics& g, juce::Rectangle<float> r, float corner, float depth,
                    juce::Colour fill = juce::Colour (0xff111215))
    {
        juce::Path p;
        p.addRoundedRectangle (r, corner);
        g.setColour (fill);
        g.fillPath (p);

        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (p);

        // ombra: si disegna l'ombra di un "anello" esterno, che cade dentro l'area
        juce::Path ring;
        ring.addRectangle (r.expanded (40.0f));
        ring.addRoundedRectangle (r, corner);
        ring.setUsingNonZeroWinding (false);
        juce::DropShadow (juce::Colours::black.withAlpha (0.9f), juce::roundToInt (depth * 1.6f),
                          { juce::roundToInt (depth * 0.3f), juce::roundToInt (depth * 0.8f) }).drawForPath (g, ring);
        juce::DropShadow (juce::Colours::white.withAlpha (0.05f), juce::roundToInt (depth),
                          { 0, -juce::roundToInt (depth * 0.5f) }).drawForPath (g, ring);
    }

    void drawIcon (juce::Graphics& g, const juce::String& icon, juce::Rectangle<float> r, juce::Colour c)
    {
        g.setColour (c);
        juce::Path p;
        if (icon == "rec")
        {
            g.fillEllipse (r.reduced (r.getWidth() * 0.15f));
            return;
        }
        if (icon == "play")
        {
            p.addTriangle (r.getX() + 1.0f, r.getY(), r.getX() + 1.0f, r.getBottom(), r.getRight(), r.getCentreY());
            g.fillPath (p);
            return;
        }
        if (icon == "stop")
        {
            g.fillRoundedRectangle (r.reduced (1.0f), 1.5f);
            return;
        }
        if (icon == "loop")
        {
            const auto ctr = r.getCentre();
            const float rad = r.getWidth() * 0.45f;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.5f, juce::MathConstants<float>::twoPi - 0.3f, true);
            g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            juce::Path head;
            const auto tip = ctr.getPointOnCircumference (rad, 0.5f);
            head.addTriangle (tip.x - 3.0f, tip.y - 3.5f, tip.x + 3.5f, tip.y - 0.5f, tip.x - 2.0f, tip.y + 3.0f);
            g.fillPath (head);
            return;
        }
        if (icon == "save")
        {
            p.startNewSubPath (r.getCentreX(), r.getY());
            p.lineTo (r.getCentreX(), r.getBottom() - 4.0f);
            p.startNewSubPath (r.getCentreX() - 4.0f, r.getBottom() - 8.0f);
            p.lineTo (r.getCentreX(), r.getBottom() - 4.0f);
            p.lineTo (r.getCentreX() + 4.0f, r.getBottom() - 8.0f);
            p.startNewSubPath (r.getX(), r.getBottom());
            p.lineTo (r.getRight(), r.getBottom());
            g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

//==============================================================================
RiserLookAndFeel::RiserLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::textColourOffId, Colours::dim);
    setColour (juce::TextButton::textColourOnId, Colours::accent2);
}

void RiserLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const auto  bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c      = bounds.getCentre();
    const float angle  = startAngle + pos * (endAngle - startAngle);

    // Tacche della scala (piccole, incise nel pannello)
    constexpr int ticks = 41;
    for (int i = 0; i < ticks; ++i)
    {
        const float t  = (float) i / (float) (ticks - 1);
        const float a  = startAngle + t * (endAngle - startAngle);
        const bool  major = (i % 10) == 0;
        const auto  pt = c.getPointOnCircumference (radius - 3.0f, a);
        const float s  = major ? 2.4f : 1.5f;

        if (t <= pos)
        {
            g.setColour (Colours::accent2.withAlpha (0.25f));
            g.fillEllipse (pt.x - s * 2.2f, pt.y - s * 2.2f, s * 4.4f, s * 4.4f);
            g.setColour (Colours::accent.interpolatedWith (Colours::accent2, t));
        }
        else
        {
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillEllipse (pt.x - s, pt.y - s + 1.0f, s * 2.0f, s * 2.0f);
            g.setColour (Colours::outline.brighter (0.15f));
        }
        g.fillEllipse (pt.x - s, pt.y - s, s * 2.0f, s * 2.0f);
    }

    // Anello di luce blu sotto alla manopola (proporzionale al valore)
    const float r = radius * 0.80f;
    if (pos > 0.0f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r + 7.0f, r + 7.0f, 0.0f, startAngle, angle, true);
        g.setColour (Colours::accent.withAlpha (0.18f));
        g.strokePath (arc, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (Colours::accent2.withAlpha (0.55f));
        g.strokePath (arc, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Manopola: grande disco opaco che "galleggia" sul pannello
    juce::Path body;
    body.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.85f), juce::roundToInt (r * 0.45f),
                      { juce::roundToInt (r * 0.08f), juce::roundToInt (r * 0.22f) }).drawForPath (g, body);
    juce::DropShadow (juce::Colours::white.withAlpha (0.05f), juce::roundToInt (r * 0.25f),
                      { -juce::roundToInt (r * 0.06f), -juce::roundToInt (r * 0.1f) }).drawForPath (g, body);

    // Lato (bordo smussato)
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5d616b), c.x - r * 0.4f, c.y - r,
                                             juce::Colour (0xff0a0b0d), c.x + r * 0.4f, c.y + r, false));
    g.fillPath (body);

    // Faccia superiore, leggermente concava
    const float f = r * 0.93f;
    juce::ColourGradient face (juce::Colour (0xff17181b), c.x, c.y - f,
                               juce::Colour (0xff33353c), c.x, c.y + f, false);
    g.setGradientFill (face);
    g.fillEllipse (c.x - f, c.y - f, f * 2.0f, f * 2.0f);

    // Luce morbida dall'alto
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.07f), c.x, c.y - f,
                                juce::Colours::transparentWhite, c.x, c.y, false);
    g.setGradientFill (sheen);
    g.fillEllipse (c.x - f, c.y - f, f * 2.0f, f * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (c.x - f, c.y - f, f * 2.0f, f * 2.0f, 1.2f);

    // Riflesso sul bordo inferiore della faccia (effetto conca)
    juce::Path rim;
    rim.addCentredArc (c.x, c.y, f - 1.5f, f - 1.5f, 0.0f, juce::MathConstants<float>::pi * 0.6f,
                       juce::MathConstants<float>::pi * 1.4f, true);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.strokePath (rim, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Indicatore: tacca chiara incisa vicino al bordo
    const auto p1 = c.getPointOnCircumference (f * 0.68f, angle);
    const auto p2 = c.getPointOnCircumference (f * 0.88f, angle);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawLine (juce::Line<float> (p1, p2).withShortenedStart (-0.5f).withShortenedEnd (-0.5f), 5.0f);
    g.setColour (Colours::text);
    g.drawLine ({ p1, p2 }, 2.6f);
}

void RiserLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                             bool highlighted, bool down)
{
    const auto style = b.getProperties()["style"].toString();
    auto r = b.getLocalBounds().toFloat().reduced (3.0f, 4.0f);
    const bool on = b.getToggleState();

    // Selettore incassato: solo il valore scelto è in rilievo
    if (style == "seg")
    {
        if (! on) return;
        r = b.getLocalBounds().toFloat().reduced (3.0f);
        juce::Path p;
        p.addRoundedRectangle (r, 6.0f);
        softRaise (g, p, 2.5f);
        g.setGradientFill (juce::ColourGradient (Colours::panelTop.brighter (0.08f), 0.0f, r.getY(),
                                                 Colours::key.darker (0.15f), 0.0f, r.getBottom(), false));
        g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.strokePath (p, juce::PathStrokeType (1.0f));
        return;
    }

    const float corner = 8.0f;
    juce::Path p;
    p.addRoundedRectangle (r, corner);

    const bool sunk = down || on;
    if (sunk)
    {
        // Tasto premuto dentro al pannello
        softInset (g, r, corner, 5.0f, on ? juce::Colour (0xff12161f) : Colours::key.darker (0.35f));
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (Colours::key.brighter (highlighted ? 0.12f : 0.06f), 0.0f, r.getY(),
                                                 Colours::key.darker (0.25f), 0.0f, r.getBottom(), false));
        g.fillPath (p);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.09f), 0.0f, r.getY(),
                                                 juce::Colours::transparentWhite, 0.0f, r.getCentreY(), false));
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }

    // Retroilluminazione blu dei tasti accesi
    if (on)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (p);
        g.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.32f), r.getCentreX(), r.getBottom() + 4.0f,
                                                 juce::Colours::transparentBlack, r.getCentreX(), r.getY(), true));
        g.fillRect (r);
        g.setColour (Colours::accent2.withAlpha (0.25f));
        g.drawRoundedRectangle (r.reduced (1.0f), corner - 1.0f, 1.0f);
    }

    // Spia: fessura sotto al testo che si accende di blu
    if (style == "led")
    {
        auto slot = juce::Rectangle<float> (18.0f, 3.0f).withCentre ({ r.getCentreX(), r.getBottom() - 7.0f });
        if (on)
        {
            juce::Path sp;
            sp.addRoundedRectangle (slot, 1.5f);
            juce::DropShadow (Colours::accent.withAlpha (0.9f), 10, {}).drawForPath (g, sp);
            juce::DropShadow (Colours::accent2.withAlpha (0.7f), 4, {}).drawForPath (g, sp);
            g.setColour (Colours::accent2);
            g.fillRoundedRectangle (slot, 1.5f);
        }
        else
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (slot, 1.5f);
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.drawHorizontalLine ((int) slot.getBottom(), slot.getX(), slot.getRight());
        }
    }
}

void RiserLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const auto style = b.getProperties()["style"].toString();
    const auto icon  = b.getProperties()["icon"].toString();
    const bool on    = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (6.0f, 4.0f);
    if (style == "led") r.removeFromBottom (7.0f);              // spazio per la spia
    if (down || (on && style != "seg")) r.translate (0.0f, 1.0f);

    const float alpha = b.isEnabled() ? 1.0f : 0.35f;
    const auto  col   = (on ? Colours::accent2 : Colours::dim).withMultipliedAlpha (alpha);
    const auto  font  = getTextButtonFont (b, b.getHeight());
    g.setFont (font);

    const auto text = b.getButtonText();
    const float textW = juce::GlyphArrangement::getStringWidth (font, text);
    const bool  hasIcon = icon.isNotEmpty();
    const float iconW = hasIcon ? 11.0f : 0.0f, gap = hasIcon ? 7.0f : 0.0f;
    float xx = r.getCentreX() - (iconW + gap + textW) * 0.5f;

    if (hasIcon)
    {
        drawIcon (g, icon, juce::Rectangle<float> (xx, r.getCentreY() - iconW * 0.5f, iconW, iconW), col);
        xx += iconW + gap;
    }

    const auto tr = juce::Rectangle<float> (xx, r.getY(), textW + 2.0f, r.getHeight());
    if (on)
    {
        // alone del testo
        g.setColour (Colours::accent.withAlpha (0.3f * alpha));
        for (auto d : { juce::Point<float> (0.0f, 1.0f), juce::Point<float> (0.0f, -1.0f),
                        juce::Point<float> (1.0f, 0.0f), juce::Point<float> (-1.0f, 0.0f) })
            g.drawText (text, tr.translated (d.x, d.y), juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f * alpha));
        g.drawText (text, tr.translated (0.0f, 1.0f), juce::Justification::centredLeft);
    }
    g.setColour (col);
    g.drawText (text, tr, juce::Justification::centredLeft);
}

juce::Font RiserLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (12.0f, (float) buttonHeight * 0.42f), juce::Font::bold))
               .withExtraKerningFactor (0.06f);
}

//==============================================================================
void RiserWaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);

    // Schermo profondamente incassato
    softInset (g, r, 12.0f, 10.0f, juce::Colour (0xff0c0d10));

    auto inner = r.reduced (18.0f, 30.0f);
    const auto& peaks = proc.getPeaks();

    // Linea di base
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawHorizontalLine ((int) inner.getCentreY(), inner.getX(), inner.getRight());

    if (proc.hasRender() && ! peaks.empty())
    {
        // Forma d'onda a barre, con alone
        const int   bars = juce::jmax (24, (int) (inner.getWidth() / 3.0f));
        const float bw   = inner.getWidth() / (float) bars;
        const float mid  = inner.getCentreY();
        const float half = inner.getHeight() * 0.5f;
        const int   nb   = (int) peaks.size();

        juce::Path wave;
        for (int i = 0; i < bars; ++i)
        {
            const int   k0 = i * nb / bars, k1 = juce::jmax (k0 + 1, (i + 1) * nb / bars);
            float m = 0.0f;
            for (int k = k0; k < k1; ++k) m = juce::jmax (m, peaks[(size_t) k]);
            const float hh = juce::jmax (1.0f, m * half);
            wave.addRoundedRectangle (inner.getX() + (float) i * bw + bw * 0.2f, mid - hh, bw * 0.6f, hh * 2.0f, bw * 0.3f);
        }

        juce::DropShadow (Colours::accent.withAlpha (0.6f), 12, {}).drawForPath (g, wave);
        g.setGradientFill (juce::ColourGradient (Colours::accent.withAlpha (0.55f), inner.getX(), 0.0f,
                                                 Colours::accent2, inner.getRight(), 0.0f, false));
        g.fillPath (wave);

        // Playhead anteprima
        if (proc.previewPlaying.load())
        {
            const float px = inner.getX() + inner.getWidth() * proc.previewProgress.load();
            g.setColour (Colours::accent2.withAlpha (0.3f));
            g.fillRect (px - 2.0f, inner.getY() - 10.0f, 5.0f, inner.getHeight() + 20.0f);
            g.setColour (juce::Colours::white);
            g.fillRect (px, inner.getY() - 10.0f, 1.2f, inner.getHeight() + 20.0f);
        }

        // Attacco della voce
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        const float dash[] = { 3.0f, 3.0f };
        g.drawDashedLine ({ inner.getRight() + 2.0f, inner.getY() - 10.0f, inner.getRight() + 2.0f, inner.getBottom() + 10.0f },
                          dash, 2, 1.0f);

        g.setFont (juce::FontOptions (10.5f));
        g.setColour (Colours::dim);
        g.drawText (proc.getRenderInfo(), r.reduced (16, 10), juce::Justification::topLeft);
        g.setColour (Colours::accent2.withAlpha (0.85f));
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (juce::CharPointer_UTF8 ("TRASCINA NELLA SESSIONE \xe2\x86\x97"),
                    r.reduced (16, 10), juce::Justification::bottomRight);
    }

    g.setColour (Colours::text.withAlpha (0.9f));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (status, r.reduced (16, 10), juce::Justification::topRight);
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
VocalRiserAudioProcessorEditor::VocalRiserAudioProcessorEditor (VocalRiserAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), wave (p)
{
    setLookAndFeel (&lnf);
    auto& apvts = proc.apvts;

    addAndMakeVisible (wave);

    // Trasporto del riser
    captureBtn.getProperties().set ("icon", "rec");
    loopBtn.getProperties().set ("icon", "loop");
    saveBtn.getProperties().set ("icon", "save");

    captureBtn.setTooltip ("Premi CATTURA, poi play: il plugin ascolta la prossima frase");
    captureBtn.onClick = [this] { proc.armCapture(); updateStatus(); };

    // LOOP: a trasporto fermo il plugin suona l'anteprima del riser in loop
    loopBtn.setTooltip ("Con la DAW ferma suona il riser in loop");
    loopBtn.setClickingTogglesState (true);
    loopBtn.setToggleState (proc.previewLoop.load(), juce::dontSendNotification);
    loopBtn.onClick = [this] { proc.previewLoop.store (loopBtn.getToggleState()); };
    saveBtn.onClick = [this]
    {
        const auto f = proc.writeRiserFile();
        if (f.existsAsFile()) f.revealToUser();
    };

    for (auto* b : { &captureBtn, &loopBtn, &saveBtn })
        addAndMakeVisible (*b);

    // Manopola principale
    amountKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amountKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amountKnob.setDoubleClickReturnValue (true, 0.6);
    addAndMakeVisible (amountKnob);
    amountAtt = std::make_unique<SliderAtt> (apvts, "amount", amountKnob);
    amountKnob.onValueChange = [this] { repaint (knobArea.withHeight (knobArea.getHeight() + 30)); };

    // Durata: selettore incassato
    const char* lenNames[] = { "1/4", "1/2", "1", "2" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = lengthBtns[(size_t) i];
        b.setButtonText (lenNames[i]);
        b.getProperties().set ("style", "seg");
        b.setRadioGroupId (1001);
        b.setClickingTogglesState (true);
        b.onClick = [this, i]
        {
            if (lengthBtns[(size_t) i].getToggleState())
                if (auto* prm = proc.apvts.getParameter ("length"))
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
        };
        addAndMakeVisible (b);
    }

    // Effetti
    const char* fxNames[] = { "FILTRO", "STUTTER", "PHONE", "CHORUS" };
    const char* fxIds[]   = { "filterOn", "stutOn", "phoneOn", "chorusOn" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = fxBtns[(size_t) i];
        b.setButtonText (fxNames[i]);
        b.getProperties().set ("style", "led");
        b.setClickingTogglesState (true);
        addAndMakeVisible (b);
        buttonAtts.push_back (std::make_unique<ButtonAtt> (apvts, fxIds[i], b));
    }

    setSize (440, 640);
    timerCallback();
    startTimerHz (30);
}

VocalRiserAudioProcessorEditor::~VocalRiserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VocalRiserAudioProcessorEditor::updateStatus()
{
    juce::String s;
    switch (proc.getCaptureState())
    {
        case VocalRiserAudioProcessor::stateArmed:     s = "IN ASCOLTO..."; break;
        case VocalRiserAudioProcessor::stateRecording: s = "CATTURA IN CORSO"; break;
        default: s = ! proc.hasRender()            ? "PREMI CATTURA, POI PLAY"
                   : proc.previewPlaying.load()   ? "LOOP IN ANTEPRIMA"
                                                  : "PRONTO"; break;
    }
    if (s != wave.status) { wave.status = s; wave.repaint(); }
}

//==============================================================================
void VocalRiserAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::bg);

    // Corpo del plugin: blocco opaco con ombra profonda
    auto r = getLocalBounds().reduced (12).toFloat();
    juce::Path shell;
    shell.addRoundedRectangle (r, 18.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.9f), 22, { 0, 8 }).drawForPath (g, shell);

    g.setGradientFill (juce::ColourGradient (Colours::panelTop, r.getX(), r.getY(),
                                             Colours::panelBot, r.getX() + r.getWidth() * 0.3f, r.getBottom(), false));
    g.fillPath (shell);

    // Luce radiale dietro alla manopola
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.045f), (float) knobArea.getCentreX(), (float) knobArea.getY(),
                                             juce::Colours::transparentWhite, (float) knobArea.getCentreX(), (float) knobArea.getBottom() + 60.0f, true));
    g.fillPath (shell);

    // Smusso del bordo
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.12f), 0.0f, r.getY(),
                                             juce::Colours::black.withAlpha (0.6f), 0.0f, r.getBottom(), false));
    g.strokePath (shell, juce::PathStrokeType (1.2f));

    // Titolo inciso, spaziato
    auto title = juce::Rectangle<float> (34.0f, 30.0f, 220.0f, 22.0f);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)).withExtraKerningFactor (0.25f));
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawText ("VOCAL RISER", title.translated (0.0f, 1.0f), juce::Justification::centredLeft);
    g.setColour (Colours::text.withAlpha (0.92f));
    g.drawText ("VOCAL RISER", title, juce::Justification::centredLeft);

    g.setColour (Colours::accent2.withAlpha (0.3f));
    g.fillEllipse (22.0f, 37.0f, 8.0f, 8.0f);
    g.setColour (Colours::accent2);
    g.fillEllipse (24.0f, 39.0f, 4.0f, 4.0f);

    // Etichetta manopola + valore
    const auto lab = juce::Rectangle<int> (knobArea.getX(), knobArea.getBottom() + 4, knobArea.getWidth(), 18).toFloat();
    const auto txt = juce::String (juce::CharPointer_UTF8 ("INTENSIT\xc3\x80   ")) + juce::String (juce::roundToInt (amountKnob.getValue() * 100.0)) + "%";
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)).withExtraKerningFactor (0.12f));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawText (txt, lab.translated (0.0f, 1.0f), juce::Justification::centred);
    g.setColour (Colours::text.withAlpha (0.85f));
    g.drawText (txt, lab, juce::Justification::centred);

    // Ombre dei tasti in rilievo (disegnate qui per non essere tagliate)
    auto keyShadow = [&g] (juce::Button& b)
    {
        juce::Path p;
        p.addRoundedRectangle (b.getBounds().toFloat().reduced (3.0f, 4.0f), 8.0f);
        softRaise (g, p, 4.0f);
    };
    for (auto* b : { &captureBtn, &loopBtn, &saveBtn })
        if (! b->getToggleState()) keyShadow (*b);

    // Scanalatura del selettore durata
    const auto lenRow = lengthBtns[0].getBounds().getUnion (lengthBtns[3].getBounds()).toFloat();
    softInset (g, lenRow, 9.0f, 5.0f, juce::Colour (0xff141518));

    g.setColour (Colours::dim);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)).withExtraKerningFactor (0.15f));
    g.drawText ("DURATA (BAR)", lenRow.getX(), lenRow.getY() - 19.0f, lenRow.getWidth(), 14.0f, juce::Justification::centred);

    // Vassoio incassato per gli effetti
    const auto fxTray = fxBtns[0].getBounds().getUnion (fxBtns[3].getBounds()).toFloat().expanded (8.0f, 8.0f);
    softInset (g, fxTray, 12.0f, 6.0f, juce::Colour (0xff18191d));
    for (auto& b : fxBtns) if (! b.getToggleState()) keyShadow (b);
}

void VocalRiserAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (30, 26);

    r.removeFromTop (30); // titolo

    r.removeFromTop (12);
    wave.setBounds (r.removeFromTop (140));

    r.removeFromTop (12);
    auto transport = r.removeFromTop (36);
    const int tw = (transport.getWidth() - 2 * 10) / 3;
    for (auto* b : { &captureBtn, &loopBtn, &saveBtn })
    {
        b->setBounds (transport.removeFromLeft (tw));
        transport.removeFromLeft (10);
    }

    // Vassoio effetti (2 righe)
    auto fxArea = r.removeFromBottom (82).reduced (10, 6);
    auto fxRow1 = fxArea.removeFromTop (36);
    auto fxRow2 = fxArea.withTrimmedTop (fxArea.getHeight() - 36);
    const int fw = (fxRow1.getWidth() - 8) / 2;
    for (int i = 0; i < 4; ++i)
    {
        auto& row = i < 2 ? fxRow1 : fxRow2;
        fxBtns[(size_t) i].setBounds (row.removeFromLeft (fw));
        row.removeFromLeft (8);
    }

    r.removeFromBottom (18);
    auto lenRow = r.removeFromBottom (32).withSizeKeepingCentre (232, 32);
    const int lw = lenRow.getWidth() / 4;
    for (auto& b : lengthBtns)
        b.setBounds (lenRow.removeFromLeft (lw));

    r.removeFromBottom (30);
    r.removeFromTop (12);
    const int ks = juce::jmin (r.getWidth(), r.getHeight() - 20);
    knobArea = r.withSizeKeepingCentre (ks, ks).withY (r.getY());
    amountKnob.setBounds (knobArea);
}

void VocalRiserAudioProcessorEditor::timerCallback()
{
    if (proc.updateRender())
        wave.repaint();

    const bool playingPreview = proc.previewPlaying.load();
    if (playingPreview || playingPreview != lastPreviewPlaying)
        wave.repaint();
    lastPreviewPlaying = playingPreview;

    updateStatus();

    saveBtn.setEnabled (proc.hasRender());
    const bool capturing = proc.getCaptureState() != VocalRiserAudioProcessor::stateIdle;
    if (captureBtn.getToggleState() != capturing)
    {
        captureBtn.setToggleState (capturing, juce::dontSendNotification);
        repaint (captureBtn.getBounds().expanded (12));
    }
    // Ridisegna le ombre quando un tasto cambia stato
    int mask = loopBtn.getToggleState() ? 1 : 0;
    for (size_t i = 0; i < fxBtns.size(); ++i)
        if (fxBtns[i].getToggleState()) mask |= 4 << i;
    if (mask != lastToggleMask) { lastToggleMask = mask; repaint(); }

    const int lenIdx = juce::jlimit (0, 3, (int) proc.apvts.getRawParameterValue ("length")->load());
    if (! lengthBtns[(size_t) lenIdx].getToggleState())
        lengthBtns[(size_t) lenIdx].setToggleState (true, juce::dontSendNotification);
}
