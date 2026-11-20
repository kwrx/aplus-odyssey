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

#include "Tab.h"

#include "App.h"
#include "BrowserWindow.h"
#include "Pages.h"
#include "WebBackForwardList.h"
#include "WebFrame.h"
#include "WebView.h"

#include "Document.h"
#include "Element.h"
#include "Frame.h"
#include "FullscreenManager.h"
#include "Page.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static std::string normalize(const std::string& text)
{
    std::string url = text;

    while (!url.empty() && isspace((unsigned char)url.front())) {
        url.erase(url.begin());
    }
    while (!url.empty() && isspace((unsigned char)url.back())) {
        url.pop_back();
    }

    if (url.empty()) {
        return std::string();
    }

    if (url.find("://") != std::string::npos || url.rfind("about:", 0) == 0 || url.rfind("data:", 0) == 0 || url.rfind("javascript:", 0) == 0) {
        return url;
    }

    if (url[0] == '/') {
        return "file://" + url;
    }

    bool hostLike = url.find(' ') == std::string::npos && (url.find('.') != std::string::npos || url.rfind("localhost", 0) == 0);

    if (hostLike) {
        return "https://" + url;
    }

    std::string query;
    for (unsigned char c : url) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            query += (char)c;
        } else if (c == ' ') {
            query += '+';
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            query += hex;
        }
    }

    return "https://duckduckgo.com/html/?q=" + query;
}

Tab::Tab(BrowserWindow* window)
    : m_window(window)
{
}

Tab::~Tab()
{
    if (m_webView) {
        m_webView->setViewWindow(nullptr);
        delete m_webView;
    }

    if (m_node.cr) {
        cairo_destroy(m_node.cr);
    }
    if (m_node.surface) {
        cairo_surface_destroy(m_node.surface);
    }
}

Tab* Tab::fromView(WebView* webView)
{
    if (!webView || !webView->viewWindow()) {
        return nullptr;
    }
    return static_cast<Tab*>(webView->viewWindow()->host);
}

Tab* Tab::fromFrame(WebFrame* frame)
{
    return frame ? fromView(frame->webView()) : nullptr;
}

bool Tab::init(int width, int height)
{
    m_webView = WebView::createInstance();
    if (!m_webView) {
        return false;
    }

    m_node.host = this;
    m_node.webView = m_webView;

    App::attachDelegates(m_webView);
    m_webView->setCustomUserAgent(App::userAgent());

    m_width = std::max(width, 1);
    m_height = std::max(height, 1);

    BalRectangle rect = { 0, 0, m_width, m_height };
    m_webView->initWithFrame(rect, "", "");
    m_webView->setViewWindow(&m_node);

    resize(m_width, m_height);
    return true;
}

void Tab::resize(int width, int height)
{
    width = std::max(width, 1);
    height = std::max(height, 1);

    if (m_node.surface && width == m_width && height == m_height) {
        return;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return;
    }

    cairo_t* cr = cairo_create(surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    if (m_node.cr) {
        cairo_destroy(m_node.cr);
    }
    if (m_node.surface) {
        cairo_surface_destroy(m_node.surface);
    }

    m_node.surface = surface;
    m_node.cr = cr;
    m_width = width;
    m_height = height;

    BalResizeEvent event = { width, height };
    m_webView->onResize(event);
    damageAll();
}

void Tab::exposeIfNeeded()
{
    if (m_node.expose && m_webView) {
        m_webView->onExpose(0);
    }
}

bool Tab::takeDamage(ui_rect_t& rect)
{
    if (!m_hasDamage) {
        return false;
    }

    rect = ui_rect_clip(&m_damage, m_width, m_height);
    m_hasDamage = false;
    return !ui_rect_empty(&rect);
}

void Tab::damageAll()
{
    m_damage = { 0, 0, m_width, m_height };
    m_hasDamage = true;
    m_window->tabDamaged(this);
}

void Tab::invalidate(int x, int y, int width, int height)
{
    ui_rect_t rect = { x, y, width, height };

    if (m_hasDamage) {
        m_damage = ui_rect_union(&m_damage, &rect);
    } else {
        m_damage = rect;
        m_hasDamage = true;
    }

    m_window->tabDamaged(this);
}

void Tab::screenSize(int& width, int& height)
{
    width = m_window->width();
    height = m_window->height();
}

void Tab::setCursor(int cursor)
{
    if (m_cursor != cursor) {
        m_cursor = cursor;
        m_window->tabCursorChanged(this);
    }
}

void Tab::setHoveredLink(const char* url)
{
    std::string link = url ? url : "";

    if (link != m_hoveredLink) {
        m_hoveredLink = link;
        m_window->tabStatusChanged(this);
    }
}

void Tab::setStatusText(const char* text)
{
    std::string status = text ? text : "";

    if (status != m_statusText) {
        m_statusText = status;
        m_window->tabStatusChanged(this);
    }
}

void Tab::setTitle(const char* title)
{
    std::string text = title ? title : "";

    if (text != m_title) {
        m_title = text;
        m_window->tabChanged(this);
    }
}

void Tab::closeView()
{
    m_closing = true;
    m_window->tabChanged(this);
}

void Tab::openExternalURL(const char* url)
{
    App::openExternal(url);
}

bool Tab::isActive()
{
    return m_window->focused() && m_window->isActiveTab(this);
}

const char* Tab::userAgent()
{
    return App::userAgent().c_str();
}

viewnode* Tab::createView(const char* url, bool background)
{
    Tab* tab = m_window->openTab(url, background);
    return tab ? tab->node() : nullptr;
}

bool Tab::login(const char* host, const char* realm, char** username, char** password)
{
    std::string user = username && *username ? *username : "";
    std::string pass;

    if (!m_window->login(this, host ? host : "", realm ? realm : "", user, pass)) {
        return false;
    }

    *username = strdup(user.c_str());
    *password = strdup(pass.c_str());
    return true;
}

void Tab::setURL(const std::string& url)
{
    std::string shown = url == "about:blank" ? std::string() : url;

    if (shown != m_url) {
        m_url = shown;
        m_hoveredLink.clear();
        m_window->tabChanged(this);
    }
}

void Tab::load(const std::string& text)
{
    std::string url = normalize(text);

    if (url.empty()) {
        showStartPage();
        return;
    }

    loadURL(url);
}

void Tab::loadURL(const std::string& url)
{
    m_pendingStart = false;
    m_pendingError = false;
    setURL(url);
    m_webView->mainFrame()->loadURL(url.c_str());
}

void Tab::showStartPage()
{
    m_pendingStart = true;
    m_pendingError = false;
    m_title = "New Tab";
    setURL(std::string());
    m_webView->mainFrame()->loadHTMLString(Pages::startPage().c_str(), "about:blank");
    m_window->tabChanged(this);
}

void Tab::showError(const std::string& url, const std::string& description)
{
    m_pendingError = true;
    m_pendingStart = false;
    m_errorURL = url;
    m_webView->mainFrame()->loadHTMLString(Pages::errorPage(url, description).c_str(), url.c_str());
}

void Tab::reload()
{
    if (!m_errorURL.empty()) {
        loadURL(m_errorURL);
        return;
    }

    if (m_startPage) {
        showStartPage();
        return;
    }

    m_webView->mainFrame()->reload();
}

void Tab::stop()
{
    m_webView->mainFrame()->stopLoading();

    if (m_loading) {
        m_loading = false;
        m_progress = 1.0;
        m_window->tabChanged(this);
    }
}

void Tab::goBack()
{
    m_webView->goBack();
}

void Tab::goForward()
{
    m_webView->goForward();
}

bool Tab::canGoBack() const
{
    WebBackForwardList* list = m_webView ? m_webView->backForwardList() : nullptr;
    return list && list->backListCount() > 0;
}

bool Tab::canGoForward() const
{
    WebBackForwardList* list = m_webView ? m_webView->backForwardList() : nullptr;
    return list && list->forwardListCount() > 0;
}

void Tab::setActive(bool active)
{
    if (!m_webView) {
        return;
    }

    if (active) {
        m_webView->setFocus();
    } else {
        m_webView->clearFocus();
    }

    m_webView->updateActiveState();
}

void Tab::zoomIn()
{
    m_webView->zoomPageIn();
}

void Tab::zoomOut()
{
    m_webView->zoomPageOut();
}

void Tab::resetZoom()
{
    m_webView->resetPageZoom();
}

int Tab::zoomPercent() const
{
    return m_webView ? (int)std::lround(m_webView->pageSizeMultiplier() * 100.0f) : 100;
}

void Tab::didStartLoading()
{
    if (!m_pendingError) {
        m_showingError = false;
        m_errorURL.clear();
    }
    if (!m_pendingStart) {
        m_startPage = false;
    }

    m_loading = true;
    m_progress = 0.08;
    m_window->tabChanged(this);
}

void Tab::didCommit(const std::string& url)
{
    if (m_pendingError) {
        m_pendingError = false;
        m_showingError = true;
        setURL(m_errorURL);
        return;
    }

    if (m_pendingStart) {
        m_pendingStart = false;
        m_startPage = true;
        setURL(std::string());
        return;
    }

    setURL(url);
}

void Tab::didFinishLoading()
{
    m_loading = false;
    m_progress = 1.0;
    m_window->tabChanged(this);
}

void Tab::didFail(const std::string& url, const std::string& description, bool cancelled)
{
    m_loading = false;
    m_progress = 1.0;

    if (!cancelled && !url.empty() && !m_pendingError) {
        showError(url, description);
    } else {
        m_pendingError = false;
        m_pendingStart = false;
    }

    m_window->tabChanged(this);
}

void Tab::updateProgress()
{
    if (!m_loading || !m_webView) {
        return;
    }

    double progress = std::max(0.08, std::min(1.0, m_webView->estimatedProgress()));

    if (progress > m_progress) {
        m_progress = progress;
        m_window->tabChanged(this);
    }
}

void Tab::setFullScreen(bool on)
{
    if (m_fullScreen == on) {
        return;
    }

    m_fullScreen = on;
    m_window->tabFullScreenChanged(this);
}

void Tab::exitFullScreen()
{
    if (!m_fullScreen) {
        return;
    }

    WebCore::Page* page = m_webView ? m_webView->page() : nullptr;
    WebCore::Document* document = page ? page->mainFrame().document() : nullptr;

    if (document && document->fullscreenManager().isFullscreen()) {
        document->fullscreenManager().cancelFullscreen();
    } else {
        setFullScreen(false);
    }
}
