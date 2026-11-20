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

#include "BrowserUI.h"

#include "Glyphs.h"

#include <algorithm>
#include <cmath>

static constexpr int buttonSize = 34;
static constexpr int buttonGap = 4;
static constexpr int margin = 8;
static constexpr int tabTop = 6;
static constexpr int tabMaxWidth = 232;
static constexpr int tabMinWidth = 40;
static constexpr int newTabSize = 28;
static constexpr int findWidth = 400;
static constexpr int findHeight = 44;
static constexpr int dialogWidth = 440;
static constexpr int dialogPadding = 24;
static constexpr int dialogFieldHeight = 36;
static constexpr int dialogButtonHeight = 36;
static constexpr int dropdownPadding = 6;
static constexpr int dropdownMaxRows = 12;
static constexpr double tabFontSize = 13.0;
static constexpr double statusFontSize = 12.5;

static bool contains(const ui_rect_t& rect, int x, int y)
{
    return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
}

static double clamp01(double v)
{
    return std::max(0.0, std::min(1.0, v));
}

void BrowserUI::layout(const UIModel& model)
{
    m_width = model.width;
    m_height = model.height;
    m_top = model.fullScreen ? 0 : contentTop;
    m_content = { 0, m_top, m_width, std::max(1, m_height - m_top) };

    int count = (int)model.tabs.size();
    int available = m_width - margin * 2 - newTabSize - 6;
    int tabWidth = count ? std::max(tabMinWidth, std::min(tabMaxWidth, available / count)) : 0;

    m_tabs.clear();
    m_tabCloses.clear();

    for (int i = 0; i < count; i++) {
        int x = margin + i * tabWidth;
        m_tabs.push_back({ x, tabTop, tabWidth, tabStripHeight - tabTop });
        m_tabCloses.push_back({ x + tabWidth - 30, tabTop + 6, 22, 22 });
    }

    m_newTab = { margin + count * tabWidth + 6, tabTop + 3, newTabSize, newTabSize };

    int by = tabStripHeight + (toolbarHeight - buttonSize) / 2;
    m_back = { margin, by, buttonSize, buttonSize };
    m_forward = { m_back.x + buttonSize + buttonGap, by, buttonSize, buttonSize };
    m_reload = { m_forward.x + buttonSize + buttonGap, by, buttonSize, buttonSize };
    m_menu = { m_width - margin - buttonSize, by, buttonSize, buttonSize };

    int urlX = m_reload.x + buttonSize + margin;
    m_url = { urlX, by, std::max(80, m_menu.x - margin - urlX), buttonSize };
    m_urlText = { m_url.x + 36, m_url.y, std::max(10, m_url.width - 36 - 14), m_url.height };

    int fw = std::min(findWidth, m_width - 24);
    m_find = { m_width - fw - 12, m_top + 10, fw, findHeight };
    m_findButtons[2] = { m_find.x + fw - 6 - 32, m_find.y + 6, 32, 32 };
    m_findButtons[1] = { m_findButtons[2].x - 34, m_find.y + 6, 32, 32 };
    m_findButtons[0] = { m_findButtons[1].x - 34, m_find.y + 6, 32, 32 };
    m_findBox = { m_find.x + 6, m_find.y + 6, std::max(40, m_findButtons[0].x - 8 - (m_find.x + 6)), 32 };
    m_findText = { m_findBox.x + 30, m_findBox.y, std::max(10, m_findBox.width - 30 - 76), 32 };

    m_dialogFields.clear();
    m_dialogFieldText.clear();
    m_dialogButtons.clear();
    m_dialogLines.clear();

    if (model.dialog.visible) {
        int w = std::min(dialogWidth, m_width - 48);
        m_dialogLines = wrap(model.dialog.message, 14.0, w - dialogPadding * 2);

        int lines = std::min((int)m_dialogLines.size(), 12);
        int h = dialogPadding + 24 + (lines ? 10 + lines * 21 : 0);
        if (!model.dialog.fields.empty()) {
            h += 16 + (int)model.dialog.fields.size() * (dialogFieldHeight + 10) - 10;
        }
        h += 24 + dialogButtonHeight + 20;

        int x = (m_width - w) / 2;
        int y = m_top + std::max(24, (m_content.height - h) / 3);
        m_dialog = { x, y, w, h };

        int cy = y + dialogPadding + 24 + (lines ? 10 + lines * 21 : 0) + 16;
        for (size_t i = 0; i < model.dialog.fields.size(); i++) {
            ui_rect_t field = { x + dialogPadding, cy, w - dialogPadding * 2, dialogFieldHeight };
            m_dialogFields.push_back(field);
            m_dialogFieldText.push_back({ field.x + 12, field.y, field.width - 24, field.height });
            cy += dialogFieldHeight + 10;
        }

        int bx = x + w - dialogPadding;
        int bottom = y + h - 20 - dialogButtonHeight;
        for (size_t i = model.dialog.buttons.size(); i-- > 0;) {
            int bw = std::max(88, (int)Draw::textWidth(model.dialog.buttons[i], Draw::Weight::Medium, 14.0) + 36);
            bx -= bw;
            m_dialogButtons.insert(m_dialogButtons.begin(), { bx, bottom, bw, dialogButtonHeight });
            bx -= 8;
        }
    } else {
        m_dialog = { 0, 0, 0, 0 };
    }

    if (model.dropdown.visible && !model.dropdown.items.empty()) {
        const DropdownModel& dd = model.dropdown;
        int rows = (int)dd.items.size();
        double textMax = 0;
        for (const auto& item : dd.items) {
            textMax = std::max(textMax, Draw::textWidth(item.text, item.label ? Draw::Weight::Medium : Draw::Weight::Regular, 14.0));
        }

        int w = std::min(m_width - 16, std::max(dd.anchor.width, (int)textMax + 56));
        int below = (m_content.y + m_content.height) - (dd.anchor.y + dd.anchor.height) - 8;
        int above = dd.anchor.y - m_content.y - 8;
        int space = std::max(below, above);
        int fit = std::max(1, (space - dropdownPadding * 2) / dropdownRowHeight);

        m_dropdownRows = std::min({ rows, fit, dropdownMaxRows });

        int h = m_dropdownRows * dropdownRowHeight + dropdownPadding * 2;
        int x = std::max(8, std::min(dd.anchor.x, m_width - w - 8));
        int y = (below >= h || below >= above) ? dd.anchor.y + dd.anchor.height + 4 : dd.anchor.y - h - 4;

        m_dropdown = { x, y, w, h };
    } else {
        m_dropdown = { 0, 0, 0, 0 };
        m_dropdownRows = 0;
    }
}

Hit BrowserUI::hitTest(const UIModel& model, int x, int y) const
{
    if (model.dialog.visible) {
        for (size_t i = 0; i < m_dialogFields.size(); i++) {
            if (contains(m_dialogFields[i], x, y)) {
                return { Target::DialogField, (int)i };
            }
        }
        for (size_t i = 0; i < m_dialogButtons.size(); i++) {
            if (contains(m_dialogButtons[i], x, y)) {
                return { Target::DialogButton, (int)i };
            }
        }
        return {};
    }

    if (model.dropdown.visible) {
        if (contains(m_dropdown, x, y)) {
            int row = (y - m_dropdown.y - dropdownPadding) / dropdownRowHeight;
            if (row >= 0 && row < m_dropdownRows) {
                return { Target::DropdownItem, row + model.dropdown.scroll };
            }
            return { Target::DropdownItem, -1 };
        }
        return {};
    }

    if (model.find.visible && contains(m_find, x, y)) {
        if (contains(m_findButtons[0], x, y)) {
            return { Target::FindPrevious, 0 };
        }
        if (contains(m_findButtons[1], x, y)) {
            return { Target::FindNext, 0 };
        }
        if (contains(m_findButtons[2], x, y)) {
            return { Target::FindClose, 0 };
        }
        return { Target::FindField, 0 };
    }

    if (y >= m_top) {
        return { Target::Content, 0 };
    }

    for (size_t i = 0; i < m_tabs.size(); i++) {
        if (contains(m_tabCloses[i], x, y) && m_tabs[i].width >= 72) {
            return { Target::TabClose, (int)i };
        }
        if (contains(m_tabs[i], x, y)) {
            return { Target::Tab, (int)i };
        }
    }

    if (contains(m_newTab, x, y)) {
        return { Target::NewTab, 0 };
    }
    if (contains(m_back, x, y)) {
        return { Target::Back, 0 };
    }
    if (contains(m_forward, x, y)) {
        return { Target::Forward, 0 };
    }
    if (contains(m_reload, x, y)) {
        return { Target::Reload, 0 };
    }
    if (contains(m_url, x, y)) {
        return { Target::Url, 0 };
    }
    if (contains(m_menu, x, y)) {
        return { Target::Menu, 0 };
    }

    return {};
}

void BrowserUI::drawButton(cairo_t* cr, const ui_rect_t& rect, double hover, bool pressed, double radius)
{
    const ui_theme_t& theme = Draw::theme();

    if (pressed) {
        Draw::roundedRect(cr, rect, radius);
        Draw::setColor(cr, theme.active);
        cairo_fill(cr);
    } else if (hover > 0.01) {
        Draw::roundedRect(cr, rect, radius);
        Draw::setColor(cr, theme.hover, hover);
        cairo_fill(cr);
    }
}

static void tabShape(cairo_t* cr, const ui_rect_t& rect, double radius, double fillet)
{
    double x = rect.x;
    double y = rect.y;
    double w = rect.width;
    double bottom = rect.y + rect.height;

    cairo_new_path(cr);
    cairo_move_to(cr, x - fillet, bottom);
    cairo_arc_negative(cr, x - fillet, bottom - fillet, fillet, M_PI / 2.0, 0);
    cairo_line_to(cr, x, y + radius);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3.0 * M_PI / 2.0);
    cairo_line_to(cr, x + w - radius, y);
    cairo_arc(cr, x + w - radius, y + radius, radius, 3.0 * M_PI / 2.0, 2.0 * M_PI);
    cairo_line_to(cr, x + w, bottom - fillet);
    cairo_arc_negative(cr, x + w + fillet, bottom - fillet, fillet, M_PI, M_PI / 2.0);
    cairo_close_path(cr);
}

void BrowserUI::drawTabs(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    const double radius = theme.corner_radius;

    for (size_t i = 0; i < m_tabs.size(); i++) {
        const ui_rect_t& tab = m_tabs[i];
        const TabModel& tm = model.tabs[i];
        bool active = (int)i == model.activeTab;

        if (active) {
            tabShape(cr, tab, radius, radius);
            Draw::setColor(cr, theme.surface);
            cairo_fill(cr);
        } else if (tm.hover > 0.01) {
            Draw::roundedRect(cr, tab.x + 2, tab.y + 3, tab.width - 4, tab.height - 9, radius);
            Draw::setColor(cr, theme.hover, tm.hover);
            cairo_fill(cr);
        }

        bool nextActive = (int)i + 1 == model.activeTab;
        bool nextHover = i + 1 < m_tabs.size() && model.tabs[i + 1].hover > 0.5;
        if (!active && !nextActive && tm.hover < 0.5 && !nextHover && i + 1 < m_tabs.size()) {
            cairo_rectangle(cr, tab.x + tab.width - 0.5, tab.y + 10, 1, tab.height - 22);
            Draw::setColor(cr, theme.text_muted, 0.25);
            cairo_fill(cr);
        }

        bool showClose = tab.width >= 72 && (active || tm.hover > 0.01);
        double contentY = tab.y + 2;
        double contentH = tab.height - 8;
        double iconX = tab.x + 12 + 8;
        double iconY = contentY + contentH / 2.0;

        if (tab.width < 72) {
            iconX = tab.x + tab.width / 2.0;
        }

        if (tm.loading) {
            Glyph::spinner(cr, iconX, iconY, 15, std::fmod(model.now * 1.1, 1.0), theme.primary);
        } else {
            Glyph::draw(cr, Glyph::Kind::Globe, iconX, iconY, 15, active ? theme.text : theme.text_muted, active ? 0.75 : 0.7);
        }

        if (tab.width >= 72) {
            double textX = tab.x + 36;
            double textRight = tab.x + tab.width - (showClose ? 34 : 14);
            std::string title = tm.title.empty() ? std::string("New Tab") : tm.title;
            std::string shown = Draw::ellipsize(title, Draw::Weight::Regular, tabFontSize, textRight - textX);
            Draw::text(cr, shown, textX, contentY, contentH, Draw::Weight::Regular, tabFontSize, active ? theme.text : theme.text_muted);
        }

        if (showClose) {
            const ui_rect_t& close = m_tabCloses[i];
            double alpha = active ? 1.0 : clamp01(tm.hover * 1.4);

            if (tm.closeHover > 0.01) {
                cairo_new_path(cr);
                cairo_arc(cr, close.x + close.width / 2.0, close.y + close.height / 2.0, close.width / 2.0, 0, 2.0 * M_PI);
                Draw::setColor(cr, theme.hover, tm.closeHover * 1.6);
                cairo_fill(cr);
            }

            Glyph::draw(cr, Glyph::Kind::Close, close.x + close.width / 2.0, close.y + close.height / 2.0, 15, Draw::mix(theme.text_muted, theme.text, tm.closeHover), alpha);
        }
    }

    drawButton(cr, m_newTab, model.newTabHover, model.pressed == Target::NewTab, radius);
    Glyph::draw(cr, Glyph::Kind::Plus, m_newTab.x + m_newTab.width / 2.0, m_newTab.y + m_newTab.height / 2.0, 15, theme.text_muted);
}

void BrowserUI::drawToolbar(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    const double radius = theme.corner_radius;

    cairo_rectangle(cr, 0, tabStripHeight, m_width, toolbarHeight);
    Draw::setColor(cr, theme.surface);
    cairo_fill(cr);

    cairo_rectangle(cr, 0, contentTop - 1, m_width, 1);
    Draw::setColor(cr, Draw::isLight() ? ui_color_t { 0, 0, 0, 0.12 } : ui_color_t { 0, 0, 0, 0.45 });
    cairo_fill(cr);

    struct Item {
        const ui_rect_t* rect;
        Glyph::Kind kind;
        double hover;
        Target target;
        bool enabled;
    };

    Item items[] = {
        { &m_back, Glyph::Kind::Back, model.hoverBack, Target::Back, model.canGoBack },
        { &m_forward, Glyph::Kind::Forward, model.hoverForward, Target::Forward, model.canGoForward },
        { &m_reload, model.loading ? Glyph::Kind::Stop : Glyph::Kind::Reload, model.hoverReload, Target::Reload, true },
        { &m_menu, Glyph::Kind::Menu, model.hoverMenu, Target::Menu, true },
    };

    for (const auto& item : items) {
        if (item.enabled) {
            drawButton(cr, *item.rect, item.hover, model.pressed == item.target, radius);
        }
        Glyph::draw(cr, item.kind, item.rect->x + item.rect->width / 2.0, item.rect->y + item.rect->height / 2.0, 17, theme.text, item.enabled ? 0.9 : 0.3);
    }

    drawUrl(cr, model);

    if (model.progressAlpha > 0.01) {
        double w = std::round(m_width * clamp01(model.progress));
        cairo_rectangle(cr, 0, contentTop - 2, w, 2);
        Draw::setColor(cr, theme.focus_ring, model.progressAlpha);
        cairo_fill(cr);
    }
}

struct UrlParts {
    std::string scheme;
    std::string host;
    std::string rest;
};

static bool validUtf8(const std::string& text)
{
    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = text[i];
        int extra = c < 0x80 ? 0 : (c & 0xe0) == 0xc0 ? 1 : (c & 0xf0) == 0xe0 ? 2 : (c & 0xf8) == 0xf0 ? 3 : -1;
        if (extra < 0 || (extra == 1 && c < 0xc2)) {
            return false;
        }
        for (int k = 1; k <= extra; k++) {
            if (i + k >= text.size() || ((unsigned char)text[i + k] & 0xc0) != 0x80) {
                return false;
            }
        }
        i += extra + 1;
    }
    return true;
}

static int hexValue(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static std::string decodeForDisplay(const std::string& text)
{
    std::string out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '%' && i + 2 < text.size()) {
            int hi = hexValue(text[i + 1]);
            int lo = hexValue(text[i + 2]);
            if (hi >= 0 && lo >= 0 && hi * 16 + lo >= 0x80) {
                out.push_back((char)(hi * 16 + lo));
                i += 2;
                continue;
            }
        }
        out.push_back(text[i]);
    }

    return validUtf8(out) ? out : text;
}

static UrlParts splitUrl(const std::string& url)
{
    UrlParts parts;
    size_t sep = url.find("://");

    if (sep == std::string::npos) {
        parts.rest = url;
        return parts;
    }

    parts.scheme = url.substr(0, sep);
    size_t start = sep + 3;
    size_t end = url.find_first_of("/?#", start);
    if (end == std::string::npos) {
        end = url.size();
    }

    std::string authority = url.substr(start, end - start);
    size_t at = authority.rfind('@');
    if (at != std::string::npos) {
        authority = authority.substr(at + 1);
    }

    parts.host = authority;
    parts.rest = decodeForDisplay(url.substr(end));
    if (parts.rest == "/") {
        parts.rest.clear();
    }
    return parts;
}

void BrowserUI::drawUrl(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    const double radius = theme.corner_radius;
    const double size = theme.font_size;
    bool focused = model.urlField && model.urlField->focused();

    Draw::roundedRect(cr, m_url, radius);
    Draw::setColor(cr, Draw::isLight() && !focused ? Draw::mix(theme.background, theme.text, 0.035 * (1.0 - model.hoverUrl)) : theme.surface_sunken);
    cairo_fill(cr);

    if (focused) {
        Draw::roundedRect(cr, m_url.x + 1, m_url.y + 1, m_url.width - 2, m_url.height - 2, radius - 1);
        Draw::setColor(cr, theme.focus_ring);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke(cr);
    } else {
        Draw::roundedRect(cr, m_url.x + 0.5, m_url.y + 0.5, m_url.width - 1, m_url.height - 1, radius);
        Draw::setColor(cr, Draw::mix(theme.border, Draw::withAlpha(theme.text, 0.22), model.hoverUrl));
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    UrlParts parts = splitUrl(model.url);
    Glyph::Kind icon = Glyph::Kind::Search;

    if (!focused && !model.url.empty()) {
        if (parts.scheme == "https") {
            icon = Glyph::Kind::Lock;
        } else if (parts.scheme == "http") {
            icon = Glyph::Kind::Warning;
        } else {
            icon = Glyph::Kind::Globe;
        }
    }

    Glyph::draw(cr, icon, m_url.x + 19, m_url.y + m_url.height / 2.0, 15, theme.text_muted, 0.9);

    if (focused) {
        model.urlField->draw(cr, m_urlText, size, theme.text, model.now);
        return;
    }

    cairo_save(cr);
    cairo_rectangle(cr, m_urlText.x, m_urlText.y, m_urlText.width, m_urlText.height);
    cairo_clip(cr);

    if (model.url.empty()) {
        Draw::text(cr, "Search or enter address", m_urlText.x, m_urlText.y, m_urlText.height, Draw::Weight::Regular, size, theme.text_muted);
    } else if (parts.host.empty()) {
        Draw::text(cr, Draw::ellipsize(model.url, Draw::Weight::Regular, size, m_urlText.width), m_urlText.x, m_urlText.y, m_urlText.height, Draw::Weight::Regular, size, theme.text);
    } else {
        double x = m_urlText.x;
        std::string prefix = parts.scheme == "https" ? std::string() : parts.scheme + "://";

        Draw::text(cr, prefix, x, m_urlText.y, m_urlText.height, Draw::Weight::Regular, size, theme.text_muted);
        x += Draw::textWidth(prefix, Draw::Weight::Regular, size);

        Draw::text(cr, parts.host, x, m_urlText.y, m_urlText.height, Draw::Weight::Regular, size, theme.text);
        x += Draw::textWidth(parts.host, Draw::Weight::Regular, size);

        double room = m_urlText.x + m_urlText.width - x;
        Draw::text(cr, Draw::ellipsize(parts.rest, Draw::Weight::Regular, size, room), x, m_urlText.y, m_urlText.height, Draw::Weight::Regular, size, theme.text_muted);
    }

    cairo_restore(cr);
}

void BrowserUI::drawBars(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();

    cairo_rectangle(cr, 0, 0, m_width, tabStripHeight);
    Draw::setColor(cr, theme.background);
    cairo_fill(cr);

    drawToolbar(cr, model);
    drawTabs(cr, model);
}

ui_rect_t BrowserUI::statusRect(const UIModel& model) const
{
    if (model.status.empty()) {
        return { 0, 0, 0, 0 };
    }

    int maxWidth = std::max(120, m_width * 3 / 5);
    int w = std::min(maxWidth, (int)Draw::textWidth(model.status, Draw::Weight::Regular, statusFontSize) + 24);
    int h = 26;
    int y = m_content.y + m_content.height - h - 8;
    ui_rect_t rect = { 8, y, w, h };

    if (model.pointerY >= y - 24 && model.pointerX >= 0 && model.pointerX < rect.x + rect.width + 24) {
        rect.x = m_width - w - 8;
    }

    return rect;
}

void BrowserUI::drawStatus(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    ui_rect_t rect = statusRect(model);

    if (rect.width <= 0) {
        return;
    }

    Draw::shadow(cr, rect, 8, 6, 0.30);
    Draw::roundedRect(cr, rect, 8);
    Draw::setColor(cr, theme.surface);
    cairo_fill(cr);
    Draw::roundedRect(cr, rect.x + 0.5, rect.y + 0.5, rect.width - 1, rect.height - 1, 8);
    Draw::setColor(cr, theme.border);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    std::string shown = Draw::ellipsize(model.status, Draw::Weight::Regular, statusFontSize, rect.width - 24);
    Draw::text(cr, shown, rect.x + 12, rect.y, rect.height, Draw::Weight::Regular, statusFontSize, theme.text_muted);
}

void BrowserUI::drawFind(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    const FindModel& find = model.find;
    const double radius = theme.corner_radius;

    Draw::shadow(cr, m_find, radius + 2, 10, 0.35);
    Draw::roundedRect(cr, m_find, radius + 2);
    Draw::setColor(cr, theme.surface);
    cairo_fill(cr);
    Draw::roundedRect(cr, m_find.x + 0.5, m_find.y + 0.5, m_find.width - 1, m_find.height - 1, radius + 2);
    Draw::setColor(cr, theme.border);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    Draw::roundedRect(cr, m_findBox, radius);
    Draw::setColor(cr, theme.surface_sunken);
    cairo_fill(cr);

    bool focused = find.field && find.field->focused();
    if (focused) {
        Draw::roundedRect(cr, m_findBox.x + 1, m_findBox.y + 1, m_findBox.width - 2, m_findBox.height - 2, radius - 1);
        Draw::setColor(cr, theme.focus_ring);
        cairo_set_line_width(cr, 2.0);
        cairo_stroke(cr);
    }

    Glyph::draw(cr, Glyph::Kind::Search, m_findBox.x + 16, m_findBox.y + 16, 14, theme.text_muted);

    if (find.field) {
        if (find.field->text().empty() && !focused) {
            Draw::text(cr, "Find in page", m_findText.x, m_findText.y, m_findText.height, Draw::Weight::Regular, 13.5, theme.text_muted);
        } else {
            find.field->draw(cr, m_findText, 13.5, theme.text, model.now);
        }

        if (!find.field->text().empty() && find.matches >= 0) {
            std::string count = find.matches == 0 ? std::string("No results") : std::to_string(find.matches) + (find.matches == 1 ? " match" : " matches");
            double w = Draw::textWidth(count, Draw::Weight::Regular, 12.5);
            Draw::text(cr, count, m_findBox.x + m_findBox.width - w - 10, m_findBox.y, m_findBox.height, Draw::Weight::Regular, 12.5, find.matches == 0 ? theme.danger : theme.text_muted);
        }
    }

    Glyph::Kind kinds[3] = { Glyph::Kind::Up, Glyph::Kind::Down, Glyph::Kind::Close };
    Target targets[3] = { Target::FindPrevious, Target::FindNext, Target::FindClose };

    for (int i = 0; i < 3; i++) {
        drawButton(cr, m_findButtons[i], find.hover[i], model.pressed == targets[i], radius);
        Glyph::draw(cr, kinds[i], m_findButtons[i].x + 16, m_findButtons[i].y + 16, i == 2 ? 16 : 15, theme.text, 0.85);
    }
}

std::vector<std::string> BrowserUI::wrap(const std::string& text, double size, double width) const
{
    std::vector<std::string> lines;
    size_t start = 0;

    while (start <= text.size()) {
        size_t newline = text.find('\n', start);
        std::string paragraph = text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
        std::string line;
        size_t pos = 0;

        while (pos < paragraph.size()) {
            size_t space = paragraph.find(' ', pos);
            std::string word = paragraph.substr(pos, space == std::string::npos ? std::string::npos : space - pos);
            std::string candidate = line.empty() ? word : line + " " + word;

            if (!line.empty() && Draw::textWidth(candidate, Draw::Weight::Regular, size) > width) {
                lines.push_back(line);
                line = word;
            } else {
                line = candidate;
            }

            while (Draw::textWidth(line, Draw::Weight::Regular, size) > width && line.size() > 1) {
                size_t cut = line.size() - 1;
                while (cut > 1 && Draw::textWidth(line.substr(0, cut), Draw::Weight::Regular, size) > width) {
                    cut--;
                }
                while (cut > 1 && (line[cut] & 0xC0) == 0x80) {
                    cut--;
                }
                lines.push_back(line.substr(0, cut));
                line = line.substr(cut);
            }

            if (space == std::string::npos) {
                break;
            }
            pos = space + 1;
        }

        lines.push_back(line);

        if (newline == std::string::npos) {
            break;
        }
        start = newline + 1;
    }

    while (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }

    return lines;
}

void BrowserUI::drawDialog(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    const DialogModel& dialog = model.dialog;
    const double radius = theme.corner_radius + 4;

    cairo_rectangle(cr, m_content.x, m_content.y, m_content.width, m_content.height);
    cairo_set_source_rgba(cr, 0, 0, 0, Draw::isLight() ? 0.25 : 0.45);
    cairo_fill(cr);

    Draw::shadow(cr, m_dialog, radius, 18, 0.55);
    Draw::roundedRect(cr, m_dialog, radius);
    Draw::setColor(cr, theme.surface);
    cairo_fill(cr);
    Draw::roundedRect(cr, m_dialog.x + 0.5, m_dialog.y + 0.5, m_dialog.width - 1, m_dialog.height - 1, radius);
    Draw::setColor(cr, theme.border);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    int x = m_dialog.x + dialogPadding;
    int y = m_dialog.y + dialogPadding;
    int w = m_dialog.width - dialogPadding * 2;

    Draw::text(cr, Draw::ellipsize(dialog.title, Draw::Weight::Medium, 15.5, w), x, y, 24, Draw::Weight::Medium, 15.5, theme.text);
    y += 24 + 10;

    for (size_t i = 0; i < m_dialogLines.size() && i < 12; i++) {
        Draw::text(cr, m_dialogLines[i], x, y, 21, Draw::Weight::Regular, 14.0, theme.text_muted);
        y += 21;
    }

    for (size_t i = 0; i < m_dialogFields.size(); i++) {
        const ui_rect_t& field = m_dialogFields[i];
        TextField* tf = dialog.fields[i];
        bool focused = tf && tf->focused();

        Draw::roundedRect(cr, field, theme.corner_radius);
        Draw::setColor(cr, theme.surface_sunken);
        cairo_fill(cr);

        if (focused) {
            Draw::roundedRect(cr, field.x + 1, field.y + 1, field.width - 2, field.height - 2, theme.corner_radius - 1);
            Draw::setColor(cr, theme.focus_ring);
            cairo_set_line_width(cr, 2.0);
        } else {
            Draw::roundedRect(cr, field.x + 0.5, field.y + 0.5, field.width - 1, field.height - 1, theme.corner_radius);
            Draw::setColor(cr, theme.border);
            cairo_set_line_width(cr, 1.0);
        }
        cairo_stroke(cr);

        if (tf && tf->text().empty() && !focused && i < dialog.placeholders.size()) {
            Draw::text(cr, dialog.placeholders[i], m_dialogFieldText[i].x, field.y, field.height, Draw::Weight::Regular, 14.0, theme.text_muted);
        } else if (tf) {
            tf->draw(cr, m_dialogFieldText[i], 14.0, theme.text, model.now);
        }
    }

    for (size_t i = 0; i < m_dialogButtons.size(); i++) {
        const ui_rect_t& button = m_dialogButtons[i];
        bool primary = (int)i == dialog.defaultButton;
        double hover = i < 4 ? dialog.hover[i] : 0.0;

        Draw::roundedRect(cr, button, theme.corner_radius);
        Draw::setColor(cr, primary ? theme.primary : theme.secondary);
        cairo_fill(cr);

        if (hover > 0.01) {
            Draw::roundedRect(cr, button, theme.corner_radius);
            Draw::setColor(cr, theme.hover, hover * 1.5);
            cairo_fill(cr);
        }

        const std::string& label = dialog.buttons[i];
        double tw = Draw::textWidth(label, Draw::Weight::Medium, 14.0);
        Draw::text(cr, label, button.x + (button.width - tw) / 2.0, button.y, button.height, Draw::Weight::Medium, 14.0, primary ? theme.on_primary : theme.on_secondary);
    }
}

void BrowserUI::drawDropdown(cairo_t* cr, const UIModel& model)
{
    const ui_theme_t& theme = Draw::theme();
    const DropdownModel& dd = model.dropdown;
    const double radius = theme.corner_radius + 2;

    Draw::shadow(cr, m_dropdown, radius, 12, 0.45);
    Draw::roundedRect(cr, m_dropdown, radius);
    Draw::setColor(cr, theme.surface);
    cairo_fill(cr);
    Draw::roundedRect(cr, m_dropdown.x + 0.5, m_dropdown.y + 0.5, m_dropdown.width - 1, m_dropdown.height - 1, radius);
    Draw::setColor(cr, theme.border);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    cairo_save(cr);
    cairo_rectangle(cr, m_dropdown.x, m_dropdown.y + dropdownPadding, m_dropdown.width, m_dropdownRows * dropdownRowHeight);
    cairo_clip(cr);

    for (int row = 0; row < m_dropdownRows; row++) {
        int index = row + dd.scroll;
        if (index < 0 || index >= (int)dd.items.size()) {
            break;
        }

        const DropdownItem& item = dd.items[index];
        ui_rect_t r = { m_dropdown.x + dropdownPadding, m_dropdown.y + dropdownPadding + row * dropdownRowHeight, m_dropdown.width - dropdownPadding * 2, dropdownRowHeight };

        if (item.separator) {
            cairo_rectangle(cr, r.x + 8, r.y + r.height / 2.0, r.width - 16, 1);
            Draw::setColor(cr, theme.border, 1.5);
            cairo_fill(cr);
            continue;
        }

        bool selectable = item.enabled && !item.label;

        if (index == dd.highlighted && selectable) {
            Draw::roundedRect(cr, r, theme.corner_radius - 2);
            Draw::setColor(cr, theme.primary);
            cairo_fill(cr);
        }

        ui_color_t color = index == dd.highlighted && selectable ? theme.on_primary : (item.enabled ? theme.text : theme.text_muted);
        double textX = r.x + 30;

        if (index == dd.selected) {
            cairo_save(cr);
            Draw::setColor(cr, color);
            cairo_set_line_width(cr, 1.8);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
            double cx = r.x + 15;
            double cy = r.y + r.height / 2.0;
            cairo_move_to(cr, cx - 4.5, cy + 0.2);
            cairo_line_to(cr, cx - 1.2, cy + 3.4);
            cairo_line_to(cr, cx + 4.8, cy - 3.6);
            cairo_stroke(cr);
            cairo_restore(cr);
        }

        if (item.label) {
            Draw::text(cr, Draw::ellipsize(item.text, Draw::Weight::Medium, 12.5, r.width - 20), r.x + 10, r.y, r.height, Draw::Weight::Medium, 12.5, theme.text_muted);
        } else {
            Draw::text(cr, Draw::ellipsize(item.text, Draw::Weight::Regular, 14.0, r.x + r.width - textX - 8), textX, r.y, r.height, Draw::Weight::Regular, 14.0, color, item.enabled ? 1.0 : 0.6);
        }
    }

    cairo_restore(cr);

    if ((int)dd.items.size() > m_dropdownRows) {
        double track = m_dropdownRows * dropdownRowHeight - 8;
        double thumb = std::max(24.0, track * m_dropdownRows / dd.items.size());
        double pos = (track - thumb) * dd.scroll / std::max(1, (int)dd.items.size() - m_dropdownRows);
        Draw::roundedRect(cr, m_dropdown.x + m_dropdown.width - 6, m_dropdown.y + dropdownPadding + 4 + pos, 3, thumb, 1.5);
        Draw::setColor(cr, theme.text_muted, 0.5);
        cairo_fill(cr);
    }
}

void BrowserUI::drawOverlays(cairo_t* cr, const UIModel& model)
{
    cairo_save(cr);
    cairo_rectangle(cr, m_content.x, m_content.y, m_content.width, m_content.height);
    cairo_clip(cr);

    drawStatus(cr, model);

    if (model.find.visible) {
        drawFind(cr, model);
    }
    if (model.dropdown.visible && m_dropdown.width > 0) {
        drawDropdown(cr, model);
    }
    if (model.dialog.visible) {
        drawDialog(cr, model);
    }

    cairo_restore(cr);
}
