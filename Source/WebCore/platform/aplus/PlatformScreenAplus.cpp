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
#include "PlatformScreen.h"

#include "AplusBrowserHost.h"
#include "FloatRect.h"
#include "FrameView.h"
#include "HostWindow.h"
#include "Widget.h"

namespace WebCore {

int screenDepth(Widget*)
{
    return 24;
}

int screenDepthPerComponent(Widget*)
{
    return 8;
}

bool screenIsMonochrome(Widget*)
{
    return false;
}

FloatRect screenRect(Widget* widget)
{
    int width = 1024;
    int height = 768;

    if (widget && widget->root() && widget->root()->hostWindow()) {
        if (auto* client = widget->root()->hostWindow()->platformPageClient()) {
            if (client->host) {
                client->host->screenSize(width, height);
            }
        }
    }

    return FloatRect(0, 0, width, height);
}

FloatRect screenAvailableRect(Widget* widget)
{
    return screenRect(widget);
}

bool screenHasInvertedColors()
{
    return false;
}

bool screenSupportsExtendedColor(Widget*)
{
    return false;
}

}
