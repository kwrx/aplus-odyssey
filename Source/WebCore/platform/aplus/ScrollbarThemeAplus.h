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

#pragma once

#include "ScrollbarThemeComposite.h"

namespace WebCore {

class ScrollbarThemeAplus final : public ScrollbarThemeComposite {
public:
    int scrollbarThickness(ScrollbarControlSize = ScrollbarControlSize::Regular, ScrollbarExpansionState = ScrollbarExpansionState::Expanded) override;
    bool invalidateOnMouseEnterExit() override { return true; }

    bool hasButtons(Scrollbar&) override { return false; }
    bool hasThumb(Scrollbar&) override;

    IntRect backButtonRect(Scrollbar&, ScrollbarPart, bool painting = false) override;
    IntRect forwardButtonRect(Scrollbar&, ScrollbarPart, bool painting = false) override;
    IntRect trackRect(Scrollbar&, bool painting = false) override;
    int minimumThumbLength(Scrollbar&) override;

    void paintScrollbarBackground(GraphicsContext&, Scrollbar&) override;
    void paintThumb(GraphicsContext&, Scrollbar&, const IntRect&) override;
    void paintScrollCorner(ScrollableArea&, GraphicsContext&, const IntRect&) override;
};

}
