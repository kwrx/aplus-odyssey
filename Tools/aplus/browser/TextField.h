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

#include "Draw.h"

#include <cstdint>
#include <string>

class TextField {
public:
    enum class Result {
        Ignored,
        Handled,
        Submit,
        Cancel,
    };

    const std::string& text() const { return m_text; }
    void setText(const std::string&);
    void setPassword(bool password) { m_password = password; }
    bool password() const { return m_password; }

    bool focused() const { return m_focused; }
    void focus(bool selectAll);
    void blur();
    void selectAll();
    bool hasSelection() const { return m_anchor != m_caret; }
    std::string selectedText() const;

    Result key(uint16_t vkey, bool down, const char* utf8, uint8_t modifiers);

    void setRect(const ui_rect_t& rect, double size)
    {
        m_rect = rect;
        m_size = size;
    }
    void pointerDown(double x, int clickCount);
    void pointerDrag(double x);

    void draw(cairo_t*, const ui_rect_t&, double size, const ui_color_t& textColor, double now);
    int timeout(double now) const;
    bool changed();

private:
    std::string display() const;
    size_t displayOffset(size_t index) const;
    double offsetToX(size_t index, double size) const;
    size_t xToIndex(double x) const;
    size_t previous(size_t index) const;
    size_t next(size_t index) const;
    size_t previousWord(size_t index) const;
    size_t nextWord(size_t index) const;
    void move(size_t index, bool extend);
    void erase(size_t from, size_t to);
    void insert(const std::string&);
    void deleteSelection();
    void touch();

    std::string m_text;
    size_t m_caret { 0 };
    size_t m_anchor { 0 };
    double m_scroll { 0 };
    double m_size { 14 };
    ui_rect_t m_rect { 0, 0, 0, 0 };
    double m_blinkStart { 0 };
    bool m_focused { false };
    bool m_password { false };
    bool m_changed { false };
};
