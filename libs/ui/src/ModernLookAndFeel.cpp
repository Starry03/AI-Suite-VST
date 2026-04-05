#include <ai_ui/ModernLookAndFeel.h>

// Color scheme moderno
const juce::Colour ModernLookAndFeel::ColorScheme::background = juce::Colour(0xff1a1a1f);
const juce::Colour ModernLookAndFeel::ColorScheme::backgroundDark = juce::Colour(0xff0f0f12);
const juce::Colour ModernLookAndFeel::ColorScheme::accent = juce::Colour(0xff4a9eff);
const juce::Colour ModernLookAndFeel::ColorScheme::accentBright = juce::Colour(0xff6bb6ff);
const juce::Colour ModernLookAndFeel::ColorScheme::text = juce::Colour(0xfff0f0f0);
const juce::Colour ModernLookAndFeel::ColorScheme::textDim = juce::Colour(0xff808080);
const juce::Colour ModernLookAndFeel::ColorScheme::gridLine = juce::Colour(0xff2a2a2f);

ModernLookAndFeel::ModernLookAndFeel()
{
    // Setup colori di base
    setColour(juce::ResizableWindow::backgroundColourId, ColorScheme::background);
    setColour(juce::Slider::thumbColourId, ColorScheme::accent);
    setColour(juce::Slider::trackColourId, ColorScheme::gridLine);
    setColour(juce::Slider::backgroundColourId, ColorScheme::backgroundDark);
    setColour(juce::TextButton::buttonColourId, ColorScheme::backgroundDark.brighter(0.1f));

    // ComboBox colors
    setColour(juce::ComboBox::backgroundColourId, ColorScheme::backgroundDark.brighter(0.08f));
    setColour(juce::ComboBox::textColourId, ColorScheme::textDim);
    setColour(juce::ComboBox::outlineColourId, ColorScheme::gridLine);
    setColour(juce::ComboBox::arrowColourId, ColorScheme::textDim);

    // PopupMenu colors
    setColour(juce::PopupMenu::backgroundColourId, ColorScheme::backgroundDark);
    setColour(juce::PopupMenu::textColourId, ColorScheme::textDim);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ColorScheme::accent.withAlpha(0.15f));
    setColour(juce::PopupMenu::highlightedTextColourId, ColorScheme::text);
}

void ModernLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    juce::ignoreUnused(slider);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    // Background arc
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                               bounds.getCentreY(),
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               rotaryEndAngle,
                               true);

    g.setColour(ColorScheme::gridLine);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc con gradient
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
                              bounds.getCentreY(),
                              arcRadius,
                              arcRadius,
                              0.0f,
                              rotaryStartAngle,
                              toAngle,
                              true);

        // Gradient per l'arc del valore
        juce::ColourGradient gradient(ColorScheme::accent, bounds.getCentreX(), bounds.getY(),
                                     ColorScheme::accentBright, bounds.getCentreX(), bounds.getBottom(),
                                     false);
        g.setGradientFill(gradient);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Indicator line from center toward arc edge
    auto indicatorLength = arcRadius - lineW;
    juce::Path indicator;
    indicator.startNewSubPath(0.0f, -indicatorLength * 0.6f);
    indicator.lineTo(0.0f, -indicatorLength);

    g.setColour(ColorScheme::accentBright);
    g.strokePath(indicator,
                 juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                 juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));
}

void ModernLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style);

    auto trackWidth = juce::jmin(6.0f, slider.isHorizontal() ? height * 0.25f : width * 0.25f);
    juce::Point<float> startPoint(slider.isHorizontal() ? x : x + width * 0.5f,
                                 slider.isHorizontal() ? y + height * 0.5f : height + y);
    juce::Point<float> endPoint(slider.isHorizontal() ? width + x : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : y);

    juce::Path backgroundTrack;
    backgroundTrack.startNewSubPath(startPoint);
    backgroundTrack.lineTo(endPoint);
    g.setColour(ColorScheme::gridLine);
    g.strokePath(backgroundTrack, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value track con gradient
    juce::Path valueTrack;
    juce::Point<float> thumbPoint(slider.isHorizontal() ? sliderPos : (x + width * 0.5f),
                                 slider.isHorizontal() ? (y + height * 0.5f) : sliderPos);

    valueTrack.startNewSubPath(startPoint);
    valueTrack.lineTo(thumbPoint);

    juce::ColourGradient gradient(ColorScheme::accent, startPoint.x, startPoint.y,
                                 ColorScheme::accentBright, thumbPoint.x, thumbPoint.y,
                                 false);
    g.setGradientFill(gradient);
    g.strokePath(valueTrack, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Thumb - simple circle, no glow
    auto thumbWidth = trackWidth * 2.0f;
    g.setColour(ColorScheme::accentBright);
    g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

void ModernLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);

    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    float cornerRadius = 4.0f;

    // Default muted/dark look
    auto baseColour = ColorScheme::backgroundDark.brighter(0.08f);

    if (!button.isEnabled())
        baseColour = baseColour.withMultipliedAlpha(0.4f);
    else if (shouldDrawButtonAsDown)
        baseColour = baseColour.brighter(0.05f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.12f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(ColorScheme::gridLine);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void ModernLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    float cornerRadius = 4.0f;
    bool isOn = button.getToggleState();

    juce::Colour bgColour;
    if (isOn)
        bgColour = ColorScheme::accent;
    else
        bgColour = ColorScheme::backgroundDark.brighter(0.15f);

    if (shouldDrawButtonAsDown)
        bgColour = bgColour.brighter(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        bgColour = bgColour.brighter(0.1f);

    if (!button.isEnabled())
        bgColour = bgColour.withMultipliedAlpha(0.4f);

    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(ColorScheme::gridLine.brighter(0.3f));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    g.setColour(isOn ? ColorScheme::text : ColorScheme::textDim);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred);
}

void ModernLookAndFeel::drawGlowEffect(juce::Graphics& g, juce::Rectangle<float> bounds,
                                      juce::Colour color, float intensity)
{
    const int glowRadius = 8;

    for (int i = glowRadius; i > 0; --i)
    {
        float alpha = (intensity / glowRadius) * (glowRadius - i + 1);
        g.setColour(color.withAlpha(alpha));
        g.drawRoundedRectangle(bounds.expanded(i), bounds.getWidth() / 2 + i, 1.0f);
    }
}

void ModernLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int buttonX, int buttonY, int buttonW, int buttonH,
                                    juce::ComboBox& box)
{
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH);

    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
    float cornerRadius = 4.0f;

    auto baseColour = ColorScheme::backgroundDark.brighter(0.08f);
    if (isButtonDown)
        baseColour = baseColour.brighter(0.05f);
    else if (box.isMouseOver())
        baseColour = baseColour.brighter(0.12f);

    if (!box.isEnabled())
        baseColour = baseColour.withMultipliedAlpha(0.4f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(ColorScheme::gridLine);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // Down arrow
    auto arrowArea = bounds.removeFromRight(height * 0.7f).reduced(6.0f);
    juce::Path arrow;
    auto cx = arrowArea.getCentreX();
    auto cy = arrowArea.getCentreY();
    auto sz = juce::jmin(arrowArea.getWidth(), arrowArea.getHeight()) * 0.3f;
    arrow.startNewSubPath(cx - sz, cy - sz * 0.4f);
    arrow.lineTo(cx, cy + sz * 0.4f);
    arrow.lineTo(cx + sz, cy - sz * 0.4f);

    g.setColour(box.isEnabled() ? ColorScheme::textDim : ColorScheme::textDim.withAlpha(0.4f));
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}

void ModernLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    g.setColour(ColorScheme::backgroundDark.brighter(0.04f));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(ColorScheme::gridLine.brighter(0.1f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void ModernLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool hasSubMenu,
                                         const juce::String& text, const juce::String& shortcutKeyText,
                                         const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused(shortcutKeyText, icon, textColour, hasSubMenu);

    if (isSeparator)
    {
        auto sepArea = area.reduced(5, 0);
        g.setColour(ColorScheme::gridLine);
        g.drawHorizontalLine(sepArea.getCentreY(), (float)sepArea.getX(), (float)sepArea.getRight());
        return;
    }

    auto itemArea = area.reduced(2, 1);

    if (isHighlighted && isActive)
    {
        g.setColour(ColorScheme::accent.withAlpha(0.15f));
        g.fillRoundedRectangle(itemArea.toFloat(), 3.0f);
    }

    // Icon area on the left
    auto iconArea = itemArea.removeFromLeft(24).toFloat().reduced(2.0f, 4.0f);
    drawFilterIcon(g, iconArea, text);

    // Tick mark for selected item
    if (isTicked)
    {
        g.setColour(ColorScheme::accent);
        auto dotArea = itemArea.removeFromLeft(4);
        g.fillRoundedRectangle(dotArea.toFloat().withSizeKeepingCentre(3.0f, 12.0f), 1.5f);
        itemArea.removeFromLeft(2);
    }
    else
    {
        itemArea.removeFromLeft(6);
    }

    // Text
    auto textColourToUse = isActive
        ? (isHighlighted ? ColorScheme::text : ColorScheme::textDim)
        : ColorScheme::textDim.withAlpha(0.4f);

    g.setColour(textColourToUse);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(text, itemArea, juce::Justification::centredLeft, true);
}

juce::Font ModernLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions(12.0f));
}

void ModernLookAndFeel::drawFilterIcon(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text)
{
    juce::Path iconPath;
    auto cx = area.getCentreX();
    auto cy = area.getCentreY();
    auto w = area.getWidth() * 0.8f;
    auto h = area.getHeight() * 0.6f;

    auto left = cx - w * 0.5f;
    auto right = cx + w * 0.5f;
    auto top = cy - h * 0.5f;
    auto bottom = cy + h * 0.5f;
    auto mid = cx;

    if (text.containsIgnoreCase("LP"))
    {
        // Low Pass: flat then smooth slope down
        iconPath.startNewSubPath(left, top);
        iconPath.lineTo(left + w * 0.4f, top);
        iconPath.cubicTo(left + w * 0.6f, top, mid, top + h * 0.2f, right, bottom);
    }
    else if (text.containsIgnoreCase("HP"))
    {
        // High Pass: classic response with hump near cutoff then flat
        iconPath.startNewSubPath(left, bottom);
        iconPath.cubicTo(mid, bottom, left + w * 0.4f, top - h * 0.15f, left + w * 0.55f, top);
        iconPath.lineTo(right, top);
    }
    else if (text.containsIgnoreCase("Bell"))
    {
        // Bell: peak curve
        iconPath.startNewSubPath(left, cy);
        iconPath.quadraticTo(left + w * 0.25f, cy, mid, top);
        iconPath.quadraticTo(right - w * 0.25f, cy, right, cy);
    }
    else if (text.containsIgnoreCase("L") && text.containsIgnoreCase("Sh"))
    {
        // Low Shelf: step on the left, flat right
        iconPath.startNewSubPath(left, top);
        iconPath.quadraticTo(mid, top, mid, cy);
        iconPath.quadraticTo(mid, bottom, right, bottom);
    }
    else if (text.containsIgnoreCase("H") && text.containsIgnoreCase("Sh"))
    {
        // High Shelf: flat left, step on the right
        iconPath.startNewSubPath(left, bottom);
        iconPath.quadraticTo(mid, bottom, mid, cy);
        iconPath.quadraticTo(mid, top, right, top);
    }
    else if (text.containsIgnoreCase("Notch"))
    {
        // Notch: V-shaped dip
        iconPath.startNewSubPath(left, top);
        iconPath.lineTo(mid - w * 0.15f, top);
        iconPath.lineTo(mid, bottom);
        iconPath.lineTo(mid + w * 0.15f, top);
        iconPath.lineTo(right, top);
    }
    else if (text.containsIgnoreCase("dB"))
    {
        // Slope indicator: diagonal line with steepness hint
        iconPath.startNewSubPath(left, top);
        iconPath.lineTo(right, bottom);
    }
    else
    {
        return;
    }

    g.setColour(ColorScheme::accent.withAlpha(0.7f));
    g.strokePath(iconPath, juce::PathStrokeType(1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
