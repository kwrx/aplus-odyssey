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
#include "PlatformMouseEvent.h"

#include <wtf/WallTime.h>

namespace WebCore {

static OptionSet<PlatformEvent::Modifier> modifiersFromAplus(uint8_t modifiers)
{
    OptionSet<PlatformEvent::Modifier> result;

    if (modifiers & APLUS_MOD_SHIFT) {
        result.add(PlatformEvent::Modifier::ShiftKey);
    }
    if (modifiers & APLUS_MOD_CTRL) {
        result.add(PlatformEvent::Modifier::ControlKey);
    }
    if (modifiers & APLUS_MOD_ALT) {
        result.add(PlatformEvent::Modifier::AltKey);
    }
    if (modifiers & APLUS_MOD_META) {
        result.add(PlatformEvent::Modifier::MetaKey);
    }
    if (modifiers & APLUS_MOD_CAPSLOCK) {
        result.add(PlatformEvent::Modifier::CapsLockKey);
    }

    return result;
}

static MouseButton buttonFromAplus(uint8_t button)
{
    if (button & APLUS_BUTTON_LEFT) {
        return LeftButton;
    }
    if (button & APLUS_BUTTON_MIDDLE) {
        return MiddleButton;
    }
    if (button & APLUS_BUTTON_RIGHT) {
        return RightButton;
    }

    return NoButton;
}

PlatformMouseEvent::PlatformMouseEvent(BalEventButton* event)
{
    m_timestamp = WallTime::now();
    m_position = IntPoint(event->x, event->y);
    m_globalPosition = IntPoint(event->x, event->y);
    m_modifiers = modifiersFromAplus(event->modifiers);
    m_clickCount = 0;

    switch (event->kind) {
    case APLUS_POINTER_PRESS:
        m_type = PlatformEvent::MousePressed;
        m_button = buttonFromAplus(event->button);
        m_clickCount = event->clickCount;
        break;
    case APLUS_POINTER_RELEASE:
        m_type = PlatformEvent::MouseReleased;
        m_button = buttonFromAplus(event->button);
        m_clickCount = event->clickCount;
        break;
    default:
        m_type = PlatformEvent::MouseMoved;
        m_button = buttonFromAplus(event->buttons);
        break;
    }

    if (event->buttons & APLUS_BUTTON_LEFT) {
        m_buttons |= 1;
    }
    if (event->buttons & APLUS_BUTTON_RIGHT) {
        m_buttons |= 2;
    }
    if (event->buttons & APLUS_BUTTON_MIDDLE) {
        m_buttons |= 4;
    }
}

}
