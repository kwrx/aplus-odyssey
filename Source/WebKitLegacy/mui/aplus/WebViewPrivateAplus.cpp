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
#include "WebViewPrivate.h"

#include "AplusBrowserHost.h"
#include "WebFrame.h"
#include "WebView.h"

#include <WebCore/EventHandler.h>
#include <WebCore/FocusController.h>
#include <WebCore/GraphicsContextCairo.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/PlatformMouseEvent.h>
#include <WebCore/PlatformWheelEvent.h>
#include <WebCore/Scrollbar.h>
#include <WebCore/WindowsKeyboardCodes.h>
#include <wtf/MemoryPressureHandler.h>
#include <cairo.h>
#include <sys/sysinfo.h>

using namespace WebCore;

WebViewPrivate::WebViewPrivate(WebView* webView)
    : m_webView(webView)
    , m_initialized(false)
    , m_closeWindowTimer(*this, &WebViewPrivate::closeWindowTimerFired)
{
}

bool WebViewPrivate::shouldCoalesce(IntRect rect)
{
    size_t count = m_dirtyRegions.size();

    if (count <= 1 || count > 10) {
        return true;
    }

    float unionPixels = rect.width() * rect.height();
    float singlePixels = 0;

    for (auto& r : m_dirtyRegions) {
        singlePixels += r.width() * r.height();
    }

    return 1 - (singlePixels / unionPixels) <= 0.75f;
}

void WebViewPrivate::clearDirtyRegion()
{
    m_dirtyRegions.clear();
    m_backingStoreDirtyRegion = IntRect();
}

BalRectangle WebViewPrivate::dirtyRegion()
{
    BalRectangle rect = { m_backingStoreDirtyRegion.x(), m_backingStoreDirtyRegion.y(), m_backingStoreDirtyRegion.width(), m_backingStoreDirtyRegion.height() };
    return rect;
}

void WebViewPrivate::addToDirtyRegion(const BalRectangle& dirtyRect)
{
    IntRect rect(dirtyRect.x, dirtyRect.y, dirtyRect.w, dirtyRect.h);

    m_backingStoreDirtyRegion.unite(rect);

    if (m_dirtyRegions.size() > 10) {
        m_dirtyRegions.clear();
        m_dirtyRegions.append(m_backingStoreDirtyRegion);
    } else {
        m_dirtyRegions.append(rect);
    }
}

BalRectangle WebViewPrivate::onExpose(BalEventExpose)
{
    Frame* frame = core(m_webView->mainFrame());

    if (!frame || !frame->view()) {
        return IntRect();
    }

    if (!m_initialized) {
        m_initialized = true;
        frame->view()->resize(m_rect.width(), m_rect.height());
        frame->view()->forceLayout();
        frame->view()->adjustViewSize();
    }

    BalWidget* widget = m_webView->viewWindow();

    if (!widget || !widget->cr) {
        return IntRect();
    }

    widget->expose = false;

    IntRect rect(m_webView->dirtyRegion());

    if (!frame->contentRenderer() || rect.isEmpty()) {
        return rect;
    }

    GraphicsContextCairo ctx(widget->cr);

    Vector<IntRect> regions;

    if (shouldCoalesce(rect)) {
        regions.append(rect);
    } else {
        regions = m_dirtyRegions;
    }

    clearDirtyRegion();

    frame->view()->updateLayoutAndStyleIfNeededRecursive();

    for (auto& r : regions) {
        ctx.save();
        ctx.clip(r);
        frame->view()->paint(ctx, r);
        ctx.restore();
    }

    cairo_surface_flush(cairo_get_target(widget->cr));

    updateView(widget, rect);

    return rect;
}

static bool hasShift(const BalEventKey& event)
{
    return event.modifiers & APLUS_MOD_SHIFT;
}

static bool hasCommand(const BalEventKey& event)
{
    return event.modifiers & (APLUS_MOD_CTRL | APLUS_MOD_META);
}

bool WebViewPrivate::scrollForKey(FrameView* view, int virtualKey, bool shift)
{
    WebCore::ScrollDirection direction;
    WebCore::ScrollGranularity granularity = shift ? ScrollByPage : ScrollByLine;

    switch (virtualKey) {
    case VK_DOWN:
        direction = WebCore::ScrollDown;
        break;
    case VK_UP:
        direction = WebCore::ScrollUp;
        break;
    case VK_RIGHT:
        direction = WebCore::ScrollRight;
        break;
    case VK_LEFT:
        direction = WebCore::ScrollLeft;
        break;
    case VK_SPACE:
        granularity = ScrollByPage;
        direction = shift ? WebCore::ScrollUp : WebCore::ScrollDown;
        break;
    case VK_NEXT:
        granularity = ScrollByPage;
        direction = WebCore::ScrollDown;
        break;
    case VK_PRIOR:
        granularity = ScrollByPage;
        direction = WebCore::ScrollUp;
        break;
    case VK_HOME:
        granularity = ScrollByDocument;
        direction = WebCore::ScrollUp;
        break;
    case VK_END:
        granularity = ScrollByDocument;
        direction = WebCore::ScrollDown;
        break;
    default:
        return false;
    }

    bool vertical = direction == WebCore::ScrollUp || direction == WebCore::ScrollDown;
    int visible = vertical ? view->visibleHeight() : view->visibleWidth();
    int total = vertical ? view->contentsHeight() : view->contentsWidth();
    int step = 0;

    switch (granularity) {
    case ScrollByPage:
        step = std::max(std::max<int>(visible * Scrollbar::minFractionToStepWhenPaging(), visible - Scrollbar::maxOverlapBetweenPages()), 1);
        break;
    case ScrollByDocument:
        step = total;
        break;
    default:
        step = Scrollbar::pixelsPerLineStep();
        break;
    }

    if (direction == WebCore::ScrollUp || direction == WebCore::ScrollLeft) {
        step = -step;
    }

    view->scrollBy(vertical ? IntSize(0, step) : IntSize(step, 0));
    return true;
}

bool WebViewPrivate::onKeyDown(BalEventKey event)
{
    Frame& frame = m_webView->page()->focusController().focusedOrMainFrame();
    PlatformKeyboardEvent keyboardEvent(&event);
    int key = keyboardEvent.windowsVirtualKeyCode();

    if (frame.eventHandler().keyEvent(keyboardEvent)) {
        return true;
    }

    if ((event.modifiers & APLUS_MOD_ALT) && key == VK_LEFT) {
        return m_webView->goBack();
    }

    if ((event.modifiers & APLUS_MOD_ALT) && key == VK_RIGHT) {
        return m_webView->goForward();
    }

    if (hasCommand(event)) {
        switch (key) {
        case VK_C:
            m_webView->copy();
            return true;
        case VK_X:
            m_webView->cut();
            return true;
        case VK_V:
            m_webView->paste();
            return true;
        case VK_R:
            m_webView->mainFrame()->reload();
            return true;
        case VK_OEM_PLUS:
            m_webView->zoomPageIn();
            return true;
        case VK_OEM_MINUS:
            m_webView->zoomPageOut();
            return true;
        case VK_0:
            m_webView->resetPageZoom();
            return true;
        default:
            return false;
        }
    }

    if (key == VK_F5) {
        m_webView->mainFrame()->reload();
        return true;
    }

    if (key == VK_BACK) {
        return m_webView->goBack();
    }

    if (!frame.view() || (event.modifiers & APLUS_MOD_ALT)) {
        return false;
    }

    return scrollForKey(frame.view(), key, hasShift(event));
}

bool WebViewPrivate::onKeyUp(BalEventKey event)
{
    Frame& frame = m_webView->page()->focusController().focusedOrMainFrame();
    PlatformKeyboardEvent keyboardEvent(&event);

    return frame.eventHandler().keyEvent(keyboardEvent);
}

bool WebViewPrivate::onMouseMotion(BalEventMotion event)
{
    Frame* frame = core(m_webView->mainFrame());

    if (!frame) {
        return false;
    }

    return frame->eventHandler().mouseMoved(PlatformMouseEvent(&event));
}

static URL linkAtPoint(Frame* frame, const IntPoint& position)
{
    FrameView* view = frame->view();

    if (!view) {
        return URL();
    }

    HitTestResult result = frame->eventHandler().hitTestResultAtPoint(view->windowToContents(position), { HitTestRequest::Type::ReadOnly });

    if (!result.innerNonSharedNode()) {
        return URL();
    }

    return result.absoluteLinkURL();
}

bool WebViewPrivate::onMouseButtonDown(BalEventButton event)
{
    Frame* frame = core(m_webView->mainFrame());

    if (!frame) {
        return false;
    }

    PlatformMouseEvent mouseEvent(&event);

    if (mouseEvent.button() == MiddleButton) {
        return !linkAtPoint(frame, mouseEvent.position()).isEmpty();
    }

    return frame->eventHandler().handleMousePressEvent(mouseEvent);
}

bool WebViewPrivate::onMouseButtonUp(BalEventButton event)
{
    Frame* frame = core(m_webView->mainFrame());

    if (!frame) {
        return false;
    }

    PlatformMouseEvent mouseEvent(&event);

    if (mouseEvent.button() == MiddleButton) {
        URL url = linkAtPoint(frame, mouseEvent.position());

        if (url.isEmpty()) {
            return false;
        }

        BalWidget* widget = m_webView->viewWindow();

        if (widget && widget->host) {
            widget->host->createView(url.string().utf8().data(), true);
        }

        return true;
    }

    return frame->eventHandler().handleMouseReleaseEvent(mouseEvent);
}

bool WebViewPrivate::onScroll(BalEventScroll event)
{
    Frame* frame = core(m_webView->mainFrame());

    if (!frame) {
        return false;
    }

    if (event.modifiers & APLUS_MOD_CTRL) {
        if (event.dy > 0) {
            m_webView->zoomPageIn();
        } else if (event.dy < 0) {
            m_webView->zoomPageOut();
        }
        return true;
    }

    PlatformWheelEvent wheelEvent(&event);
    return frame->eventHandler().handleWheelEvent(wheelEvent, { WheelEventProcessingSteps::MainThreadForScrolling, WheelEventProcessingSteps::MainThreadForBlockingDOMEventDispatch });
}

void WebViewPrivate::onResize(BalResizeEvent event)
{
    Frame* frame = core(m_webView->mainFrame());

    m_rect.setWidth(event.w);
    m_rect.setHeight(event.h);

    if (!frame || !frame->view()) {
        return;
    }

    frame->view()->resize(event.w, event.h);
    frame->view()->forceLayout();
    frame->view()->adjustViewSize();

    m_webView->addToDirtyRegion(IntRect(0, 0, event.w, event.h));
    sendExposeEvent(m_rect);
}

void WebViewPrivate::updateView(BalWidget* widget, IntRect rect)
{
    if (!widget || !widget->host || rect.isEmpty()) {
        return;
    }

    rect.intersect(IntRect(0, 0, m_rect.width(), m_rect.height()));

    if (!rect.isEmpty()) {
        widget->host->invalidate(rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void WebViewPrivate::sendExposeEvent(IntRect)
{
    if (BalWidget* widget = m_webView->viewWindow()) {
        widget->expose = true;
    }
}

void WebViewPrivate::repaint(const IntRect& windowRect, bool contentChanged, bool immediate, bool repaintContentOnly)
{
    IntRect rect = windowRect;
    rect.intersect(IntRect(0, 0, m_rect.width(), m_rect.height()));

    if (rect.isEmpty()) {
        return;
    }

    if (contentChanged) {
        m_webView->addToDirtyRegion(rect);
    }

    if (!repaintContentOnly || immediate) {
        sendExposeEvent(rect);
    }
}

void WebViewPrivate::scrollBackingStore(FrameView*, int dx, int dy, const IntRect& scrollViewRect, const IntRect& clipRect)
{
    m_backingStoreDirtyRegion.move(dx, dy);

    for (auto& r : m_dirtyRegions) {
        r.move(dx, dy);
    }

    IntRect updateRect = clipRect;
    updateRect.intersect(scrollViewRect);

    BalWidget* widget = m_webView->viewWindow();

    if (widget && widget->surface && updateRect.width() > 0 && updateRect.height() > 0 && std::abs(dx) < updateRect.width() && std::abs(dy) < updateRect.height() && (!dx || !dy)) {
        cairo_surface_t* surface = widget->surface;
        cairo_surface_flush(surface);

        unsigned char* data = cairo_image_surface_get_data(surface);
        int stride = cairo_image_surface_get_stride(surface);
        int width = cairo_image_surface_get_width(surface);
        int height = cairo_image_surface_get_height(surface);

        IntRect area = intersection(updateRect, IntRect(0, 0, width, height));

        if (data && !area.isEmpty()) {
            IntRect source = area;
            source.move(-dx, -dy);
            source.intersect(area);
            IntRect target = source;
            target.move(dx, dy);

            int rows = target.height();
            int bytes = target.width() * 4;

            if (dy > 0) {
                for (int i = rows - 1; i >= 0; i--) {
                    memmove(data + (target.y() + i) * stride + target.x() * 4, data + (source.y() + i) * stride + source.x() * 4, bytes);
                }
            } else {
                for (int i = 0; i < rows; i++) {
                    memmove(data + (target.y() + i) * stride + target.x() * 4, data + (source.y() + i) * stride + source.x() * 4, bytes);
                }
            }

            cairo_surface_mark_dirty(surface);

            IntRect exposed = area;

            if (dx > 0) {
                exposed.setWidth(dx);
            } else if (dx < 0) {
                exposed.setX(area.maxX() + dx);
                exposed.setWidth(-dx);
            } else if (dy > 0) {
                exposed.setHeight(dy);
            } else if (dy < 0) {
                exposed.setY(area.maxY() + dy);
                exposed.setHeight(-dy);
            }

            m_webView->addToDirtyRegion(exposed);
            updateView(widget, area);
            sendExposeEvent(area);
            return;
        }
    }

    m_webView->addToDirtyRegion(updateRect);
    sendExposeEvent(updateRect);
}

void WebViewPrivate::repaintAfterNavigationIfNeeded()
{
    m_webView->addToDirtyRegion(IntRect(0, 0, m_rect.width(), m_rect.height()));
    onExpose(0);
}

void WebViewPrivate::closeWindowSoon()
{
    m_closeWindowTimer.startOneShot(0_s);
}

void WebViewPrivate::closeWindowTimerFired()
{
    if (BalWidget* widget = m_webView->viewWindow()) {
        if (widget->host) {
            widget->host->closeView();
        }
    }
}

bool WebViewPrivate::screenshot(int&, int&, Vector<char>*)
{
    return false;
}

bool WebViewPrivate::screenshot(String&)
{
    return false;
}

void WebViewPrivate::requestMemoryRelease()
{
    MemoryPressureHandler::singleton().setUnderMemoryPressure(true);
    MemoryPressureHandler::singleton().releaseMemory(Critical::Yes, Synchronous::Yes);
    MemoryPressureHandler::singleton().setUnderMemoryPressure(false);
}

bool canAllocateMemory(long long size)
{
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        return true;
    }
    long long available = (long long)si.freeram * (si.mem_unit ? si.mem_unit : 1);
    return available >= size + 1024 * 1024;
}
