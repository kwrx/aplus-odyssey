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

#include "AplusBrowserHost.h"
#include "App.h"
#include "BrowserWindow.h"
#include "Delegates.h"
#include "Tab.h"

#include <wtf/MainThread.h>
#include <wtf/RunLoop.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <aplus/ui-keymap.h>
#include <aplus/ui-widgets.h>
#include <aplus/ui.h>

extern char** environ;

static constexpr uint16_t keyLeftMeta = 125;
static constexpr uint16_t keyRightMeta = 126;

static ui_connection_t* g_conn;
static ui_keymap_t* g_keymap;
static std::vector<BrowserWindow*> g_windows;
static std::vector<ui_event_t> g_pending;
static BrowserWindow* g_active;
static int g_wake[2] = { -1, -1 };
static bool g_meta;
static int g_modalDepth;
static const std::string g_userAgent = "Mozilla/5.0 (X11; aplus x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Safari/605.1.15";

static void wakeMainLoop()
{
    char c = 1;
    if (write(g_wake[1], &c, 1) < 0) {
        return;
    }
}

static void drainWake()
{
    char buffer[64];
    while (read(g_wake[0], buffer, sizeof(buffer)) > 0) {
    }
}

static BrowserWindow* windowById(uint32_t id)
{
    for (auto* window : g_windows) {
        if (window->id() == id) {
            return window;
        }
    }
    return nullptr;
}

static void dispatchEvent(const ui_event_t& event)
{
    if (event.type == UI_EVENT_THEME) {
        ui_theme_update(g_conn);
        for (auto* window : g_windows) {
            window->handleEvent(event);
        }
        return;
    }

    if (BrowserWindow* window = windowById(event.window_id)) {
        window->handleEvent(event);
    }
}

static int waitForEvents(int timeout)
{
    struct pollfd fds[2] = {
        { ui_connection_fd(g_conn), POLLIN, 0 },
        { g_wake[0], POLLIN, 0 },
    };

    if (poll(fds, 2, timeout) < 0 && errno != EINTR) {
        return -1;
    }

    if (fds[1].revents & POLLIN) {
        drainWake();
    }

    return 0;
}

class Clipboard final : public AplusClipboard {
public:
    void writeText(const char* data, size_t length) override
    {
        std::string text(data, length);
        ui_clipboard_set_text(g_conn, text.c_str());
    }

    char* readText() override
    {
        uint32_t serial = 0;
        if (ui_clipboard_request(g_conn, &serial) < 0) {
            return nullptr;
        }

        double deadline = App::now() + 0.5;

        while (App::now() < deadline) {
            ui_event_t event;
            int r = ui_next_event(g_conn, &event, 50);

            if (r < 0) {
                return nullptr;
            }
            if (r == 0) {
                continue;
            }

            if (event.type == UI_EVENT_CLIPBOARD && event.clipboard.serial == serial) {
                size_t size = 0;
                const char* mime = nullptr;
                const void* data = ui_clipboard_data(g_conn, &size, &mime);

                if (!data || !size) {
                    return nullptr;
                }

                char* out = (char*)malloc(size + 1);
                if (!out) {
                    return nullptr;
                }

                memcpy(out, data, size);
                out[size] = '\0';
                return out;
            }

            g_pending.push_back(event);
        }

        return nullptr;
    }
};

static Clipboard g_clipboard;

namespace App {

ui_connection_t* connection()
{
    return g_conn;
}

double now()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

uint8_t modifiers()
{
    uint8_t mods = g_keymap ? ui_keymap_modifiers(g_keymap) : 0;
    uint8_t out = 0;

    if (mods & ((1 << 0) | (1 << 4) | (1 << 5))) {
        out |= APLUS_MOD_SHIFT;
    }
    if (mods & ((1 << 2) | (1 << 6) | (1 << 7))) {
        out |= APLUS_MOD_CTRL;
    }
    if (mods & (1 << 3)) {
        out |= APLUS_MOD_ALT;
    }
    if (g_meta) {
        out |= APLUS_MOD_META;
    }

    return out;
}

void noteKey(uint16_t vkey, bool down)
{
    if (vkey == keyLeftMeta || vkey == keyRightMeta) {
        g_meta = down;
    }
}

size_t translate(uint16_t vkey, bool down, char* out, size_t size)
{
    if (!g_keymap || !size) {
        return 0;
    }

    size_t length = ui_keymap_translate(g_keymap, vkey, down, out, size);
    out[std::min(length, size - 1)] = '\0';
    return length;
}

const std::string& userAgent()
{
    return g_userAgent;
}

std::string clipboard()
{
    char* text = g_clipboard.readText();
    std::string out = text ? text : "";
    free(text);
    return out;
}

void setClipboard(const std::string& text)
{
    ui_clipboard_set_text(g_conn, text.c_str());
}

BrowserWindow* openWindow(const char* url)
{
    BrowserWindow* window = BrowserWindow::create(url);

    if (window) {
        g_windows.push_back(window);
        g_active = window;
    }

    return window;
}

BrowserWindow* activeWindow()
{
    return g_active;
}

void setActiveWindow(BrowserWindow* window)
{
    g_active = window;
}

void openExternal(const char* url)
{
    aplusOpenExternal(url);
}

void attachDelegates(WebView* webView)
{
    Delegates::attach(webView);
}

std::string downloadsDirectory()
{
    const char* home = getenv("HOME");
    std::string dir = std::string(home && home[0] ? home : "/tmp") + "/Downloads";
    mkdir(dir.c_str(), 0755);
    return dir;
}

int runModal(BrowserWindow* modal, const int* result)
{
    g_modalDepth++;

    while (*result < 0 && !modal->closed()) {
        modal->update();

        for (auto* window : g_windows) {
            window->present();
        }

        ui_event_t event;
        int r = ui_next_event(g_conn, &event, 0);

        if (r < 0) {
            break;
        }

        if (r == 0) {
            if (waitForEvents(modal->timeout()) < 0) {
                break;
            }
            continue;
        }

        do {
            if (event.window_id == modal->id() || event.type == UI_EVENT_THEME) {
                dispatchEvent(event);
            } else {
                g_pending.push_back(event);
            }
        } while (*result < 0 && (r = ui_next_event(g_conn, &event, 0)) > 0);

        if (r < 0) {
            break;
        }
    }

    g_modalDepth--;
    return *result;
}

}

static void reapWindows()
{
    for (size_t i = 0; i < g_windows.size();) {
        BrowserWindow* window = g_windows[i];

        if (!window->closed()) {
            i++;
            continue;
        }

        g_windows.erase(g_windows.begin() + i);

        if (g_active == window) {
            g_active = g_windows.empty() ? nullptr : g_windows.back();
        }

        delete window;
    }
}

static int runLoop()
{
    while (!g_windows.empty()) {
        WTF::RunLoop::iterate();

        if (!g_pending.empty()) {
            std::vector<ui_event_t> pending;
            pending.swap(g_pending);
            for (auto& event : pending) {
                dispatchEvent(event);
            }
        }

        for (size_t i = 0; i < g_windows.size(); i++) {
            g_windows[i]->update();
        }

        reapWindows();

        if (g_windows.empty()) {
            break;
        }

        int timeout = -1;

        for (auto* window : g_windows) {
            if (window->present() < 0) {
                window->close();
            }

            int t = window->timeout();
            if (t >= 0 && (timeout < 0 || t < timeout)) {
                timeout = t;
            }
        }

        int next = (int)std::ceil(WTF::RunLoop::secondsUntilNextIterate().milliseconds());
        if (next >= 0 && (timeout < 0 || next < timeout)) {
            timeout = next;
        }

        if (!g_pending.empty()) {
            continue;
        }

        ui_event_t event;
        int r = ui_next_event(g_conn, &event, 0);

        if (r < 0) {
            return -1;
        }

        if (r == 0) {
            if (waitForEvents(timeout) < 0) {
                return -1;
            }
            continue;
        }

        do {
            dispatchEvent(event);
        } while ((r = ui_next_event(g_conn, &event, 0)) > 0);

        if (r < 0) {
            return -1;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if (pipe(g_wake) < 0) {
        perror("odyssey: pipe");
        return 1;
    }

    fcntl(g_wake[0], F_SETFL, O_NONBLOCK);
    fcntl(g_wake[1], F_SETFL, O_NONBLOCK);

    g_conn = ui_connect(nullptr, 2000);
    if (!g_conn) {
        fprintf(stderr, "odyssey: cannot connect to the window manager: %s\n", strerror(errno));
        return 1;
    }

    g_keymap = ui_keymap_open(nullptr);

    WTF::initializeMainThread();
    WTF::RunLoop::setWakeUpCallback(wakeMainLoop);

    aplusSetClipboard(&g_clipboard);
    Delegates::registerObservers();

    if (!App::openWindow(argc > 1 ? argv[1] : nullptr)) {
        fprintf(stderr, "odyssey: cannot create the browser window\n");
        return 1;
    }

    int ret = runLoop();

    while (!g_windows.empty()) {
        delete g_windows.back();
        g_windows.pop_back();
    }

    if (g_keymap) {
        ui_keymap_close(g_keymap);
    }

    ui_disconnect(g_conn);
    return ret < 0 ? 1 : 0;
}
