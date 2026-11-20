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

#include "BrowserWindow.h"

#include "App.h"
#include "PopupMenuMorphOS.h"
#include "Tab.h"
#include "WebFrame.h"
#include "WebHitTestResults.h"
#include "WebView.h"

#include <wtf/text/CString.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

static constexpr uint16_t keyEscape = 1;
static constexpr uint16_t key1 = 2;
static constexpr uint16_t key9 = 10;
static constexpr uint16_t keyTab = 15;
static constexpr uint16_t keyW = 17;
static constexpr uint16_t keyR = 19;
static constexpr uint16_t keyT = 20;
static constexpr uint16_t keyEnter = 28;
static constexpr uint16_t keyD = 32;
static constexpr uint16_t keyF = 33;
static constexpr uint16_t keyG = 34;
static constexpr uint16_t keyL = 38;
static constexpr uint16_t keyN = 49;
static constexpr uint16_t keySpace = 57;
static constexpr uint16_t keyF3 = 61;
static constexpr uint16_t keyF5 = 63;
static constexpr uint16_t keyF6 = 64;
static constexpr uint16_t keyKeypadEnter = 96;
static constexpr uint16_t keyHome = 102;
static constexpr uint16_t keyUp = 103;
static constexpr uint16_t keyPageUp = 104;
static constexpr uint16_t keyLeft = 105;
static constexpr uint16_t keyRight = 106;
static constexpr uint16_t keyEnd = 107;
static constexpr uint16_t keyDown = 108;
static constexpr uint16_t keyPageDown = 109;

static constexpr double hoverTime = 0.07;
static constexpr double spinnerInterval = 1.0 / 30.0;

enum : uint32_t {
    MenuNewTab = 1,
    MenuNewWindow,
    MenuFind,
    MenuZoomIn,
    MenuZoomOut,
    MenuZoomReset,
    MenuDownloads,
    MenuCloseWindow,
    MenuOpenLinkTab = 10,
    MenuOpenLinkWindow,
    MenuCopyLink,
    MenuOpenImageTab = 20,
    MenuCopyImage,
    MenuCopy = 30,
    MenuSearchSelection,
    MenuCut = 40,
    MenuPaste,
    MenuSelectAll,
    MenuBack = 50,
    MenuForward,
    MenuReload,
    MenuTabNew = 60,
    MenuTabReload,
    MenuTabDuplicate,
    MenuTabClose,
    MenuTabCloseOthers,
    MenuTabReopen,
};

static bool step(double& value, double target, double k)
{
    if (std::fabs(target - value) < 0.002) {
        bool changed = value != target;
        value = target;
        return changed;
    }

    value += (target - value) * k;
    return true;
}

static void addItem(std::vector<ui_menu_item_t>& items, uint32_t id, const std::string& label, uint32_t flags = 0)
{
    if (items.size() >= UI_MENU_ITEMS_MAX) {
        return;
    }

    ui_menu_item_t item;
    memset(&item, 0, sizeof(item));
    item.id = id;
    item.flags = flags;
    snprintf(item.label, sizeof(item.label), "%s", label.c_str());
    items.push_back(item);
}

static void addSeparator(std::vector<ui_menu_item_t>& items)
{
    if (items.empty() || (items.back().flags & UI_MENU_SEPARATOR)) {
        return;
    }
    addItem(items, 0xFFFF0000u + (uint32_t)items.size(), "", UI_MENU_SEPARATOR | UI_MENU_DISABLED);
}

static std::string takeString(const char* text)
{
    std::string out = text ? text : "";
    free((void*)text);
    return out;
}

static std::string shorten(const std::string& text, size_t max)
{
    std::string out;
    for (char c : text) {
        out += (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
    }

    while (out.find("  ") != std::string::npos) {
        out.erase(out.find("  "), 1);
    }

    if (out.size() <= max) {
        return out;
    }

    size_t cut = max;
    while (cut > 0 && ((unsigned char)out[cut] & 0xC0) == 0x80) {
        cut--;
    }
    return out.substr(0, cut) + "…";
}

static std::string urlEncode(const std::string& text)
{
    std::string out;
    for (unsigned char c : text) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else if (c == ' ') {
            out += '+';
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            out += hex;
        }
    }
    return out;
}

BrowserWindow* BrowserWindow::create(const char* url)
{
    BrowserWindow* window = new BrowserWindow();

    if (!window->init()) {
        delete window;
        return nullptr;
    }

    if (url && url[0]) {
        Tab* tab = window->openTab(url, false);
        if (!tab) {
            delete window;
            return nullptr;
        }
    } else {
        Tab* tab = window->openTab(nullptr, false);
        if (!tab) {
            delete window;
            return nullptr;
        }
        tab->showStartPage();
        window->focusUrl();
    }

    return window;
}

bool BrowserWindow::init()
{
    m_window = ui_window_create(App::connection(), 1180, 800, "Odyssey");
    if (!m_window) {
        return false;
    }

    ui_window_set_icon(m_window, "odyssey");

    m_width = ui_window_width(m_window);
    m_height = ui_window_height(m_window);
    m_lastFrame = App::now();

    layout();
    return true;
}

BrowserWindow::~BrowserWindow()
{
    closeDropdown(true);

    for (Tab* tab : m_tabs) {
        delete tab;
    }
    m_tabs.clear();

    if (m_window) {
        ui_window_destroy(m_window);
    }
}

uint32_t BrowserWindow::id() const
{
    return ui_window_id(m_window);
}

int BrowserWindow::width() const
{
    return m_width;
}

int BrowserWindow::height() const
{
    return m_height;
}

Tab* BrowserWindow::activeTab() const
{
    if (m_active < 0 || m_active >= (int)m_tabs.size()) {
        return nullptr;
    }
    return m_tabs[m_active];
}

void BrowserWindow::layout()
{
    syncModel();
    m_ui.layout(m_model);
    resizeTabs();
    m_barsDirty = true;
    m_overlayDirty = true;
    m_contentDirty = true;
}

void BrowserWindow::resizeTabs()
{
    ui_rect_t content = m_ui.contentRect();

    if (Tab* tab = activeTab()) {
        tab->resize(content.width, content.height);
    }
}

Tab* BrowserWindow::openTab(const char* url, bool background)
{
    ui_rect_t content = m_ui.contentRect();
    Tab* tab = new Tab(this);

    if (!tab->init(content.width, content.height)) {
        delete tab;
        return nullptr;
    }

    int index = m_tabs.empty() ? 0 : std::min(m_active + 1, (int)m_tabs.size());
    m_tabs.insert(m_tabs.begin() + index, tab);
    m_tabAnims.insert(m_tabAnims.begin() + index, TabAnim());

    if (index <= m_active && m_tabs.size() > 1) {
        m_active++;
    }

    if (url && url[0]) {
        tab->load(url);
    }

    if (!background || m_tabs.size() == 1) {
        activateTab(tab);
    }

    markBars();
    return tab;
}

void BrowserWindow::activateTab(Tab* tab)
{
    auto it = std::find(m_tabs.begin(), m_tabs.end(), tab);
    if (it == m_tabs.end()) {
        return;
    }

    int index = (int)(it - m_tabs.begin());
    Tab* previous = activeTab();

    if (previous && previous != tab) {
        previous->exitFullScreen();
        previous->setActive(false);
    }

    if (m_model.find.visible && previous && previous != tab) {
        previous->webView()->unmarkAllTextMatches();
    }

    closeDropdown(true);

    m_active = index;
    m_progress = tab->loading() ? tab->progress() : 0.0;
    m_progressAlpha.value = tab->loading() ? 1.0 : 0.0;

    ui_rect_t content = m_ui.contentRect();
    tab->resize(content.width, content.height);
    tab->damageAll();

    if (m_focus == Focus::Url) {
        m_urlField.blur();
        m_focus = Focus::Web;
    }

    if (tab->isStartPage() && tab->url().empty()) {
        focusUrl();
    } else {
        tab->setActive(m_focused && m_focus == Focus::Web);
    }

    if (m_model.find.visible) {
        findChanged();
    }

    m_contentDirty = true;
    markBars();
    markOverlays();
    updateTitle();
    updateCursor();
}

void BrowserWindow::closeTab(Tab* tab)
{
    if (!tab || (modal() && tab == m_dialogTab)) {
        return;
    }

    tab->closeView();
}

void BrowserWindow::tabChanged(Tab* tab)
{
    markBars();

    if (tab == activeTab()) {
        updateTitle();
    }
}

void BrowserWindow::tabDamaged(Tab*)
{
}

void BrowserWindow::tabCursorChanged(Tab* tab)
{
    if (tab == activeTab()) {
        updateCursor();
    }
}

void BrowserWindow::tabStatusChanged(Tab* tab)
{
    if (tab == activeTab()) {
        markOverlays();
    }
}

void BrowserWindow::tabFullScreenChanged(Tab* tab)
{
    bool on = tab == activeTab() && tab->fullScreen();

    if (m_model.fullScreen == on) {
        return;
    }

    m_model.fullScreen = on;

    if (on) {
        if (m_model.find.visible) {
            closeFind();
        }
        closeDropdown(true);
        focusWeb();
    }

    layout();

    if (Tab* active = activeTab()) {
        active->damageAll();
    }

    if (on) {
        flash("Press Esc to exit full screen", 3.0);
    }

    updateCursor();
}

void BrowserWindow::flash(const std::string& text, double seconds)
{
    m_flash = text;
    m_flashUntil = App::now() + seconds;
    markOverlays();
}

std::string BrowserWindow::currentStatus() const
{
    Tab* tab = activeTab();

    if (tab && !tab->hoveredLink().empty()) {
        return tab->hoveredLink();
    }

    if (!m_flash.empty() && App::now() < m_flashUntil) {
        return m_flash;
    }

    if (tab && !tab->statusText().empty()) {
        return tab->statusText();
    }

    return std::string();
}

void BrowserWindow::updateTitle()
{
    Tab* tab = activeTab();
    std::string title = tab && !tab->title().empty() ? tab->title() + " — Odyssey" : std::string("Odyssey");

    if (title != m_windowTitle) {
        m_windowTitle = title;
        ui_window_set_title(m_window, title.c_str());
    }
}

void BrowserWindow::updateCursor()
{
    int cursor = UI_CURSOR_DEFAULT;

    if (m_hover.target == Target::Url || m_hover.target == Target::FindField || m_hover.target == Target::DialogField) {
        cursor = UI_CURSOR_TEXT;
    } else if (m_hover.target == Target::Content || m_captureContent) {
        Tab* tab = activeTab();
        cursor = tab && tab->cursor() == UI_CURSOR_TEXT ? UI_CURSOR_TEXT : UI_CURSOR_DEFAULT;
    }

    if (cursor != m_cursor) {
        m_cursor = cursor;
        ui_window_set_cursor(m_window, (ui_cursor_t)cursor);
    }
}

void BrowserWindow::syncModel()
{
    Tab* tab = activeTab();
    double now = App::now();

    m_model.width = m_width;
    m_model.height = m_height;
    m_model.now = now;
    m_model.activeTab = m_active;
    m_model.tabs.resize(m_tabs.size());

    for (size_t i = 0; i < m_tabs.size(); i++) {
        TabModel& tm = m_model.tabs[i];
        Tab* t = m_tabs[i];

        tm.title = t->title();
        if (tm.title.empty()) {
            tm.title = t->url().empty() ? std::string(t->loading() ? "Loading…" : "New Tab") : t->url();
        }
        tm.loading = t->loading();
        tm.hover = i < m_tabAnims.size() ? m_tabAnims[i].hover.value : 0.0;
        tm.closeHover = i < m_tabAnims.size() ? m_tabAnims[i].close.value : 0.0;
    }

    m_model.newTabHover = m_newTabHover.value;
    m_model.hoverBack = m_backHover.value;
    m_model.hoverForward = m_forwardHover.value;
    m_model.hoverReload = m_reloadHover.value;
    m_model.hoverMenu = m_menuHover.value;
    m_model.hoverUrl = m_urlHover.value;
    m_model.pressed = m_pressed.target;

    m_model.canGoBack = tab && tab->canGoBack();
    m_model.canGoForward = tab && tab->canGoForward();
    m_model.loading = tab && tab->loading();
    m_model.progress = m_progress;
    m_model.progressAlpha = m_progressAlpha.value;

    m_model.url = tab ? tab->url() : std::string();
    m_model.urlField = &m_urlField;

    m_model.status = currentStatus();
    m_model.pointerX = m_pointerX;
    m_model.pointerY = m_pointerY;

    m_model.find.field = &m_findField;
    for (int i = 0; i < 3; i++) {
        m_model.find.hover[i] = m_findHover[i].value;
    }
    for (int i = 0; i < 4; i++) {
        m_model.dialog.hover[i] = m_dialogHover[i].value;
    }
}

bool BrowserWindow::animating() const
{
    auto moving = [](const Anim& a) {
        return std::fabs(a.value - a.target) >= 0.002;
    };

    for (const auto& anim : m_tabAnims) {
        if (moving(anim.hover) || moving(anim.close)) {
            return true;
        }
    }

    if (moving(m_newTabHover) || moving(m_backHover) || moving(m_forwardHover) || moving(m_reloadHover) || moving(m_menuHover) || moving(m_urlHover)) {
        return true;
    }

    for (const auto& anim : m_findHover) {
        if (moving(anim)) {
            return true;
        }
    }
    for (const auto& anim : m_dialogHover) {
        if (moving(anim)) {
            return true;
        }
    }

    Tab* tab = activeTab();
    double target = tab && tab->loading() ? tab->progress() : (m_progressAlpha.value > 0.002 ? 1.0 : 0.0);

    return moving(m_progressAlpha) || (m_progressAlpha.value > 0.002 && std::fabs(m_progress - target) >= 0.002);
}

void BrowserWindow::animate(double dt)
{
    double k = 1.0 - std::exp(-dt / hoverTime);
    bool bars = false;
    bool overlays = false;

    for (auto& anim : m_tabAnims) {
        bars |= step(anim.hover.value, anim.hover.target, k);
        bars |= step(anim.close.value, anim.close.target, k);
    }

    bars |= step(m_newTabHover.value, m_newTabHover.target, k);
    bars |= step(m_backHover.value, m_backHover.target, k);
    bars |= step(m_forwardHover.value, m_forwardHover.target, k);
    bars |= step(m_reloadHover.value, m_reloadHover.target, k);
    bars |= step(m_menuHover.value, m_menuHover.target, k);
    bars |= step(m_urlHover.value, m_urlHover.target, k);

    for (auto& anim : m_findHover) {
        overlays |= step(anim.value, anim.target, k);
    }
    for (auto& anim : m_dialogHover) {
        overlays |= step(anim.value, anim.target, k);
    }

    Tab* tab = activeTab();
    bool loading = tab && tab->loading();
    double target = loading ? tab->progress() : 1.0;

    if (loading && m_progressAlpha.value < 0.002) {
        m_progress = 0.0;
    }

    m_progressAlpha.target = (loading || m_progress < 0.985) && (loading || m_progressAlpha.value > 0.002) ? 1.0 : 0.0;

    if (m_progressAlpha.value > 0.002 || loading) {
        bars |= step(m_progress, target, 1.0 - std::exp(-dt / (loading ? 0.25 : 0.09)));
    }

    bars |= step(m_progressAlpha.value, m_progressAlpha.target, 1.0 - std::exp(-dt / (m_progressAlpha.target > 0.5 ? 0.05 : 0.22)));

    if (m_progressAlpha.value <= 0.002 && !loading) {
        m_progress = 0.0;
    }

    if (bars) {
        markBars();
    }
    if (overlays) {
        markOverlays();
    }
}

void BrowserWindow::setHover(const Hit& hit)
{
    m_hover = hit;

    for (size_t i = 0; i < m_tabAnims.size(); i++) {
        bool over = (hit.target == Target::Tab || hit.target == Target::TabClose) && hit.index == (int)i;
        m_tabAnims[i].hover.target = over ? 1.0 : 0.0;
        m_tabAnims[i].close.target = hit.target == Target::TabClose && hit.index == (int)i ? 1.0 : 0.0;
    }

    m_newTabHover.target = hit.target == Target::NewTab ? 1.0 : 0.0;
    m_backHover.target = hit.target == Target::Back ? 1.0 : 0.0;
    m_forwardHover.target = hit.target == Target::Forward ? 1.0 : 0.0;
    m_reloadHover.target = hit.target == Target::Reload ? 1.0 : 0.0;
    m_menuHover.target = hit.target == Target::Menu ? 1.0 : 0.0;
    m_urlHover.target = hit.target == Target::Url ? 1.0 : 0.0;
    m_findHover[0].target = hit.target == Target::FindPrevious ? 1.0 : 0.0;
    m_findHover[1].target = hit.target == Target::FindNext ? 1.0 : 0.0;
    m_findHover[2].target = hit.target == Target::FindClose ? 1.0 : 0.0;

    for (int i = 0; i < 4; i++) {
        m_dialogHover[i].target = hit.target == Target::DialogButton && hit.index == i ? 1.0 : 0.0;
    }

    if (m_model.dropdown.visible && hit.target == Target::DropdownItem && hit.index >= 0 && hit.index != m_model.dropdown.highlighted) {
        const DropdownItem& item = m_model.dropdown.items[hit.index];
        if (item.enabled && !item.label && !item.separator) {
            m_model.dropdown.highlighted = hit.index;
            markOverlays();
        }
    }
}

void BrowserWindow::update()
{
    double now = App::now();
    double dt = std::min(0.1, std::max(0.0, now - m_lastFrame));
    m_lastFrame = now;

    for (size_t i = 0; i < m_tabs.size() && !modal();) {
        Tab* tab = m_tabs[i];

        if (!tab->closing()) {
            i++;
            continue;
        }

        if (modal() && tab == m_dialogTab) {
            i++;
            continue;
        }

        if (!tab->isStartPage() && !tab->url().empty()) {
            m_closedTabs.push_back(tab->url());
            if (m_closedTabs.size() > 25) {
                m_closedTabs.erase(m_closedTabs.begin());
            }
        }

        bool wasActive = (int)i == m_active;

        m_tabs.erase(m_tabs.begin() + i);
        m_tabAnims.erase(m_tabAnims.begin() + i);

        if ((int)i < m_active || (wasActive && m_active >= (int)m_tabs.size())) {
            m_active = std::max(0, m_active - 1);
        }

        if (wasActive) {
            closeDropdown(true);
        }

        delete tab;

        if (m_tabs.empty()) {
            m_closing = true;
            return;
        }

        if (wasActive) {
            Tab* next = m_tabs[m_active];
            if (m_model.fullScreen) {
                m_model.fullScreen = false;
                layout();
            }
            m_active = -1;
            activateTab(next);
        }

        markBars();
    }

    for (Tab* tab : m_tabs) {
        tab->updateProgress();
    }

    if (Tab* tab = activeTab(); tab && !modal()) {
        tab->exposeIfNeeded();
    }

    animate(dt);

    bool spinning = false;
    for (Tab* tab : m_tabs) {
        spinning |= tab->loading();
    }

    if (spinning && now - m_lastSpinner >= spinnerInterval) {
        m_lastSpinner = now;
        markBars();
    }

    if (!m_flash.empty() && now >= m_flashUntil) {
        m_flash.clear();
        markOverlays();
    }

    TextField* field = focusedField();
    if (m_focused && field && field->timeout(now) >= 0 && now >= m_nextBlink) {
        if (m_focus == Focus::Url) {
            markBars();
        } else {
            markOverlays();
        }
    }

    std::string status = currentStatus();
    if (status != m_lastStatus) {
        m_lastStatus = status;
        markOverlays();
    }
}

int BrowserWindow::timeout() const
{
    double now = App::now();
    int timeout = -1;

    auto take = [&](int t) {
        if (t >= 0 && (timeout < 0 || t < timeout)) {
            timeout = t;
        }
    };

    if (animating()) {
        take(16);
    }

    for (Tab* tab : m_tabs) {
        if (tab->loading()) {
            take((int)(spinnerInterval * 1000.0));
            break;
        }
    }

    if (m_focused) {
        if (const TextField* field = const_cast<BrowserWindow*>(this)->focusedField()) {
            take(field->timeout(now));
        }
    }

    if (!m_flash.empty()) {
        take(std::max(0, (int)std::ceil((m_flashUntil - now) * 1000.0)));
    }

    return timeout;
}

void BrowserWindow::blitContent(const ui_rect_t& area)
{
    ui_rect_t content = m_ui.contentRect();
    ui_rect_t rect = ui_rect_clip(&area, content.width, content.height);

    if (ui_rect_empty(&rect)) {
        return;
    }

    uint32_t* pixels = ui_window_pixels(m_window);
    size_t stride = ui_window_stride(m_window);
    Tab* tab = activeTab();
    cairo_surface_t* surface = tab ? tab->surface() : nullptr;

    const unsigned char* src = nullptr;
    int srcStride = 0;
    int srcWidth = 0;
    int srcHeight = 0;

    if (surface) {
        cairo_surface_flush(surface);
        src = cairo_image_surface_get_data(surface);
        srcStride = cairo_image_surface_get_stride(surface);
        srcWidth = cairo_image_surface_get_width(surface);
        srcHeight = cairo_image_surface_get_height(surface);
    }

    for (int row = rect.y; row < rect.y + rect.height; row++) {
        int dy = content.y + row;
        if (dy < 0 || dy >= m_height) {
            continue;
        }

        uint32_t* dst = (uint32_t*)((unsigned char*)pixels + dy * stride) + content.x + rect.x;
        int copy = 0;

        if (src && row < srcHeight && rect.x < srcWidth) {
            copy = std::min(rect.width, srcWidth - rect.x);
            memcpy(dst, src + row * srcStride + rect.x * 4, copy * 4);
        }

        for (int x = copy; x < rect.width; x++) {
            dst[x] = 0xFFFFFFFF;
        }
    }
}

int BrowserWindow::present()
{
    syncModel();
    m_ui.layout(m_model);

    Tab* tab = activeTab();
    ui_rect_t content = m_ui.contentRect();
    ui_rect_t region = { 0, 0, 0, 0 };
    bool hasRegion = false;

    auto add = [&](const ui_rect_t& rect) {
        if (ui_rect_empty(&rect)) {
            return;
        }
        region = hasRegion ? ui_rect_union(&region, &rect) : rect;
        hasRegion = true;
    };

    if (m_contentDirty) {
        add({ 0, 0, content.width, content.height });
        if (tab) {
            ui_rect_t ignored;
            tab->takeDamage(ignored);
        }
    } else if (tab) {
        ui_rect_t damage;
        if (tab->takeDamage(damage)) {
            add(damage);
        }
    }

    ui_rect_t overlay = { 0, 0, 0, 0 };
    bool hasOverlay = false;

    auto addOverlay = [&](ui_rect_t rect, int inflate) {
        if (ui_rect_empty(&rect)) {
            return;
        }
        rect.x -= inflate;
        rect.y -= inflate;
        rect.width += inflate * 2;
        rect.height += inflate * 2;
        overlay = hasOverlay ? ui_rect_union(&overlay, &rect) : rect;
        hasOverlay = true;
    };

    addOverlay(m_ui.statusRect(m_model), 16);
    if (m_model.find.visible) {
        addOverlay(m_ui.findRect(), 24);
    }
    if (m_model.dropdown.visible) {
        addOverlay(m_ui.dropdownRect(), 24);
    }
    if (m_model.dialog.visible) {
        addOverlay(content, 0);
    }

    if (hasOverlay) {
        overlay.y -= content.y;
        overlay = ui_rect_clip(&overlay, content.width, content.height);
    }

    if (m_overlayDirty) {
        add(m_overlayRect);
        if (hasOverlay) {
            add(overlay);
        }
    }

    if (!m_barsDirty && !hasRegion) {
        return 0;
    }

    cairo_surface_t* target = cairo_image_surface_create_for_data((unsigned char*)ui_window_pixels(m_window), CAIRO_FORMAT_RGB24, m_width, m_height, (int)ui_window_stride(m_window));
    cairo_t* cr = cairo_create(target);

    if (m_barsDirty && m_ui.top() > 0) {
        cairo_save(cr);
        cairo_rectangle(cr, 0, 0, m_width, m_ui.top());
        cairo_clip(cr);
        m_ui.drawBars(cr, m_model);
        cairo_restore(cr);
        ui_window_damage(m_window, 0, 0, m_width, m_ui.top());
    }

    if (hasRegion) {
        region = ui_rect_clip(&region, content.width, content.height);
        cairo_surface_flush(target);
        blitContent(region);
        cairo_surface_mark_dirty(target);

        if (hasOverlay) {
            cairo_save(cr);
            cairo_rectangle(cr, content.x + region.x, content.y + region.y, region.width, region.height);
            cairo_clip(cr);
            m_ui.drawOverlays(cr, m_model);
            cairo_restore(cr);
        }

        ui_window_damage(m_window, content.x + region.x, content.y + region.y, region.width, region.height);
    }

    cairo_destroy(cr);
    cairo_surface_flush(target);
    cairo_surface_destroy(target);

    if (TextField* field = focusedField()) {
        int t = field->timeout(App::now());
        m_nextBlink = t >= 0 ? App::now() + t / 1000.0 : 0.0;
    }

    m_overlayRect = hasOverlay ? overlay : ui_rect_t { 0, 0, 0, 0 };
    m_barsDirty = false;
    m_overlayDirty = false;
    m_contentDirty = false;

    return ui_window_commit(m_window);
}

TextField* BrowserWindow::focusedField()
{
    switch (m_focus) {
    case Focus::Url:
        return &m_urlField;
    case Focus::Find:
        return &m_findField;
    case Focus::Dialog:
        for (auto& field : m_dialogFields) {
            if (field.focused()) {
                return &field;
            }
        }
        return nullptr;
    default:
        return nullptr;
    }
}

void BrowserWindow::handleEvent(const ui_event_t& event)
{
    switch (event.type) {
    case UI_EVENT_KEY:
        handleKey(event);
        break;

    case UI_EVENT_POINTER:
        handlePointer(event);
        break;

    case UI_EVENT_SCROLL:
        handleScroll(event);
        break;

    case UI_EVENT_MENU:
        handleMenu(event);
        break;

    case UI_EVENT_CONFIGURE:
        if (ui_window_apply_configure(m_window) > 0) {
            m_width = ui_window_width(m_window);
            m_height = ui_window_height(m_window);
            closeDropdown(true);
            layout();
        }
        break;

    case UI_EVENT_FOCUS:
        m_focused = event.focus.focused;
        if (m_focused) {
            App::setActiveWindow(this);
        }
        if (Tab* tab = activeTab()) {
            tab->setActive(m_focused && m_focus == Focus::Web);
        }
        if (!m_focused) {
            closeDropdown(true);
        }
        markBars();
        markOverlays();
        break;

    case UI_EVENT_LEAVE:
        if (m_hover.target == Target::Content && !m_captureContent) {
            sendPointer(APLUS_POINTER_MOVE, -1, -1, 0);
        }
        setHover({});
        m_pointerX = -1;
        m_pointerY = -1;
        markOverlays();
        updateCursor();
        break;

    case UI_EVENT_CLOSE:
        if (modal()) {
            finishDialog(m_dialogCancel);
        }
        m_closing = true;
        break;

    case UI_EVENT_THEME:
        for (Tab* tab : m_tabs) {
            if (tab->isStartPage()) {
                tab->showStartPage();
            }
        }
        layout();
        break;

    default:
        break;
    }
}

void BrowserWindow::sendPointer(int kind, int x, int y, int button)
{
    Tab* tab = activeTab();
    if (!tab) {
        return;
    }

    ui_rect_t content = m_ui.contentRect();
    AplusPointerEvent event = {};
    event.kind = (AplusPointerKind)kind;
    event.x = x < 0 ? -1 : x - content.x;
    event.y = y < 0 ? -1 : y - content.y;
    event.buttons = m_buttons;
    event.button = button;
    event.modifiers = App::modifiers();
    event.clickCount = kind == APLUS_POINTER_MOVE ? 0 : m_clickCount;
    event.timestamp = App::now();

    switch (event.kind) {
    case APLUS_POINTER_MOVE:
        tab->webView()->onMouseMotion(event);
        break;
    case APLUS_POINTER_PRESS:
        tab->webView()->onMouseButtonDown(event);
        break;
    case APLUS_POINTER_RELEASE:
        tab->webView()->onMouseButtonUp(event);
        break;
    }
}

void BrowserWindow::sendKey(uint16_t vkey, bool down, const char* text, uint8_t mods)
{
    Tab* tab = activeTab();
    if (!tab) {
        return;
    }

    AplusKeyEvent key = {};
    key.vkey = vkey;
    key.down = down;
    key.modifiers = mods;
    key.timestamp = App::now();

    if (text && text[0] && (unsigned char)text[0] >= 0x20 && (unsigned char)text[0] != 0x7f && !(mods & (APLUS_MOD_CTRL | APLUS_MOD_META))) {
        snprintf(key.text, sizeof(key.text), "%s", text);
    }

    if (down) {
        tab->webView()->onKeyDown(key);
    } else {
        tab->webView()->onKeyUp(key);
    }
}

void BrowserWindow::handlePointer(const ui_event_t& event)
{
    int x = event.pointer.x;
    int y = event.pointer.y;
    uint8_t buttons = event.pointer.buttons;

    m_pointerX = x;
    m_pointerY = y;

    Hit hit = m_ui.hitTest(m_model, x, y);
    bool wasInContent = m_hover.target == Target::Content;

    ui_rect_t status = m_ui.statusRect(m_model);
    if (status.x != m_lastStatusRect.x || status.width != m_lastStatusRect.width) {
        m_lastStatusRect = status;
        markOverlays();
    }

    if (m_captureField && (buttons & UI_BUTTON_LEFT)) {
        if (TextField* field = focusedField()) {
            field->pointerDrag(x);
            if (m_focus == Focus::Url) {
                markBars();
            } else {
                markOverlays();
            }
        }
    }

    if (m_captureContent || hit.target == Target::Content) {
        sendPointer(APLUS_POINTER_MOVE, x, y, 0);
    } else if (wasInContent) {
        sendPointer(APLUS_POINTER_MOVE, -1, -1, 0);
    }

    setHover(m_captureContent ? Hit { Target::Content, 0 } : hit);

    static const uint8_t order[] = { UI_BUTTON_LEFT, UI_BUTTON_MIDDLE, UI_BUTTON_RIGHT };
    static const int mapped[] = { APLUS_BUTTON_LEFT, APLUS_BUTTON_MIDDLE, APLUS_BUTTON_RIGHT };

    for (size_t i = 0; i < sizeof(order); i++) {
        bool was = m_buttons & order[i];
        bool is = buttons & order[i];

        if (was == is) {
            continue;
        }

        if (is) {
            double now = App::now();

            if (m_lastClickButton == mapped[i] && now - m_lastClickTime < 0.4 && std::abs(x - m_clickX) < 5 && std::abs(y - m_clickY) < 5) {
                m_clickCount++;
            } else {
                m_clickCount = 1;
            }

            m_lastClickTime = now;
            m_lastClickButton = mapped[i];
            m_clickX = x;
            m_clickY = y;
            m_buttons |= order[i];

            if (hit.target == Target::Content && !m_captureField) {
                if (m_focus != Focus::Web) {
                    focusWeb();
                }
                m_captureContent = true;
                sendPointer(APLUS_POINTER_PRESS, x, y, mapped[i]);
            } else if (!m_captureContent) {
                pressControl(hit, mapped[i], x, m_clickCount);
            }
        } else {
            m_buttons &= ~order[i];

            if (m_captureContent) {
                sendPointer(APLUS_POINTER_RELEASE, x, y, mapped[i]);
                if (mapped[i] == APLUS_BUTTON_RIGHT && hit.target == Target::Content) {
                    showContextMenu(x, y);
                }
            } else {
                releaseControl(hit, mapped[i]);
            }
        }
    }

    if (!m_buttons) {
        m_captureContent = false;
        m_captureField = false;
    }

    updateCursor();
}

void BrowserWindow::pressControl(const Hit& hit, int button, int x, int clickCount)
{
    if (button == APLUS_BUTTON_MIDDLE) {
        if (hit.target == Target::Tab || hit.target == Target::TabClose) {
            m_pressed = hit;
        }
        return;
    }

    if (button == APLUS_BUTTON_RIGHT) {
        if (hit.target == Target::Tab || hit.target == Target::TabClose) {
            showTabMenu(hit.index, m_pointerX, m_pointerY);
        }
        return;
    }

    switch (hit.target) {
    case Target::None:
        if (m_model.dropdown.visible) {
            closeDropdown(true);
        }
        break;

    case Target::Tab:
        if (hit.index >= 0 && hit.index < (int)m_tabs.size()) {
            activateTab(m_tabs[hit.index]);
        }
        break;

    case Target::Url:
        if (m_focus != Focus::Url) {
            focusUrl();
        } else {
            m_urlField.setRect(m_ui.urlTextRect(), Draw::theme().font_size);
            m_urlField.pointerDown(x, clickCount);
            m_captureField = true;
        }
        markBars();
        break;

    case Target::FindField:
        if (m_focus != Focus::Find) {
            if (Tab* tab = activeTab()) {
                tab->setActive(false);
            }
            if (m_focus == Focus::Url) {
                m_urlField.blur();
                markBars();
            }
            m_focus = Focus::Find;
            m_findField.focus(false);
        }
        m_findField.setRect(m_ui.findTextRect(), 13.5);
        m_findField.pointerDown(x, clickCount);
        m_captureField = true;
        markOverlays();
        break;

    case Target::DialogField:
        for (int i = 0; i < 2; i++) {
            if (i == hit.index) {
                m_dialogFields[i].focus(false);
            } else {
                m_dialogFields[i].blur();
            }
        }
        if (hit.index >= 0 && hit.index < 2) {
            m_dialogFields[hit.index].setRect(m_ui.dialogFieldRect(hit.index), 14.0);
            m_dialogFields[hit.index].pointerDown(x, clickCount);
            m_captureField = true;
        }
        markOverlays();
        break;

    case Target::DropdownItem:
        if (hit.index < 0) {
            break;
        }
        m_pressed = hit;
        break;

    default:
        m_pressed = hit;
        markBars();
        markOverlays();
        break;
    }
}

void BrowserWindow::releaseControl(const Hit& hit, int button)
{
    Hit pressed = m_pressed;
    m_pressed = {};

    if (pressed.target == Target::None) {
        return;
    }

    markBars();
    markOverlays();

    if (button == APLUS_BUTTON_MIDDLE) {
        if (hit.index == pressed.index && (hit.target == Target::Tab || hit.target == Target::TabClose) && hit.index < (int)m_tabs.size()) {
            closeTab(m_tabs[hit.index]);
        }
        return;
    }

    if (button != APLUS_BUTTON_LEFT || hit != pressed) {
        return;
    }

    Tab* tab = activeTab();

    switch (hit.target) {
    case Target::TabClose:
        if (hit.index >= 0 && hit.index < (int)m_tabs.size()) {
            closeTab(m_tabs[hit.index]);
        }
        break;
    case Target::NewTab:
        if (Tab* created = openTab(nullptr, false)) {
            created->showStartPage();
            focusUrl();
        }
        break;
    case Target::Back:
        if (tab && tab->canGoBack()) {
            tab->goBack();
        }
        break;
    case Target::Forward:
        if (tab && tab->canGoForward()) {
            tab->goForward();
        }
        break;
    case Target::Reload:
        if (tab) {
            if (tab->loading()) {
                tab->stop();
            } else {
                tab->reload();
            }
        }
        break;
    case Target::Menu:
        showMainMenu();
        break;
    case Target::FindPrevious:
        findNext(false);
        break;
    case Target::FindNext:
        findNext(true);
        break;
    case Target::FindClose:
        closeFind();
        break;
    case Target::DialogButton:
        finishDialog(hit.index);
        break;
    case Target::DropdownItem:
        chooseDropdown(hit.index);
        break;
    default:
        break;
    }
}

void BrowserWindow::handleScroll(const ui_event_t& event)
{
    if (modal()) {
        return;
    }

    if (m_model.dropdown.visible) {
        ui_rect_t rect = m_ui.dropdownRect();
        if (ui_rect_contains_point(&rect, m_pointerX, m_pointerY)) {
            int rows = m_ui.dropdownVisibleRows();
            int count = (int)m_model.dropdown.items.size();
            int scroll = m_model.dropdown.scroll - event.scroll.dy * 3;
            m_model.dropdown.scroll = std::max(0, std::min(scroll, count - rows));
            markOverlays();
        }
        return;
    }

    if (!m_model.fullScreen && m_pointerY >= 0 && m_pointerY < BrowserUI::tabStripHeight && m_tabs.size() > 1) {
        int next = m_active + (event.scroll.dy > 0 ? -1 : 1);
        if (event.scroll.dy != 0 && next >= 0 && next < (int)m_tabs.size()) {
            activateTab(m_tabs[next]);
        }
        return;
    }

    if (m_pointerY < m_ui.top()) {
        return;
    }

    Tab* tab = activeTab();
    if (!tab) {
        return;
    }

    ui_rect_t content = m_ui.contentRect();
    AplusScrollEvent scroll = {};
    scroll.x = m_pointerX - content.x;
    scroll.y = m_pointerY - content.y;
    scroll.dx = event.scroll.dx;
    scroll.dy = event.scroll.dy;
    scroll.modifiers = App::modifiers();
    scroll.timestamp = App::now();

    tab->webView()->onScroll(scroll);
    markBars();
}

void BrowserWindow::handleKey(const ui_event_t& event)
{
    uint16_t vkey = event.key.vkey;
    bool down = event.key.down;
    char text[8] = { 0 };

    App::noteKey(vkey, down);
    App::translate(vkey, down, text, sizeof(text) - 1);

    uint8_t mods = App::modifiers();

    if (modal()) {
        if (down) {
            handleDialogKey(vkey, down, text, mods);
        }
        return;
    }

    if (m_model.dropdown.visible) {
        if (down) {
            handleDropdownKey(vkey, text);
        }
        return;
    }

    if (down && handleShortcut(vkey, mods)) {
        return;
    }

    if (m_focus == Focus::Url) {
        TextField::Result result = m_urlField.key(vkey, down, text, mods);

        if (result == TextField::Result::Submit) {
            submitUrl();
        } else if (result == TextField::Result::Cancel) {
            revertUrl();
        }

        if (m_urlField.changed()) {
            m_urlEdited = true;
        }

        markBars();
        return;
    }

    if (m_focus == Focus::Find) {
        TextField::Result result = m_findField.key(vkey, down, text, mods);

        if (result == TextField::Result::Submit) {
            findNext(!(mods & APLUS_MOD_SHIFT));
        } else if (result == TextField::Result::Cancel) {
            closeFind();
            return;
        }

        if (m_findField.changed()) {
            findChanged();
        }

        markOverlays();
        return;
    }

    sendKey(vkey, down, text, mods);
}

bool BrowserWindow::handleShortcut(uint16_t vkey, uint8_t mods)
{
    bool ctrl = mods & (APLUS_MOD_CTRL | APLUS_MOD_META);
    bool shift = mods & APLUS_MOD_SHIFT;
    bool alt = mods & APLUS_MOD_ALT;
    Tab* tab = activeTab();

    if (ctrl && !alt) {
        switch (vkey) {
        case keyT:
            if (shift) {
                if (!m_closedTabs.empty()) {
                    std::string url = m_closedTabs.back();
                    m_closedTabs.pop_back();
                    openTab(url.c_str(), false);
                }
            } else if (Tab* created = openTab(nullptr, false)) {
                created->showStartPage();
                focusUrl();
            }
            return true;
        case keyW:
            closeTab(tab);
            return true;
        case keyN:
            App::openWindow(nullptr);
            return true;
        case keyL:
            focusUrl();
            return true;
        case keyF:
            openFind();
            return true;
        case keyG:
            if (m_model.find.visible) {
                findNext(!shift);
            } else {
                openFind();
            }
            return true;
        case keyR:
            if (tab) {
                tab->reload();
            }
            return true;
        case keyTab:
        case keyPageUp:
        case keyPageDown:
            if (m_tabs.size() > 1) {
                bool backwards = vkey == keyPageUp || (vkey == keyTab && shift);
                int count = (int)m_tabs.size();
                activateTab(m_tabs[(m_active + (backwards ? count - 1 : 1)) % count]);
            }
            return true;
        default:
            break;
        }

        if (vkey >= key1 && vkey <= key9 && !shift) {
            int index = vkey == key9 ? (int)m_tabs.size() - 1 : vkey - key1;
            if (index < (int)m_tabs.size()) {
                activateTab(m_tabs[index]);
            }
            return true;
        }
    }

    if (alt && !ctrl) {
        switch (vkey) {
        case keyD:
            focusUrl();
            return true;
        case keyHome:
            if (tab) {
                tab->showStartPage();
                focusWeb();
            }
            return true;
        case keyLeft:
            if (tab && tab->canGoBack()) {
                tab->goBack();
            }
            return true;
        case keyRight:
            if (tab && tab->canGoForward()) {
                tab->goForward();
            }
            return true;
        default:
            break;
        }
    }

    switch (vkey) {
    case keyF6:
        focusUrl();
        return true;
    case keyF5:
        if (tab) {
            tab->reload();
        }
        return true;
    case keyF3:
        if (m_model.find.visible) {
            findNext(!shift);
        } else {
            openFind();
        }
        return true;
    case keyEscape:
        if (tab && tab->fullScreen()) {
            tab->exitFullScreen();
            return true;
        }
        if (tab && tab->loading() && m_focus == Focus::Web) {
            tab->stop();
        }
        return false;
    default:
        return false;
    }
}

void BrowserWindow::focusWeb()
{
    if (m_focus == Focus::Url) {
        m_urlField.blur();
        markBars();
    } else if (m_focus == Focus::Find) {
        m_findField.blur();
        markOverlays();
    }

    m_focus = Focus::Web;
    m_urlEdited = false;

    if (Tab* tab = activeTab()) {
        tab->setActive(m_focused);
    }
}

void BrowserWindow::focusUrl()
{
    Tab* tab = activeTab();

    if (m_focus == Focus::Find) {
        m_findField.blur();
        markOverlays();
    }

    if (tab) {
        tab->setActive(false);
    }

    if (m_focus != Focus::Url) {
        m_urlField.setText(tab ? tab->url() : std::string());
        m_urlEdited = false;
    }

    m_focus = Focus::Url;
    m_urlField.setRect(m_ui.urlTextRect(), Draw::theme().font_size);
    m_urlField.focus(true);
    markBars();
}

void BrowserWindow::submitUrl()
{
    std::string text = m_urlField.text();
    focusWeb();

    if (Tab* tab = activeTab()) {
        tab->load(text);
    }
}

void BrowserWindow::revertUrl()
{
    Tab* tab = activeTab();
    m_urlField.setText(tab ? tab->url() : std::string());
    focusWeb();
}

void BrowserWindow::openFind()
{
    Tab* tab = activeTab();

    if (m_focus == Focus::Url) {
        m_urlField.blur();
        markBars();
    }

    if (tab) {
        tab->setActive(false);

        std::string selection = takeString(tab->webView()->selectedText());
        if (!selection.empty() && selection.find('\n') == std::string::npos && selection.size() < 200) {
            m_findField.setText(selection);
        }
    }

    m_model.find.visible = true;
    m_focus = Focus::Find;
    m_findField.focus(true);
    findChanged();
    markOverlays();
}

void BrowserWindow::closeFind()
{
    if (Tab* tab = activeTab()) {
        tab->webView()->unmarkAllTextMatches();
    }

    m_model.find.visible = false;
    m_model.find.matches = -1;

    if (m_focus == Focus::Find) {
        focusWeb();
    }

    markOverlays();
}

void BrowserWindow::findChanged()
{
    Tab* tab = activeTab();
    const std::string& text = m_findField.text();

    if (!tab) {
        return;
    }

    WebView* webView = tab->webView();
    webView->unmarkAllTextMatches();

    if (text.empty()) {
        m_model.find.matches = -1;
        markOverlays();
        return;
    }

    m_model.find.matches = (int)webView->markAllMatchesForText(text.c_str(), false, true, 1000);

    if (m_model.find.matches > 0) {
        webView->mainFrame()->deselectAll();
        webView->searchFor(text.c_str(), true, false, true);
    }

    markOverlays();
}

void BrowserWindow::findNext(bool forward)
{
    Tab* tab = activeTab();
    const std::string& text = m_findField.text();

    if (!tab || text.empty()) {
        return;
    }

    tab->webView()->searchFor(text.c_str(), forward, false, true);
}

void BrowserWindow::showMainMenu()
{
    Tab* tab = activeTab();
    std::vector<ui_menu_item_t> items;
    int zoom = tab ? tab->zoomPercent() : 100;

    addItem(items, MenuNewTab, "New tab");
    addItem(items, MenuNewWindow, "New window");
    addSeparator(items);
    addItem(items, MenuFind, "Find in page…");
    addSeparator(items);
    addItem(items, MenuZoomIn, "Zoom in");
    addItem(items, MenuZoomOut, "Zoom out");
    addItem(items, MenuZoomReset, "Actual size (" + std::to_string(zoom) + "%)", zoom == 100 ? UI_MENU_DISABLED : 0);
    addSeparator(items);
    addItem(items, MenuDownloads, "Downloads");
    addSeparator(items);
    addItem(items, MenuCloseWindow, "Close window");

    ui_rect_t anchor = m_ui.menuRect();

    if (ui_window_popup_menu(m_window, anchor.x + anchor.width, anchor.y + anchor.height + 4, items.data(), items.size(), &m_menuSerial) == 0) {
        m_menuKind = MenuKind::Main;
    }
}

void BrowserWindow::showTabMenu(int index, int x, int y)
{
    if (index < 0 || index >= (int)m_tabs.size()) {
        return;
    }

    std::vector<ui_menu_item_t> items;

    addItem(items, MenuTabNew, "New tab");
    addSeparator(items);
    addItem(items, MenuTabReload, "Reload");
    addItem(items, MenuTabDuplicate, "Duplicate", m_tabs[index]->url().empty() ? UI_MENU_DISABLED : 0);
    addSeparator(items);
    addItem(items, MenuTabClose, "Close tab");
    addItem(items, MenuTabCloseOthers, "Close other tabs", m_tabs.size() < 2 ? UI_MENU_DISABLED : 0);
    addItem(items, MenuTabReopen, "Reopen closed tab", m_closedTabs.empty() ? UI_MENU_DISABLED : 0);

    if (ui_window_popup_menu(m_window, x, y, items.data(), items.size(), &m_menuSerial) == 0) {
        m_menuKind = MenuKind::Tab;
        m_menuTab = index;
    }
}

void BrowserWindow::showContextMenu(int x, int y)
{
    Tab* tab = activeTab();
    if (!tab) {
        return;
    }

    ui_rect_t content = m_ui.contentRect();
    BalPoint point = { x - content.x, y - content.y };
    WebHitTestResults* result = tab->webView()->elementAtPoint(point);

    m_menuLink.clear();
    m_menuImage.clear();
    m_menuEditable = false;

    if (result) {
        m_menuLink = takeString(result->absoluteLinkURL());
        m_menuImage = takeString(result->imageURL());
        m_menuEditable = result->isContentEditable();
        delete result;
    }

    m_menuSelection = takeString(tab->webView()->selectedText());

    std::vector<ui_menu_item_t> items;

    if (!m_menuLink.empty()) {
        addItem(items, MenuOpenLinkTab, "Open link in new tab");
        addItem(items, MenuOpenLinkWindow, "Open link in new window");
        addItem(items, MenuCopyLink, "Copy link address");
        addSeparator(items);
    }

    if (!m_menuImage.empty()) {
        addItem(items, MenuOpenImageTab, "Open image in new tab");
        addItem(items, MenuCopyImage, "Copy image address");
        addSeparator(items);
    }

    if (m_menuEditable) {
        bool selected = !m_menuSelection.empty();
        addItem(items, MenuCut, "Cut", selected ? 0 : UI_MENU_DISABLED);
        addItem(items, MenuCopy, "Copy", selected ? 0 : UI_MENU_DISABLED);
        addItem(items, MenuPaste, "Paste");
        addSeparator(items);
        addItem(items, MenuSelectAll, "Select all");
    } else if (!m_menuSelection.empty()) {
        addItem(items, MenuCopy, "Copy");
        addItem(items, MenuSearchSelection, "Search for “" + shorten(m_menuSelection, 22) + "”");
    } else if (m_menuLink.empty() && m_menuImage.empty()) {
        addItem(items, MenuBack, "Back", tab->canGoBack() ? 0 : UI_MENU_DISABLED);
        addItem(items, MenuForward, "Forward", tab->canGoForward() ? 0 : UI_MENU_DISABLED);
        addItem(items, MenuReload, "Reload");
        addSeparator(items);
        addItem(items, MenuSelectAll, "Select all");
        addItem(items, MenuFind, "Find in page…");
    }

    while (!items.empty() && (items.back().flags & UI_MENU_SEPARATOR)) {
        items.pop_back();
    }

    if (items.empty()) {
        return;
    }

    if (ui_window_popup_menu(m_window, x, y, items.data(), items.size(), &m_menuSerial) == 0) {
        m_menuKind = MenuKind::Context;
    }
}

void BrowserWindow::handleMenu(const ui_event_t& event)
{
    if (event.menu.serial != m_menuSerial || m_menuKind == MenuKind::None) {
        return;
    }

    MenuKind kind = m_menuKind;
    m_menuKind = MenuKind::None;

    if (event.menu.id != 0) {
        runMenuAction(kind, event.menu.id);
    }
}

void BrowserWindow::runMenuAction(MenuKind kind, uint32_t id)
{
    Tab* tab = activeTab();

    if (kind == MenuKind::Tab) {
        Tab* target = m_menuTab >= 0 && m_menuTab < (int)m_tabs.size() ? m_tabs[m_menuTab] : nullptr;

        switch (id) {
        case MenuTabNew:
            if (Tab* created = openTab(nullptr, false)) {
                created->showStartPage();
                focusUrl();
            }
            break;
        case MenuTabReload:
            if (target) {
                target->reload();
            }
            break;
        case MenuTabDuplicate:
            if (target && !target->url().empty()) {
                activateTab(target);
                openTab(target->url().c_str(), false);
            }
            break;
        case MenuTabClose:
            closeTab(target);
            break;
        case MenuTabCloseOthers:
            for (Tab* other : m_tabs) {
                if (other != target) {
                    closeTab(other);
                }
            }
            if (target) {
                activateTab(target);
            }
            break;
        case MenuTabReopen:
            if (!m_closedTabs.empty()) {
                std::string url = m_closedTabs.back();
                m_closedTabs.pop_back();
                openTab(url.c_str(), false);
            }
            break;
        default:
            break;
        }
        return;
    }

    switch (id) {
    case MenuNewTab:
        if (Tab* created = openTab(nullptr, false)) {
            created->showStartPage();
            focusUrl();
        }
        break;
    case MenuNewWindow:
        App::openWindow(nullptr);
        break;
    case MenuFind:
        openFind();
        break;
    case MenuZoomIn:
        if (tab) {
            tab->zoomIn();
        }
        break;
    case MenuZoomOut:
        if (tab) {
            tab->zoomOut();
        }
        break;
    case MenuZoomReset:
        if (tab) {
            tab->resetZoom();
        }
        break;
    case MenuDownloads:
        App::openExternal(App::downloadsDirectory().c_str());
        break;
    case MenuCloseWindow:
        m_closing = true;
        break;
    case MenuOpenLinkTab:
        openTab(m_menuLink.c_str(), true);
        flash("Opened in a new tab", 2.0);
        break;
    case MenuOpenLinkWindow:
        App::openWindow(m_menuLink.c_str());
        break;
    case MenuCopyLink:
        App::setClipboard(m_menuLink);
        break;
    case MenuOpenImageTab:
        openTab(m_menuImage.c_str(), false);
        break;
    case MenuCopyImage:
        App::setClipboard(m_menuImage);
        break;
    case MenuCopy:
        if (tab) {
            tab->webView()->copy();
        }
        break;
    case MenuSearchSelection:
        openTab(("https://duckduckgo.com/html/?q=" + urlEncode(m_menuSelection)).c_str(), false);
        break;
    case MenuCut:
        if (tab) {
            tab->webView()->cut();
        }
        break;
    case MenuPaste:
        if (tab) {
            tab->webView()->paste();
        }
        break;
    case MenuSelectAll:
        if (tab) {
            tab->webView()->mainFrame()->selectAll();
        }
        break;
    case MenuBack:
        if (tab) {
            tab->goBack();
        }
        break;
    case MenuForward:
        if (tab) {
            tab->goForward();
        }
        break;
    case MenuReload:
        if (tab) {
            tab->reload();
        }
        break;
    default:
        break;
    }
}

int BrowserWindow::runDialog(const std::string& title, const std::string& message, const std::vector<std::string>& fields, const std::vector<std::string>& placeholders,
    const std::vector<bool>& secret, const std::vector<std::string>& buttons, int defaultButton, int cancelButton, std::vector<std::string>* values)
{
    if (modal() || m_closing) {
        return cancelButton;
    }

    Focus previous = m_focus;
    closeDropdown(true);

    if (m_focus == Focus::Url) {
        m_urlField.blur();
    } else if (m_focus == Focus::Find) {
        m_findField.blur();
    }

    if (Tab* tab = activeTab()) {
        tab->setActive(false);
    }

    DialogModel& dialog = m_model.dialog;
    dialog = DialogModel();
    dialog.visible = true;
    dialog.title = title;
    dialog.message = message;
    dialog.buttons = buttons;
    dialog.defaultButton = defaultButton;
    dialog.placeholders = placeholders;

    for (size_t i = 0; i < fields.size() && i < 2; i++) {
        m_dialogFields[i].setText(fields[i]);
        m_dialogFields[i].setPassword(i < secret.size() && secret[i]);
        m_dialogFields[i].blur();
        dialog.fields.push_back(&m_dialogFields[i]);
    }

    if (!dialog.fields.empty()) {
        m_dialogFields[0].focus(true);
    }

    m_focus = Focus::Dialog;
    m_dialogResult = -1;
    m_dialogCancel = cancelButton;
    m_dialogDefault = defaultButton;
    m_captureContent = false;
    m_buttons = 0;

    for (auto& anim : m_dialogHover) {
        anim = Anim();
    }

    layout();
    App::runModal(this, &m_dialogResult);

    int result = m_dialogResult < 0 ? cancelButton : m_dialogResult;

    if (values) {
        values->clear();
        for (size_t i = 0; i < dialog.fields.size(); i++) {
            values->push_back(m_dialogFields[i].text());
        }
    }

    for (auto& field : m_dialogFields) {
        field.blur();
        field.setText(std::string());
        field.setPassword(false);
    }

    m_model.dialog = DialogModel();
    m_dialogTab = nullptr;
    m_focus = Focus::Web;

    if (previous == Focus::Find && m_model.find.visible) {
        m_focus = Focus::Find;
        m_findField.focus(false);
    } else if (Tab* tab = activeTab()) {
        tab->setActive(m_focused);
    }

    m_contentDirty = true;
    markBars();
    markOverlays();
    return result;
}

void BrowserWindow::finishDialog(int button)
{
    if (!modal()) {
        return;
    }

    m_dialogResult = std::max(0, std::min(button, (int)m_model.dialog.buttons.size() - 1));
}

void BrowserWindow::handleDialogKey(uint16_t vkey, bool down, const char* text, uint8_t mods)
{
    if (vkey == keyEscape) {
        finishDialog(m_dialogCancel);
        return;
    }

    if (vkey == keyEnter || vkey == keyKeypadEnter) {
        finishDialog(m_dialogDefault);
        return;
    }

    size_t count = m_model.dialog.fields.size();

    if (vkey == keyTab && count > 1) {
        int current = m_dialogFields[0].focused() ? 0 : 1;
        int next = (current + ((mods & APLUS_MOD_SHIFT) ? (int)count - 1 : 1)) % (int)count;
        m_dialogFields[current].blur();
        m_dialogFields[next].focus(true);
        markOverlays();
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (m_dialogFields[i].focused()) {
            m_dialogFields[i].setRect(m_ui.dialogFieldRect(i), 14.0);
            m_dialogFields[i].key(vkey, down, text, mods);
            m_dialogFields[i].changed();
            markOverlays();
            return;
        }
    }
}

int BrowserWindow::alert(Tab* tab, const std::string& title, const std::string& message)
{
    m_dialogTab = tab;
    return runDialog(title, message, {}, {}, {}, { "OK" }, 0, 0, nullptr);
}

bool BrowserWindow::confirm(Tab* tab, const std::string& title, const std::string& message)
{
    m_dialogTab = tab;
    return runDialog(title, message, {}, {}, {}, { "Cancel", "OK" }, 1, 0, nullptr) == 1;
}

bool BrowserWindow::prompt(Tab* tab, const std::string& title, const std::string& message, const std::string& value, std::string& result)
{
    std::vector<std::string> values;
    m_dialogTab = tab;

    if (runDialog(title, message, { value }, { "" }, { false }, { "Cancel", "OK" }, 1, 0, &values) != 1) {
        return false;
    }

    result = values.empty() ? std::string() : values[0];
    return true;
}

bool BrowserWindow::login(Tab* tab, const std::string& host, const std::string& realm, std::string& username, std::string& password)
{
    std::vector<std::string> values;
    std::string message = realm.empty() ? host + " requires a username and password." : host + " requires a username and password for “" + realm + "”.";
    m_dialogTab = tab;

    if (runDialog("Sign in", message, { username, std::string() }, { "Username", "Password" }, { false, true }, { "Cancel", "Sign in" }, 1, 0, &values) != 1) {
        return false;
    }

    username = values.size() > 0 ? values[0] : std::string();
    password = values.size() > 1 ? values[1] : std::string();
    return true;
}

void BrowserWindow::showDropdown(WebCore::PopupMenuMorphOS* popup)
{
    if (!popup || !popup->client() || modal()) {
        if (popup && popup->client()) {
            popup->client()->popupDidHide();
        }
        return;
    }

    closeDropdown(true);

    WebCore::PopupMenuClient* client = popup->client();
    DropdownModel& dd = m_model.dropdown;
    dd = DropdownModel();

    int size = client->listSize();
    for (int i = 0; i < size; i++) {
        DropdownItem item;
        item.text = client->itemText(i).stripWhiteSpace().utf8().data();
        item.separator = client->itemIsSeparator(i);
        item.label = client->itemIsLabel(i);
        item.enabled = client->itemIsEnabled(i);
        dd.items.push_back(item);
    }

    if (dd.items.empty()) {
        client->popupDidHide();
        return;
    }

    const WebCore::IntRect& anchor = popup->anchorRect();
    ui_rect_t content = m_ui.contentRect();

    dd.anchor = { content.x + anchor.x(), content.y + anchor.y(), anchor.width(), anchor.height() };
    dd.selected = client->selectedIndex();
    dd.highlighted = dd.selected;
    dd.visible = true;

    if (dd.highlighted < 0 || dd.highlighted >= size || !dd.items[dd.highlighted].enabled || dd.items[dd.highlighted].label || dd.items[dd.highlighted].separator) {
        dd.highlighted = -1;
        for (int i = 0; i < size; i++) {
            if (dd.items[i].enabled && !dd.items[i].label && !dd.items[i].separator) {
                dd.highlighted = i;
                break;
            }
        }
    }

    m_popup = popup;
    m_ui.layout(m_model);

    int rows = m_ui.dropdownVisibleRows();
    int focus = std::max(0, dd.highlighted);
    dd.scroll = std::max(0, std::min(focus - rows / 2, size - rows));

    markOverlays();
}

void BrowserWindow::hideDropdown(WebCore::PopupMenuMorphOS* popup)
{
    if (m_popup && m_popup.get() == popup) {
        closeDropdown(true);
    }
}

void BrowserWindow::closeDropdown(bool notify)
{
    if (!m_popup && !m_model.dropdown.visible) {
        return;
    }

    RefPtr<WebCore::PopupMenuMorphOS> popup = WTFMove(m_popup);
    m_popup = nullptr;
    m_model.dropdown = DropdownModel();
    markOverlays();

    if (notify && popup && popup->client()) {
        popup->client()->popupDidHide();
    }
}

void BrowserWindow::moveDropdown(int index)
{
    DropdownModel& dd = m_model.dropdown;
    int count = (int)dd.items.size();

    if (!count) {
        return;
    }

    index = std::max(0, std::min(index, count - 1));
    int direction = index >= dd.highlighted ? 1 : -1;

    while (index >= 0 && index < count && (!dd.items[index].enabled || dd.items[index].label || dd.items[index].separator)) {
        index += direction;
    }

    if (index < 0 || index >= count) {
        return;
    }

    dd.highlighted = index;

    int rows = m_ui.dropdownVisibleRows();
    if (index < dd.scroll) {
        dd.scroll = index;
    } else if (index >= dd.scroll + rows) {
        dd.scroll = index - rows + 1;
    }

    markOverlays();
}

void BrowserWindow::chooseDropdown(int index)
{
    DropdownModel& dd = m_model.dropdown;

    if (index < 0 || index >= (int)dd.items.size()) {
        return;
    }

    const DropdownItem& item = dd.items[index];
    if (!item.enabled || item.label || item.separator) {
        return;
    }

    RefPtr<WebCore::PopupMenuMorphOS> popup = m_popup;

    if (popup && popup->client()) {
        popup->client()->valueChanged(index);
    }

    if (m_popup == popup) {
        closeDropdown(true);
    }
}

void BrowserWindow::handleDropdownKey(uint16_t vkey, const char* text)
{
    DropdownModel& dd = m_model.dropdown;
    int rows = std::max(1, m_ui.dropdownVisibleRows());

    switch (vkey) {
    case keyUp:
        moveDropdown(dd.highlighted - 1);
        return;
    case keyDown:
        moveDropdown(dd.highlighted + 1);
        return;
    case keyPageUp:
        moveDropdown(dd.highlighted - rows);
        return;
    case keyPageDown:
        moveDropdown(dd.highlighted + rows);
        return;
    case keyHome:
        dd.highlighted = -1;
        moveDropdown(0);
        return;
    case keyEnd:
        dd.highlighted = -1;
        moveDropdown((int)dd.items.size() - 1);
        return;
    case keyEnter:
    case keyKeypadEnter:
    case keySpace:
        chooseDropdown(dd.highlighted);
        return;
    case keyEscape:
    case keyTab:
        closeDropdown(true);
        return;
    default:
        break;
    }

    if (!text || !text[0] || (unsigned char)text[0] < 0x20) {
        return;
    }

    int count = (int)dd.items.size();
    int start = dd.highlighted;

    for (int n = 1; n <= count; n++) {
        int i = (start + n + count) % count;
        const DropdownItem& item = dd.items[i];

        if (item.enabled && !item.label && !item.separator && !item.text.empty() && tolower((unsigned char)item.text[0]) == tolower((unsigned char)text[0])) {
            dd.highlighted = i - 1;
            moveDropdown(i);
            return;
        }
    }
}
