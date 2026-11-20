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

#include <stdint.h>

enum {
    APLUS_MOD_SHIFT = 1 << 0,
    APLUS_MOD_CTRL = 1 << 1,
    APLUS_MOD_ALT = 1 << 2,
    APLUS_MOD_META = 1 << 3,
    APLUS_MOD_CAPSLOCK = 1 << 4,
};

enum {
    APLUS_BUTTON_LEFT = 1 << 0,
    APLUS_BUTTON_RIGHT = 1 << 1,
    APLUS_BUTTON_MIDDLE = 1 << 2,
};

enum AplusPointerKind {
    APLUS_POINTER_MOVE,
    APLUS_POINTER_PRESS,
    APLUS_POINTER_RELEASE,
};

struct AplusKeyEvent {
    uint16_t vkey;
    bool down;
    uint8_t modifiers;
    char text[8];
    double timestamp;
};

struct AplusPointerEvent {
    AplusPointerKind kind;
    int x;
    int y;
    uint8_t buttons;
    uint8_t button;
    uint8_t modifiers;
    int clickCount;
    double timestamp;
};

struct AplusScrollEvent {
    int x;
    int y;
    int dx;
    int dy;
    uint8_t modifiers;
    double timestamp;
};
