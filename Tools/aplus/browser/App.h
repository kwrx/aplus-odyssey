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

#include "AplusEvents.h"

#include <aplus/ui.h>
#include <cstddef>
#include <cstdint>
#include <string>

class BrowserWindow;
class WebView;

namespace App {

ui_connection_t* connection();
double now();
uint8_t modifiers();
void noteKey(uint16_t vkey, bool down);
size_t translate(uint16_t vkey, bool down, char* out, size_t size);
const std::string& userAgent();

std::string clipboard();
void setClipboard(const std::string&);

BrowserWindow* openWindow(const char* url);
BrowserWindow* activeWindow();
void setActiveWindow(BrowserWindow*);

void openExternal(const char* url);
void attachDelegates(WebView*);
int runModal(BrowserWindow*, const int* result);
std::string downloadsDirectory();

}
