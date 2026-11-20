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

#include "AplusBrowserHost.h"
#include "WebKitTypes.h"

#include <aplus/ui.h>
#include <cairo.h>
#include <string>

class BrowserWindow;
class WebFrame;
class WebView;

class Tab final : public AplusBrowserHost {
public:
    explicit Tab(BrowserWindow*);
    ~Tab() override;

    static Tab* fromView(WebView*);
    static Tab* fromFrame(WebFrame*);

    bool init(int width, int height);

    void screenSize(int& width, int& height) override;
    void setCursor(int cursor) override;
    void setToolTip(const char*) override { }
    void setHoveredLink(const char* url) override;
    void setStatusText(const char* text) override;
    void setTitle(const char* title) override;
    void invalidate(int x, int y, int width, int height) override;
    void closeView() override;
    void openExternalURL(const char* url) override;
    bool isActive() override;
    bool privateBrowsing() override { return false; }
    const char* userAgent() override;
    viewnode* createView(const char* url, bool background) override;
    bool login(const char* host, const char* realm, char** username, char** password) override;
    void setFullScreen(bool) override;

    BrowserWindow* window() const { return m_window; }
    WebView* webView() const { return m_webView; }
    viewnode* node() { return &m_node; }
    cairo_surface_t* surface() const { return m_node.surface; }

    void load(const std::string& text);
    void loadURL(const std::string& url);
    void showStartPage();
    void showError(const std::string& url, const std::string& description);
    void reload();
    void stop();
    void goBack();
    void goForward();
    bool canGoBack() const;
    bool canGoForward() const;

    void resize(int width, int height);
    int width() const { return m_width; }
    int height() const { return m_height; }
    void exposeIfNeeded();
    bool takeDamage(ui_rect_t& rect);
    void damageAll();

    void setActive(bool active);
    void exitFullScreen();
    bool fullScreen() const { return m_fullScreen; }
    void zoomIn();
    void zoomOut();
    void resetZoom();
    int zoomPercent() const;

    const std::string& title() const { return m_title; }
    const std::string& url() const { return m_url; }
    const std::string& hoveredLink() const { return m_hoveredLink; }
    const std::string& statusText() const { return m_statusText; }
    bool loading() const { return m_loading; }
    double progress() const { return m_progress; }
    int cursor() const { return m_cursor; }
    bool closing() const { return m_closing; }
    bool isStartPage() const { return m_startPage; }

    void didStartLoading();
    void didCommit(const std::string& url);
    void didFinishLoading();
    void didFail(const std::string& url, const std::string& description, bool cancelled);
    void updateProgress();

private:
    void setURL(const std::string&);

    BrowserWindow* m_window { nullptr };
    viewnode m_node {};
    WebView* m_webView { nullptr };
    int m_width { 0 };
    int m_height { 0 };

    ui_rect_t m_damage { 0, 0, 0, 0 };
    bool m_hasDamage { false };

    std::string m_title;
    std::string m_url;
    std::string m_hoveredLink;
    std::string m_statusText;
    std::string m_errorURL;
    bool m_loading { false };
    double m_progress { 0 };
    int m_cursor { 0 };
    bool m_closing { false };
    bool m_startPage { false };
    bool m_showingError { false };
    bool m_pendingStart { false };
    bool m_pendingError { false };
    bool m_fullScreen { false };
};
