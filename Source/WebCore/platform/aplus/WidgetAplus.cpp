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
#include "Widget.h"

#include "AplusBrowserHost.h"
#include "Cursor.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HostWindow.h"
#include "IntRect.h"

namespace WebCore {

Widget::Widget(PlatformWidget widget)
{
    init(widget);
}

Widget::~Widget()
{
    ASSERT(!parent());
}

void Widget::setFocus(bool)
{
}

void Widget::setCursor(const Cursor& cursor)
{
    if (!isFrameView()) {
        return;
    }

    auto* view = root();
    if (!view || !view->hostWindow()) {
        return;
    }

    auto* client = view->hostWindow()->platformPageClient();
    if (!client || !client->host) {
        return;
    }

    client->host->setCursor(static_cast<int>(reinterpret_cast<intptr_t>(cursor.platformCursor())));
}

void Widget::show()
{
}

void Widget::hide()
{
}

void Widget::paint(GraphicsContext&, const IntRect&, SecurityOriginPaintPolicy, EventRegionContext*)
{
}

void Widget::setIsSelected(bool)
{
}

void Widget::invalidateRect(const IntRect&)
{
}

IntRect Widget::frameRect() const
{
    return m_frame;
}

void Widget::setFrameRect(const IntRect& rect)
{
    m_frame = rect;
}

}
