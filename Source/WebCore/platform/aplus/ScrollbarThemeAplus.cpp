/*
 * Author:
 *      Antonino Natale <antonio.natale97@hotmail.com>
 *
 * Copyright (c) 2013-2026 Antonino Natale
 *
 *
 * This file is part of aplus.
 *
 * aplus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * aplus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with aplus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "ScrollbarThemeAplus.h"

#include "FloatRoundedRect.h"
#include "GraphicsContext.h"
#include "ScrollableArea.h"
#include "Scrollbar.h"

namespace WebCore {

static constexpr int thickness = 12;
static constexpr int minimumThumb = 32;

ScrollbarTheme& ScrollbarTheme::nativeTheme()
{
    static NeverDestroyed<ScrollbarThemeAplus> theme;
    return theme;
}

static Color trackColor(bool dark)
{
    return dark ? Color(SRGBA<uint8_t> { 0x24, 0x24, 0x28 }) : Color(SRGBA<uint8_t> { 0xf1, 0xf1, 0xf3 });
}

int ScrollbarThemeAplus::scrollbarThickness(ScrollbarControlSize, ScrollbarExpansionState)
{
    return thickness;
}

bool ScrollbarThemeAplus::hasThumb(Scrollbar& scrollbar)
{
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeAplus::backButtonRect(Scrollbar&, ScrollbarPart, bool)
{
    return { };
}

IntRect ScrollbarThemeAplus::forwardButtonRect(Scrollbar&, ScrollbarPart, bool)
{
    return { };
}

IntRect ScrollbarThemeAplus::trackRect(Scrollbar& scrollbar, bool)
{
    return scrollbar.frameRect();
}

int ScrollbarThemeAplus::minimumThumbLength(Scrollbar& scrollbar)
{
    int length = scrollbar.orientation() == HorizontalScrollbar ? scrollbar.width() : scrollbar.height();
    return std::min(minimumThumb, length / 2);
}

void ScrollbarThemeAplus::paintScrollbarBackground(GraphicsContext& context, Scrollbar& scrollbar)
{
    context.fillRect(scrollbar.frameRect(), trackColor(scrollbar.scrollableArea().useDarkAppearanceForScrollbars()));
}

void ScrollbarThemeAplus::paintThumb(GraphicsContext& context, Scrollbar& scrollbar, const IntRect& rect)
{
    if (rect.isEmpty()) {
        return;
    }

    bool dark = scrollbar.scrollableArea().useDarkAppearanceForScrollbars();
    bool pressed = scrollbar.pressedPart() == ThumbPart;
    bool hovered = scrollbar.hoveredPart() != NoPart;
    int inset = hovered || pressed ? 2 : 3;

    FloatRect thumb(rect);
    if (scrollbar.orientation() == HorizontalScrollbar) {
        thumb.inflateY(-inset);
        thumb.inflateX(-2);
    } else {
        thumb.inflateX(-inset);
        thumb.inflateY(-2);
    }

    if (thumb.isEmpty()) {
        return;
    }

    uint8_t alpha = pressed ? 0xb0 : hovered ? 0x88 : 0x5c;
    Color color = dark ? Color(SRGBA<uint8_t> { 0xff, 0xff, 0xff, alpha }) : Color(SRGBA<uint8_t> { 0x00, 0x00, 0x00, alpha });
    float radius = std::min(thumb.width(), thumb.height()) / 2;

    context.fillRoundedRect(FloatRoundedRect(thumb, FloatRoundedRect::Radii(radius)), color);
}

void ScrollbarThemeAplus::paintScrollCorner(ScrollableArea& area, GraphicsContext& context, const IntRect& rect)
{
    context.fillRect(rect, trackColor(area.useDarkAppearanceForScrollbars()));
}

}
