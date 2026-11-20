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

#include "WebView.h"
#include "BALBase.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/IntRect.h>
#include <WebCore/Timer.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

class WebViewPrivate {
public:
    WebViewPrivate(WebView*);
    ~WebViewPrivate() { }

    void show() { }
    void hide() { }

    void setFrameRect(WebCore::IntRect r) { m_rect = r; }
    WebCore::IntRect frameRect() { return m_rect; }

    BalWidget* createWindow(BalRectangle r)
    {
        m_rect = WebCore::IntRect(r.x, r.y, r.w, r.h);
        return 0;
    }

    void initWithFrameView(WebCore::FrameView*) { }

    bool shouldCoalesce(WebCore::IntRect);
    void clearDirtyRegion();
    BalRectangle dirtyRegion();
    void addToDirtyRegion(const BalRectangle&);

    BalRectangle onExpose(BalEventExpose);
    bool onKeyDown(BalEventKey);
    bool onKeyUp(BalEventKey);
    bool onMouseMotion(BalEventMotion);
    bool onMouseButtonDown(BalEventButton);
    bool onMouseButtonUp(BalEventButton);
    bool onScroll(BalEventScroll);
    void onResize(BalResizeEvent);
    void onQuit(BalQuitEvent) { }
    void onUserEvent(BalUserEvent) { }
    void popupMenuHide() { }
    void popupMenuShow(void*) { }
    void sendExposeEvent(WebCore::IntRect);
    void repaint(const WebCore::IntRect&, bool contentChanged, bool immediate = false, bool repaintContentOnly = false);
    void scrollBackingStore(WebCore::FrameView*, int dx, int dy, const WebCore::IntRect& scrollViewRect, const WebCore::IntRect& clipRect);
    void resize(BalRectangle) { }
    void move(BalPoint, BalPoint) { }
    void repaintAfterNavigationIfNeeded();
    void closeWindowSoon();
    void setViewWindow(BalWidget*) { }
    bool screenshot(int& width, int& height, WTF::Vector<char>* imageData);
    bool screenshot(WTF::String& path);
    void requestMemoryRelease();

private:
    void updateView(BalWidget*, WebCore::IntRect);
    bool scrollForKey(WebCore::FrameView*, int virtualKey, bool shift);
    void closeWindowTimerFired();

    WebCore::IntRect m_rect;
    WebView* m_webView;
    bool m_initialized;
    WTF::Vector<WebCore::IntRect> m_dirtyRegions;
    WebCore::IntRect m_backingStoreDirtyRegion;
    WebCore::Timer m_closeWindowTimer;
};
