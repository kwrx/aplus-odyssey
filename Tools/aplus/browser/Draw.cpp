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

#include "Draw.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Draw {

const ui_theme_t& theme()
{
    return *ui_theme();
}

bool isLight()
{
    return ui_theme_is_light();
}

ui_color_t mix(const ui_color_t& a, const ui_color_t& b, double t)
{
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t };
}

ui_color_t withAlpha(const ui_color_t& color, double alpha)
{
    return { color.r, color.g, color.b, color.a * alpha };
}

void setColor(cairo_t* cr, const ui_color_t& color, double alpha)
{
    cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a * alpha);
}

void roundedRect(cairo_t* cr, double x, double y, double width, double height, double radius)
{
    ui_draw_rounded_rect_d(cr, x, y, width, height, radius);
}

void roundedRect(cairo_t* cr, const ui_rect_t& rect, double radius)
{
    ui_draw_rounded_rect_d(cr, rect.x, rect.y, rect.width, rect.height, radius);
}

static const char* fontPath(Weight weight)
{
    const ui_theme_t& t = theme();

    switch (weight) {
    case Weight::Medium:
        return t.font_medium ? t.font_medium : t.font_regular;
    case Weight::Bold:
        return t.font_bold ? t.font_bold : t.font_regular;
    default:
        return t.font_regular;
    }
}

bool setFont(cairo_t* cr, Weight weight, double size)
{
    cairo_font_face_t* face = ui_font_face(fontPath(weight));

    if (!face) {
        return false;
    }

    cairo_set_font_face(cr, face);
    cairo_set_font_size(cr, size);
    return true;
}

double textWidth(const std::string& text, Weight weight, double size)
{
    return ui_draw_text_width(text.c_str(), fontPath(weight), size);
}

std::string ellipsize(const std::string& text, Weight weight, double size, double width)
{
    if (width <= 0) {
        return std::string();
    }

    std::vector<char> out(text.size() + 8);
    ui_draw_ellipsize(out.data(), out.size(), text.c_str(), fontPath(weight), size, width);
    return std::string(out.data());
}

double baseline(cairo_t* cr, double y, double height)
{
    cairo_font_extents_t extents;
    cairo_font_extents(cr, &extents);
    return std::round(y + (height + extents.ascent - extents.descent) / 2.0);
}

void text(cairo_t* cr, const std::string& text, double x, double y, double height, Weight weight, double size, const ui_color_t& color, double alpha)
{
    if (text.empty() || !setFont(cr, weight, size)) {
        return;
    }

    setColor(cr, color, alpha);
    cairo_move_to(cr, x, baseline(cr, y, height));
    cairo_show_text(cr, text.c_str());
    cairo_new_path(cr);
}

void shadow(cairo_t* cr, const ui_rect_t& rect, double radius, double spread, double strength)
{
    const int steps = std::max(1, (int)spread);
    strength *= isLight() ? 0.45 : 1.0;

    for (int i = steps; i >= 1; i--) {
        double t = (double)i / steps;
        double alpha = strength * (1.0 - t) * (1.0 - t) / steps * 3.0;
        roundedRect(cr, rect.x - i, rect.y - i + spread * 0.35, rect.width + i * 2, rect.height + i * 2, radius + i);
        cairo_set_source_rgba(cr, 0, 0, 0, alpha);
        cairo_fill(cr);
    }
}

}
