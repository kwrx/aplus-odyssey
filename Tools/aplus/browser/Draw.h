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

#include <aplus/ui-draw.h>
#include <aplus/ui-widgets.h>
#include <aplus/ui.h>
#include <cairo.h>
#include <string>

namespace Draw {

enum class Weight {
    Regular,
    Medium,
    Bold,
};

const ui_theme_t& theme();
bool isLight();

ui_color_t mix(const ui_color_t& a, const ui_color_t& b, double t);
ui_color_t withAlpha(const ui_color_t& color, double alpha);
void setColor(cairo_t*, const ui_color_t&, double alpha = 1.0);

void roundedRect(cairo_t*, double x, double y, double width, double height, double radius);
void roundedRect(cairo_t*, const ui_rect_t&, double radius);

bool setFont(cairo_t*, Weight, double size);
double textWidth(const std::string&, Weight, double size);
std::string ellipsize(const std::string&, Weight, double size, double width);
double baseline(cairo_t*, double y, double height);
void text(cairo_t*, const std::string&, double x, double y, double height, Weight, double size, const ui_color_t&, double alpha = 1.0);
void shadow(cairo_t*, const ui_rect_t&, double radius, double spread, double strength);

}
