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

#include "Draw.h"
#include "TextField.h"

#include <string>
#include <vector>

enum class Target {
    None,
    Tab,
    TabClose,
    NewTab,
    Back,
    Forward,
    Reload,
    Url,
    Menu,
    FindField,
    FindPrevious,
    FindNext,
    FindClose,
    DialogField,
    DialogButton,
    DropdownItem,
    Content,
};

struct Hit {
    Target target { Target::None };
    int index { 0 };

    bool operator==(const Hit& other) const { return target == other.target && index == other.index; }
    bool operator!=(const Hit& other) const { return !(*this == other); }
};

struct TabModel {
    std::string title;
    bool loading { false };
    double hover { 0 };
    double closeHover { 0 };
};

struct FindModel {
    bool visible { false };
    TextField* field { nullptr };
    int matches { -1 };
    double hover[3] { 0, 0, 0 };
};

struct DialogModel {
    bool visible { false };
    std::string title;
    std::string message;
    std::vector<TextField*> fields;
    std::vector<std::string> placeholders;
    std::vector<std::string> buttons;
    int focusedField { -1 };
    int defaultButton { 0 };
    double hover[4] { 0, 0, 0, 0 };
};

struct DropdownItem {
    std::string text;
    bool enabled { true };
    bool label { false };
    bool separator { false };
};

struct DropdownModel {
    bool visible { false };
    ui_rect_t anchor { 0, 0, 0, 0 };
    std::vector<DropdownItem> items;
    int selected { -1 };
    int highlighted { -1 };
    int scroll { 0 };
};

struct UIModel {
    int width { 0 };
    int height { 0 };
    double now { 0 };

    std::vector<TabModel> tabs;
    int activeTab { 0 };
    double newTabHover { 0 };

    bool canGoBack { false };
    bool canGoForward { false };
    bool loading { false };
    double progress { 0 };
    double progressAlpha { 0 };
    double hoverBack { 0 };
    double hoverForward { 0 };
    double hoverReload { 0 };
    double hoverMenu { 0 };
    double hoverUrl { 0 };
    Target pressed { Target::None };

    std::string url;
    TextField* urlField { nullptr };

    std::string status;
    int pointerX { -1 };
    int pointerY { -1 };

    FindModel find;
    DialogModel dialog;
    DropdownModel dropdown;

    bool fullScreen { false };
};

class BrowserUI {
public:
    static constexpr int tabStripHeight = 40;
    static constexpr int toolbarHeight = 48;
    static constexpr int contentTop = tabStripHeight + toolbarHeight;
    static constexpr int dropdownRowHeight = 30;

    void layout(const UIModel&);

    Hit hitTest(const UIModel&, int x, int y) const;

    int top() const { return m_top; }
    ui_rect_t barsRect() const { return { 0, 0, m_width, m_top }; }
    ui_rect_t contentRect() const { return m_content; }
    ui_rect_t urlTextRect() const { return m_urlText; }
    ui_rect_t findTextRect() const { return m_findText; }
    ui_rect_t dialogFieldRect(size_t index) const { return index < m_dialogFields.size() ? m_dialogFieldText[index] : ui_rect_t { 0, 0, 0, 0 }; }
    ui_rect_t tabRect(size_t index) const { return index < m_tabs.size() ? m_tabs[index] : ui_rect_t { 0, 0, 0, 0 }; }
    ui_rect_t menuRect() const { return m_menu; }
    int dropdownVisibleRows() const { return m_dropdownRows; }

    void drawBars(cairo_t*, const UIModel&);
    void drawOverlays(cairo_t*, const UIModel&);

    ui_rect_t statusRect(const UIModel&) const;
    ui_rect_t findRect() const { return m_find; }
    ui_rect_t dialogRect() const { return m_dialog; }
    ui_rect_t dropdownRect() const { return m_dropdown; }

private:
    void drawTabs(cairo_t*, const UIModel&);
    void drawToolbar(cairo_t*, const UIModel&);
    void drawUrl(cairo_t*, const UIModel&);
    void drawStatus(cairo_t*, const UIModel&);
    void drawFind(cairo_t*, const UIModel&);
    void drawDialog(cairo_t*, const UIModel&);
    void drawDropdown(cairo_t*, const UIModel&);
    void drawButton(cairo_t*, const ui_rect_t&, double hover, bool pressed, double radius);
    std::vector<std::string> wrap(const std::string&, double size, double width) const;

    int m_width { 0 };
    int m_height { 0 };
    int m_top { contentTop };
    std::vector<ui_rect_t> m_tabs;
    std::vector<ui_rect_t> m_tabCloses;
    ui_rect_t m_newTab { 0, 0, 0, 0 };
    ui_rect_t m_back { 0, 0, 0, 0 };
    ui_rect_t m_forward { 0, 0, 0, 0 };
    ui_rect_t m_reload { 0, 0, 0, 0 };
    ui_rect_t m_url { 0, 0, 0, 0 };
    ui_rect_t m_urlText { 0, 0, 0, 0 };
    ui_rect_t m_menu { 0, 0, 0, 0 };
    ui_rect_t m_content { 0, 0, 0, 0 };
    ui_rect_t m_find { 0, 0, 0, 0 };
    ui_rect_t m_findBox { 0, 0, 0, 0 };
    ui_rect_t m_findText { 0, 0, 0, 0 };
    ui_rect_t m_findButtons[3] {};
    ui_rect_t m_dialog { 0, 0, 0, 0 };
    std::vector<std::string> m_dialogLines;
    std::vector<ui_rect_t> m_dialogFields;
    std::vector<ui_rect_t> m_dialogFieldText;
    std::vector<ui_rect_t> m_dialogButtons;
    ui_rect_t m_dropdown { 0, 0, 0, 0 };
    int m_dropdownRows { 0 };
};
