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

#include "TextField.h"

#include "App.h"

#include <algorithm>
#include <cctype>
#include <cmath>

static constexpr uint16_t keyBackspace = 14;
static constexpr uint16_t keyEnter = 28;
static constexpr uint16_t keyEscape = 1;
static constexpr uint16_t keyA = 30;
static constexpr uint16_t keyC = 46;
static constexpr uint16_t keyX = 45;
static constexpr uint16_t keyV = 47;
static constexpr uint16_t keyHome = 102;
static constexpr uint16_t keyLeft = 105;
static constexpr uint16_t keyRight = 106;
static constexpr uint16_t keyEnd = 107;
static constexpr uint16_t keyDelete = 111;
static constexpr uint16_t keyKeypadEnter = 96;
static constexpr double blinkPeriod = 0.53;
static const char* const bullet = "\xe2\x80\xa2";

static bool isContinuation(unsigned char c)
{
    return (c & 0xC0) == 0x80;
}

static bool isWordByte(unsigned char c)
{
    return isalnum(c) || c >= 0x80;
}

void TextField::setText(const std::string& text)
{
    m_text = text;
    m_caret = m_anchor = m_text.size();
    m_scroll = 0;
}

void TextField::focus(bool all)
{
    m_focused = true;
    if (all) {
        selectAll();
    }
    touch();
}

void TextField::blur()
{
    m_focused = false;
    m_anchor = m_caret;
    m_scroll = 0;
}

void TextField::selectAll()
{
    m_anchor = 0;
    m_caret = m_text.size();
}

std::string TextField::selectedText() const
{
    size_t a = std::min(m_anchor, m_caret);
    size_t b = std::max(m_anchor, m_caret);
    return m_text.substr(a, b - a);
}

bool TextField::changed()
{
    bool c = m_changed;
    m_changed = false;
    return c;
}

void TextField::touch()
{
    m_blinkStart = App::now();
}

size_t TextField::previous(size_t index) const
{
    if (index == 0) {
        return 0;
    }
    index--;
    while (index > 0 && isContinuation(m_text[index])) {
        index--;
    }
    return index;
}

size_t TextField::next(size_t index) const
{
    if (index >= m_text.size()) {
        return m_text.size();
    }
    index++;
    while (index < m_text.size() && isContinuation(m_text[index])) {
        index++;
    }
    return index;
}

size_t TextField::previousWord(size_t index) const
{
    while (index > 0 && !isWordByte(m_text[index - 1])) {
        index--;
    }
    while (index > 0 && isWordByte(m_text[index - 1])) {
        index--;
    }
    return index;
}

size_t TextField::nextWord(size_t index) const
{
    while (index < m_text.size() && !isWordByte(m_text[index])) {
        index++;
    }
    while (index < m_text.size() && isWordByte(m_text[index])) {
        index++;
    }
    return index;
}

void TextField::move(size_t index, bool extend)
{
    m_caret = std::min(index, m_text.size());
    if (!extend) {
        m_anchor = m_caret;
    }
    touch();
}

void TextField::erase(size_t from, size_t to)
{
    if (from >= to) {
        return;
    }
    m_text.erase(from, to - from);
    m_caret = m_anchor = from;
    m_changed = true;
    touch();
}

void TextField::deleteSelection()
{
    erase(std::min(m_anchor, m_caret), std::max(m_anchor, m_caret));
}

void TextField::insert(const std::string& text)
{
    std::string clean;
    for (unsigned char c : text) {
        if (c >= 0x20 && c != 0x7f) {
            clean += (char)c;
        } else if (c == '\n' || c == '\t') {
            clean += ' ';
        }
    }

    if (clean.empty()) {
        return;
    }

    deleteSelection();
    m_text.insert(m_caret, clean);
    m_caret += clean.size();
    m_anchor = m_caret;
    m_changed = true;
    touch();
}

TextField::Result TextField::key(uint16_t vkey, bool down, const char* utf8, uint8_t modifiers)
{
    if (!down) {
        return Result::Ignored;
    }

    bool shift = modifiers & APLUS_MOD_SHIFT;
    bool ctrl = modifiers & (APLUS_MOD_CTRL | APLUS_MOD_META);

    switch (vkey) {
    case keyEnter:
    case keyKeypadEnter:
        return Result::Submit;
    case keyEscape:
        return Result::Cancel;
    case keyLeft:
        if (hasSelection() && !shift) {
            move(std::min(m_anchor, m_caret), false);
        } else {
            move(ctrl ? previousWord(m_caret) : previous(m_caret), shift);
        }
        return Result::Handled;
    case keyRight:
        if (hasSelection() && !shift) {
            move(std::max(m_anchor, m_caret), false);
        } else {
            move(ctrl ? nextWord(m_caret) : next(m_caret), shift);
        }
        return Result::Handled;
    case keyHome:
        move(0, shift);
        return Result::Handled;
    case keyEnd:
        move(m_text.size(), shift);
        return Result::Handled;
    case keyBackspace:
        if (hasSelection()) {
            deleteSelection();
        } else {
            erase(ctrl ? previousWord(m_caret) : previous(m_caret), m_caret);
        }
        return Result::Handled;
    case keyDelete:
        if (hasSelection()) {
            deleteSelection();
        } else {
            erase(m_caret, ctrl ? nextWord(m_caret) : next(m_caret));
        }
        return Result::Handled;
    default:
        break;
    }

    if (ctrl) {
        switch (vkey) {
        case keyA:
            selectAll();
            touch();
            return Result::Handled;
        case keyC:
            if (hasSelection() && !m_password) {
                App::setClipboard(selectedText());
            }
            return Result::Handled;
        case keyX:
            if (hasSelection() && !m_password) {
                App::setClipboard(selectedText());
                deleteSelection();
            }
            return Result::Handled;
        case keyV:
            insert(App::clipboard());
            return Result::Handled;
        default:
            return Result::Ignored;
        }
    }

    if (utf8 && (unsigned char)utf8[0] >= 0x20 && (unsigned char)utf8[0] != 0x7f) {
        insert(utf8);
        return Result::Handled;
    }

    return Result::Ignored;
}

std::string TextField::display() const
{
    if (!m_password) {
        return m_text;
    }

    std::string out;
    for (size_t i = 0; i < m_text.size(); i = next(i)) {
        out += bullet;
    }
    return out;
}

size_t TextField::displayOffset(size_t index) const
{
    if (!m_password) {
        return index;
    }

    size_t count = 0;
    for (size_t i = 0; i < index && i < m_text.size(); i = next(i)) {
        count++;
    }
    return count * 3;
}

double TextField::offsetToX(size_t index, double size) const
{
    return Draw::textWidth(display().substr(0, displayOffset(index)), Draw::Weight::Regular, size);
}

size_t TextField::xToIndex(double x) const
{
    double local = x - m_rect.x + m_scroll;
    size_t best = 0;
    double bestDistance = std::fabs(local);

    for (size_t i = next(0); i <= m_text.size(); i = next(i)) {
        double distance = std::fabs(offsetToX(i, m_size) - local);
        if (distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
        if (i == m_text.size()) {
            break;
        }
    }

    return best;
}

void TextField::pointerDown(double x, int clickCount)
{
    size_t index = xToIndex(x);

    if (clickCount >= 3) {
        selectAll();
    } else if (clickCount == 2) {
        size_t start = index;
        size_t end = index;
        while (start > 0 && isWordByte(m_text[start - 1])) {
            start--;
        }
        while (end < m_text.size() && isWordByte(m_text[end])) {
            end++;
        }
        m_anchor = start;
        m_caret = end;
    } else {
        move(index, false);
    }

    touch();
}

void TextField::pointerDrag(double x)
{
    move(xToIndex(x), true);
}

void TextField::draw(cairo_t* cr, const ui_rect_t& rect, double size, const ui_color_t& textColor, double now)
{
    const ui_theme_t& theme = Draw::theme();

    m_rect = rect;
    m_size = size;

    std::string shown = display();
    double caretX = offsetToX(m_caret, size);
    double width = rect.width;

    if (caretX - m_scroll > width - 2) {
        m_scroll = caretX - width + 2;
    }
    if (caretX - m_scroll < 0) {
        m_scroll = caretX;
    }

    double total = Draw::textWidth(shown, Draw::Weight::Regular, size);
    if (total - m_scroll < width - 2) {
        m_scroll = std::max(0.0, total - width + 2);
    }

    cairo_save(cr);
    cairo_rectangle(cr, rect.x, rect.y, rect.width, rect.height);
    cairo_clip(cr);

    double origin = rect.x - m_scroll;

    if (hasSelection() && m_focused) {
        double a = offsetToX(std::min(m_anchor, m_caret), size);
        double b = offsetToX(std::max(m_anchor, m_caret), size);
        double h = std::round(size * 1.5);
        Draw::roundedRect(cr, origin + a - 1, rect.y + (rect.height - h) / 2.0, b - a + 2, h, 3);
        Draw::setColor(cr, theme.primary, 0.45);
        cairo_fill(cr);
    }

    Draw::text(cr, shown, origin, rect.y, rect.height, Draw::Weight::Regular, size, textColor);

    if (m_focused && !hasSelection() && std::fmod(now - m_blinkStart, blinkPeriod * 2) < blinkPeriod) {
        double h = std::round(size * 1.3);
        Draw::roundedRect(cr, std::round(origin + caretX), rect.y + (rect.height - h) / 2.0, 2, h, 1);
        Draw::setColor(cr, theme.text);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

int TextField::timeout(double now) const
{
    if (!m_focused || hasSelection()) {
        return -1;
    }

    double phase = std::fmod(now - m_blinkStart, blinkPeriod);
    return std::max(1, (int)std::ceil((blinkPeriod - phase) * 1000.0));
}
