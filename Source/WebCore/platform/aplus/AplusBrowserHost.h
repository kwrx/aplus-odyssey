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

#include <stddef.h>
#include <stdint.h>

struct viewnode;

class AplusBrowserHost {
public:
    virtual ~AplusBrowserHost() = default;

    virtual void screenSize(int& width, int& height) = 0;
    virtual void setCursor(int cursor) = 0;
    virtual void setToolTip(const char* text) = 0;
    virtual void setHoveredLink(const char* url) = 0;
    virtual void setStatusText(const char* text) = 0;
    virtual void setTitle(const char* title) = 0;
    virtual void invalidate(int x, int y, int width, int height) = 0;
    virtual void closeView() = 0;
    virtual void openExternalURL(const char* url) = 0;
    virtual bool isActive() = 0;
    virtual bool privateBrowsing() = 0;
    virtual const char* userAgent() = 0;
    virtual viewnode* createView(const char* url, bool background) = 0;
    virtual bool login(const char* host, const char* realm, char** username, char** password) = 0;
    virtual void setFullScreen(bool) { }
};

class AplusClipboard {
public:
    virtual ~AplusClipboard() = default;

    virtual void writeText(const char* data, size_t length) = 0;
    virtual char* readText() = 0;
};

const char* aplusConfigDir();
void aplusOpenExternal(const char* url);

void aplusSetClipboard(AplusClipboard*);
AplusClipboard* aplusClipboard();
