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
#include "PlatformKeyboardEvent.h"

#include "WindowsKeyboardCodes.h"
#include <wtf/HexNumber.h>
#include <wtf/WallTime.h>
#include <wtf/text/StringConcatenateNumbers.h>

namespace WebCore {

struct AplusKeyInfo {
    uint16_t vkey;
    int windowsKeyCode;
    const char* code;
    const char* key;
    const char* identifier;
    bool keypad;
};

static const AplusKeyInfo aplusKeys[] = {
    { 1, VK_ESCAPE, "Escape", "Escape", "U+001B", false },
    { 2, VK_1, "Digit1", nullptr, nullptr, false },
    { 3, VK_2, "Digit2", nullptr, nullptr, false },
    { 4, VK_3, "Digit3", nullptr, nullptr, false },
    { 5, VK_4, "Digit4", nullptr, nullptr, false },
    { 6, VK_5, "Digit5", nullptr, nullptr, false },
    { 7, VK_6, "Digit6", nullptr, nullptr, false },
    { 8, VK_7, "Digit7", nullptr, nullptr, false },
    { 9, VK_8, "Digit8", nullptr, nullptr, false },
    { 10, VK_9, "Digit9", nullptr, nullptr, false },
    { 11, VK_0, "Digit0", nullptr, nullptr, false },
    { 12, VK_OEM_MINUS, "Minus", nullptr, nullptr, false },
    { 13, VK_OEM_PLUS, "Equal", nullptr, nullptr, false },
    { 14, VK_BACK, "Backspace", "Backspace", "U+0008", false },
    { 15, VK_TAB, "Tab", "Tab", "U+0009", false },
    { 16, VK_Q, "KeyQ", nullptr, nullptr, false },
    { 17, VK_W, "KeyW", nullptr, nullptr, false },
    { 18, VK_E, "KeyE", nullptr, nullptr, false },
    { 19, VK_R, "KeyR", nullptr, nullptr, false },
    { 20, VK_T, "KeyT", nullptr, nullptr, false },
    { 21, VK_Y, "KeyY", nullptr, nullptr, false },
    { 22, VK_U, "KeyU", nullptr, nullptr, false },
    { 23, VK_I, "KeyI", nullptr, nullptr, false },
    { 24, VK_O, "KeyO", nullptr, nullptr, false },
    { 25, VK_P, "KeyP", nullptr, nullptr, false },
    { 26, VK_OEM_4, "BracketLeft", nullptr, nullptr, false },
    { 27, VK_OEM_6, "BracketRight", nullptr, nullptr, false },
    { 28, VK_RETURN, "Enter", "Enter", "Enter", false },
    { 29, VK_CONTROL, "ControlLeft", "Control", "Control", false },
    { 30, VK_A, "KeyA", nullptr, nullptr, false },
    { 31, VK_S, "KeyS", nullptr, nullptr, false },
    { 32, VK_D, "KeyD", nullptr, nullptr, false },
    { 33, VK_F, "KeyF", nullptr, nullptr, false },
    { 34, VK_G, "KeyG", nullptr, nullptr, false },
    { 35, VK_H, "KeyH", nullptr, nullptr, false },
    { 36, VK_J, "KeyJ", nullptr, nullptr, false },
    { 37, VK_K, "KeyK", nullptr, nullptr, false },
    { 38, VK_L, "KeyL", nullptr, nullptr, false },
    { 39, VK_OEM_1, "Semicolon", nullptr, nullptr, false },
    { 40, VK_OEM_7, "Quote", nullptr, nullptr, false },
    { 41, VK_OEM_3, "Backquote", nullptr, nullptr, false },
    { 42, VK_SHIFT, "ShiftLeft", "Shift", "Shift", false },
    { 43, VK_OEM_5, "Backslash", nullptr, nullptr, false },
    { 44, VK_Z, "KeyZ", nullptr, nullptr, false },
    { 45, VK_X, "KeyX", nullptr, nullptr, false },
    { 46, VK_C, "KeyC", nullptr, nullptr, false },
    { 47, VK_V, "KeyV", nullptr, nullptr, false },
    { 48, VK_B, "KeyB", nullptr, nullptr, false },
    { 49, VK_N, "KeyN", nullptr, nullptr, false },
    { 50, VK_M, "KeyM", nullptr, nullptr, false },
    { 51, VK_OEM_COMMA, "Comma", nullptr, nullptr, false },
    { 52, VK_OEM_PERIOD, "Period", nullptr, nullptr, false },
    { 53, VK_OEM_2, "Slash", nullptr, nullptr, false },
    { 54, VK_SHIFT, "ShiftRight", "Shift", "Shift", false },
    { 55, VK_MULTIPLY, "NumpadMultiply", nullptr, nullptr, true },
    { 56, VK_MENU, "AltLeft", "Alt", "Alt", false },
    { 57, VK_SPACE, "Space", nullptr, "U+0020", false },
    { 58, VK_CAPITAL, "CapsLock", "CapsLock", "CapsLock", false },
    { 59, VK_F1, "F1", "F1", "F1", false },
    { 60, VK_F2, "F2", "F2", "F2", false },
    { 61, VK_F3, "F3", "F3", "F3", false },
    { 62, VK_F4, "F4", "F4", "F4", false },
    { 63, VK_F5, "F5", "F5", "F5", false },
    { 64, VK_F6, "F6", "F6", "F6", false },
    { 65, VK_F7, "F7", "F7", "F7", false },
    { 66, VK_F8, "F8", "F8", "F8", false },
    { 67, VK_F9, "F9", "F9", "F9", false },
    { 68, VK_F10, "F10", "F10", "F10", false },
    { 69, VK_NUMLOCK, "NumLock", "NumLock", "NumLock", false },
    { 70, VK_SCROLL, "ScrollLock", "ScrollLock", "Scroll", false },
    { 71, VK_NUMPAD7, "Numpad7", nullptr, nullptr, true },
    { 72, VK_NUMPAD8, "Numpad8", nullptr, nullptr, true },
    { 73, VK_NUMPAD9, "Numpad9", nullptr, nullptr, true },
    { 74, VK_SUBTRACT, "NumpadSubtract", nullptr, nullptr, true },
    { 75, VK_NUMPAD4, "Numpad4", nullptr, nullptr, true },
    { 76, VK_NUMPAD5, "Numpad5", nullptr, nullptr, true },
    { 77, VK_NUMPAD6, "Numpad6", nullptr, nullptr, true },
    { 78, VK_ADD, "NumpadAdd", nullptr, nullptr, true },
    { 79, VK_NUMPAD1, "Numpad1", nullptr, nullptr, true },
    { 80, VK_NUMPAD2, "Numpad2", nullptr, nullptr, true },
    { 81, VK_NUMPAD3, "Numpad3", nullptr, nullptr, true },
    { 82, VK_NUMPAD0, "Numpad0", nullptr, nullptr, true },
    { 83, VK_DECIMAL, "NumpadDecimal", nullptr, nullptr, true },
    { 86, VK_OEM_102, "IntlBackslash", nullptr, nullptr, false },
    { 87, VK_F11, "F11", "F11", "F11", false },
    { 88, VK_F12, "F12", "F12", "F12", false },
    { 96, VK_RETURN, "NumpadEnter", "Enter", "Enter", true },
    { 97, VK_CONTROL, "ControlRight", "Control", "Control", false },
    { 98, VK_DIVIDE, "NumpadDivide", nullptr, nullptr, true },
    { 99, VK_SNAPSHOT, "PrintScreen", "PrintScreen", "PrintScreen", false },
    { 100, VK_MENU, "AltRight", "AltGraph", "Alt", false },
    { 102, VK_HOME, "Home", "Home", "Home", false },
    { 103, VK_UP, "ArrowUp", "ArrowUp", "Up", false },
    { 104, VK_PRIOR, "PageUp", "PageUp", "PageUp", false },
    { 105, VK_LEFT, "ArrowLeft", "ArrowLeft", "Left", false },
    { 106, VK_RIGHT, "ArrowRight", "ArrowRight", "Right", false },
    { 107, VK_END, "End", "End", "End", false },
    { 108, VK_DOWN, "ArrowDown", "ArrowDown", "Down", false },
    { 109, VK_NEXT, "PageDown", "PageDown", "PageDown", false },
    { 110, VK_INSERT, "Insert", "Insert", "Insert", false },
    { 111, VK_DELETE, "Delete", "Delete", "U+007F", false },
    { 119, VK_PAUSE, "Pause", "Pause", "Pause", false },
    { 125, VK_LWIN, "MetaLeft", "Meta", "Meta", false },
    { 126, VK_RWIN, "MetaRight", "Meta", "Meta", false },
    { 127, VK_APPS, "ContextMenu", "ContextMenu", "Apps", false },
};

static const AplusKeyInfo* aplusKeyInfo(uint16_t vkey)
{
    for (auto& info : aplusKeys) {
        if (info.vkey == vkey) {
            return &info;
        }
    }

    return nullptr;
}

static bool s_shiftKey;
static bool s_ctrlKey;
static bool s_altKey;
static bool s_metaKey;
static bool s_capsLock;

PlatformKeyboardEvent::PlatformKeyboardEvent(BalEventKey* event)
    : PlatformEvent(event->down ? PlatformEvent::KeyDown : PlatformEvent::KeyUp, OptionSet<PlatformEvent::Modifier>(), WallTime::now())
    , m_balEventKey(event)
{
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
    if (event->modifiers & APLUS_MOD_CAPSLOCK) {
        m_modifiers.add(PlatformEvent::Modifier::CapsLockKey);
    }

    String text = String::fromUTF8(event->text);
    if (text.length() == 1 && text[0] < 0x20 && text[0] != '\r' && text[0] != '\t' && text[0] != 0x1b && text[0] != 0x08) {
        text = String();
    }

    const AplusKeyInfo* info = aplusKeyInfo(event->vkey);

    m_text = text;
    m_unmodifiedText = text;
    m_windowsVirtualKeyCode = info ? info->windowsKeyCode : 0;
    m_isKeypad = info && info->keypad;
    m_code = info ? String(info->code) : String("Unidentified"_s);

    if (info && info->key) {
        m_key = String(info->key);
    } else if (!text.isEmpty()) {
        m_key = text;
    } else {
        m_key = "Unidentified"_s;
    }

    if (info && info->identifier) {
        m_keyIdentifier = String(info->identifier);
    } else if (!text.isEmpty()) {
        UChar c = u_toupper(text[0]);
        m_keyIdentifier = makeString("U+", hex(c, 4));
    } else {
        m_keyIdentifier = "Unidentified"_s;
    }

    if (m_windowsVirtualKeyCode == VK_RETURN) {
        m_text = "\r"_s;
        m_unmodifiedText = m_text;
    }

    s_shiftKey = shiftKey();
    s_ctrlKey = controlKey();
    s_altKey = altKey();
    s_metaKey = metaKey();
    s_capsLock = event->modifiers & APLUS_MOD_CAPSLOCK;
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool backwardCompatibilityMode)
{
    ASSERT(m_type == PlatformEvent::KeyDown);
    m_type = type;

    if (backwardCompatibilityMode) {
        return;
    }

    if (type == PlatformEvent::RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    return s_capsLock;
}

BalEventKey* PlatformKeyboardEvent::balEventKey() const
{
    return m_balEventKey;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    shiftKey = s_shiftKey;
    ctrlKey = s_ctrlKey;
    altKey = s_altKey;
    metaKey = s_metaKey;
}

}
