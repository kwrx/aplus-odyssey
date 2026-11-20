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

#include "Glyphs.h"

#include <cmath>
#include <initializer_list>

namespace Glyph {

static void begin(cairo_t* cr, double cx, double cy, double size, const ui_color_t& color, double alpha)
{
    cairo_save(cr);
    cairo_translate(cr, cx - size / 2.0, cy - size / 2.0);
    cairo_scale(cr, size / 16.0, size / 16.0);
    cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a * alpha);
    cairo_set_line_width(cr, 1.6);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
}

static void arrow(cairo_t* cr, bool forward)
{
    double dir = forward ? -1.0 : 1.0;
    double cx = 8.0;

    cairo_move_to(cr, cx + 5.0 * dir, 8.0);
    cairo_line_to(cr, cx - 5.0 * dir, 8.0);
    cairo_move_to(cr, cx - 0.8 * dir, 3.8);
    cairo_line_to(cr, cx - 5.0 * dir, 8.0);
    cairo_line_to(cr, cx - 0.8 * dir, 12.2);
    cairo_stroke(cr);
}

static void chevron(cairo_t* cr, bool up)
{
    double dir = up ? -1.0 : 1.0;

    cairo_move_to(cr, 3.5, 8.0 - 2.2 * dir);
    cairo_line_to(cr, 8.0, 8.0 + 2.2 * dir);
    cairo_line_to(cr, 12.5, 8.0 - 2.2 * dir);
    cairo_stroke(cr);
}

static void reload(cairo_t* cr)
{
    const double radius = 5.2;
    const double start = -M_PI / 2.0 + 0.35;
    const double end = start + 2.0 * M_PI - 1.15;

    cairo_arc(cr, 8.0, 8.0, radius, start, end);
    cairo_stroke(cr);

    double px = 8.0 + radius * cos(end);
    double py = 8.0 + radius * sin(end);
    double dx = -sin(end);
    double dy = cos(end);
    double nx = cos(end);
    double ny = sin(end);

    cairo_move_to(cr, px + dx * 2.6, py + dy * 2.6);
    cairo_line_to(cr, px - dx * 0.4 + nx * 2.3, py - dy * 0.4 + ny * 2.3);
    cairo_line_to(cr, px - dx * 0.4 - nx * 2.3, py - dy * 0.4 - ny * 2.3);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void cross(cairo_t* cr, double inset)
{
    cairo_move_to(cr, inset, inset);
    cairo_line_to(cr, 16.0 - inset, 16.0 - inset);
    cairo_move_to(cr, 16.0 - inset, inset);
    cairo_line_to(cr, inset, 16.0 - inset);
    cairo_stroke(cr);
}

static void plus(cairo_t* cr)
{
    cairo_move_to(cr, 8.0, 3.0);
    cairo_line_to(cr, 8.0, 13.0);
    cairo_move_to(cr, 3.0, 8.0);
    cairo_line_to(cr, 13.0, 8.0);
    cairo_stroke(cr);
}

static void lock(cairo_t* cr)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, 8.0, 6.6, 2.7, M_PI, 2.0 * M_PI);
    cairo_line_to(cr, 10.7, 8.0);
    cairo_move_to(cr, 5.3, 8.0);
    cairo_line_to(cr, 5.3, 6.6);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    double x = 3.6, y = 7.4, w = 8.8, h = 6.6, r = 1.6;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2.0, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2.0);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2.0, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3.0 * M_PI / 2.0);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void globe(cairo_t* cr)
{
    cairo_set_line_width(cr, 1.3);
    cairo_arc(cr, 8.0, 8.0, 6.0, 0, 2.0 * M_PI);
    cairo_stroke(cr);

    cairo_save(cr);
    cairo_translate(cr, 8.0, 8.0);
    cairo_scale(cr, 0.45, 1.0);
    cairo_arc(cr, 0, 0, 6.0, 0, 2.0 * M_PI);
    cairo_restore(cr);
    cairo_stroke(cr);

    cairo_move_to(cr, 2.2, 8.0);
    cairo_line_to(cr, 13.8, 8.0);
    cairo_stroke(cr);
}

static void search(cairo_t* cr)
{
    cairo_arc(cr, 7.0, 7.0, 4.3, 0, 2.0 * M_PI);
    cairo_stroke(cr);
    cairo_move_to(cr, 10.2, 10.2);
    cairo_line_to(cr, 13.5, 13.5);
    cairo_stroke(cr);
}

static void menu(cairo_t* cr)
{
    for (double y : { 3.6, 8.0, 12.4 }) {
        cairo_new_sub_path(cr);
        cairo_arc(cr, 8.0, y, 1.35, 0, 2.0 * M_PI);
    }
    cairo_fill(cr);
}

static void download(cairo_t* cr)
{
    cairo_move_to(cr, 8.0, 2.8);
    cairo_line_to(cr, 8.0, 10.0);
    cairo_move_to(cr, 4.6, 6.8);
    cairo_line_to(cr, 8.0, 10.2);
    cairo_line_to(cr, 11.4, 6.8);
    cairo_move_to(cr, 3.0, 13.2);
    cairo_line_to(cr, 13.0, 13.2);
    cairo_stroke(cr);
}

static void warning(cairo_t* cr)
{
    cairo_set_line_width(cr, 1.4);
    cairo_move_to(cr, 8.0, 2.4);
    cairo_line_to(cr, 14.2, 13.4);
    cairo_line_to(cr, 1.8, 13.4);
    cairo_close_path(cr);
    cairo_stroke(cr);
    cairo_move_to(cr, 8.0, 6.4);
    cairo_line_to(cr, 8.0, 9.4);
    cairo_stroke(cr);
    cairo_arc(cr, 8.0, 11.4, 0.8, 0, 2.0 * M_PI);
    cairo_fill(cr);
}

void draw(cairo_t* cr, Kind kind, double cx, double cy, double size, const ui_color_t& color, double alpha)
{
    cairo_new_path(cr);
    begin(cr, cx, cy, size, color, alpha);

    switch (kind) {
    case Kind::Back:
        arrow(cr, false);
        break;
    case Kind::Forward:
        arrow(cr, true);
        break;
    case Kind::Reload:
        reload(cr);
        break;
    case Kind::Stop:
        cross(cr, 3.6);
        break;
    case Kind::Close:
        cairo_set_line_width(cr, 1.5);
        cross(cr, 4.6);
        break;
    case Kind::Plus:
        plus(cr);
        break;
    case Kind::Lock:
        lock(cr);
        break;
    case Kind::Globe:
        globe(cr);
        break;
    case Kind::Search:
        search(cr);
        break;
    case Kind::Menu:
        menu(cr);
        break;
    case Kind::Download:
        download(cr);
        break;
    case Kind::Up:
        chevron(cr, true);
        break;
    case Kind::Down:
        chevron(cr, false);
        break;
    case Kind::Warning:
        warning(cr);
        break;
    }

    cairo_restore(cr);
}

void spinner(cairo_t* cr, double cx, double cy, double size, double phase, const ui_color_t& color)
{
    cairo_save(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, size / 8.0);

    double radius = size / 2.0 - size / 10.0;

    cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a * 0.22);
    cairo_arc(cr, cx, cy, radius, 0, 2.0 * M_PI);
    cairo_stroke(cr);

    double start = phase * 2.0 * M_PI;
    double sweep = M_PI * (0.55 + 0.35 * sin(phase * 2.0 * M_PI * 0.5));

    cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
    cairo_arc(cr, cx, cy, radius, start, start + sweep);
    cairo_stroke(cr);
    cairo_restore(cr);
}

}
