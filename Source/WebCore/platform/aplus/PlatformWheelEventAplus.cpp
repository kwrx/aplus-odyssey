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
#include "PlatformWheelEvent.h"

#include "Scrollbar.h"
#include <wtf/WallTime.h>

namespace WebCore {

PlatformWheelEvent::PlatformWheelEvent(BalEventScroll* event)
{
    m_type = PlatformEvent::Wheel;
    m_timestamp = WallTime::now();
    m_position = IntPoint(event->x, event->y);
    m_globalPosition = IntPoint(event->x, event->y);
    m_useLatchedEventNode = false;

    if (event->modifiers & APLUS_MOD_SHIFT) {
        m_modifiers.add(PlatformEvent::Modifier::ShiftKey);
    }
    if (event->modifiers & APLUS_MOD_CTRL) {
        m_modifiers.add(PlatformEvent::Modifier::ControlKey);
    }
    if (event->modifiers & APLUS_MOD_ALT) {
        m_modifiers.add(PlatformEvent::Modifier::AltKey);
    }
    if (event->modifiers & APLUS_MOD_META) {
        m_modifiers.add(PlatformEvent::Modifier::MetaKey);
    }

    float ticksX = event->dx;
    float ticksY = event->dy;

    if (shiftKey() && !ticksX) {
        ticksX = ticksY;
        ticksY = 0;
    }

    m_wheelTicksX = ticksX;
    m_wheelTicksY = ticksY;
    m_granularity = ScrollByPixelWheelEvent;
    m_deltaX = ticksX * static_cast<float>(Scrollbar::pixelsPerLineStep());
    m_deltaY = ticksY * static_cast<float>(Scrollbar::pixelsPerLineStep());
}

}
