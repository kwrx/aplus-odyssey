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

#include "BrowserUI.h"
#include "TextField.h"

#include <aplus/ui.h>
#include <cstdint>
#include <string>
#include <vector>

#include <wtf/RefPtr.h>

namespace WebCore {
class PopupMenuMorphOS;
}

class Tab;

class BrowserWindow {
public:
    static BrowserWindow* create(const char* url);
    ~BrowserWindow();

    uint32_t id() const;
    int width() const;
    int height() const;
    bool closed() const { return m_closing; }
    bool focused() const { return m_focused; }

    Tab* activeTab() const;
    Tab* openTab(const char* url, bool background);
    void closeTab(Tab*);
    void activateTab(Tab*);
    bool isActiveTab(const Tab* tab) const { return activeTab() == tab; }

    void tabChanged(Tab*);
    void tabDamaged(Tab*);
    void tabCursorChanged(Tab*);
    void tabStatusChanged(Tab*);
    void tabFullScreenChanged(Tab*);
    bool fullScreen() const { return m_model.fullScreen; }

    void flash(const std::string& text, double seconds = 4.0);

    int alert(Tab*, const std::string& title, const std::string& message);
    bool confirm(Tab*, const std::string& title, const std::string& message);
    bool prompt(Tab*, const std::string& title, const std::string& message, const std::string& value, std::string& result);
    bool login(Tab*, const std::string& host, const std::string& realm, std::string& username, std::string& password);
    bool modal() const { return m_model.dialog.visible; }

    void showDropdown(WebCore::PopupMenuMorphOS*);
    void hideDropdown(WebCore::PopupMenuMorphOS*);

    void handleEvent(const ui_event_t&);
    void update();
    int present();
    int timeout() const;

    void close() { m_closing = true; }

private:
    enum class Focus {
        Web,
        Url,
        Find,
        Dialog,
    };

    enum class MenuKind {
        None,
        Main,
        Context,
        Tab,
    };

    struct Anim {
        double value { 0 };
        double target { 0 };
    };

    struct TabAnim {
        Anim hover;
        Anim close;
    };

    BrowserWindow() = default;
    bool init();

    void layout();
    void resizeTabs();
    void syncModel();
    void animate(double dt);
    bool animating() const;
    void setHover(const Hit&);
    void markBars() { m_barsDirty = true; }
    void markOverlays() { m_overlayDirty = true; }

    void handleKey(const ui_event_t&);
    bool handleShortcut(uint16_t vkey, uint8_t mods);
    void handleDialogKey(uint16_t vkey, bool down, const char* text, uint8_t mods);
    void handleDropdownKey(uint16_t vkey, const char* text);
    void handlePointer(const ui_event_t&);
    void handleScroll(const ui_event_t&);
    void handleMenu(const ui_event_t&);
    void pressControl(const Hit&, int button, int x, int clickCount);
    void releaseControl(const Hit&, int button);
    void sendPointer(int kind, int x, int y, int button);
    void sendKey(uint16_t vkey, bool down, const char* text, uint8_t mods);

    void focusWeb();
    void focusUrl();
    void submitUrl();
    void revertUrl();
    void openFind();
    void closeFind();
    void findChanged();
    void findNext(bool forward);

    void showMainMenu();
    void showContextMenu(int x, int y);
    void showTabMenu(int index, int x, int y);
    void runMenuAction(MenuKind, uint32_t id);

    int runDialog(const std::string& title, const std::string& message, const std::vector<std::string>& fields, const std::vector<std::string>& placeholders,
        const std::vector<bool>& secret, const std::vector<std::string>& buttons, int defaultButton, int cancelButton, std::vector<std::string>* values);
    void finishDialog(int button);

    void moveDropdown(int index);
    void chooseDropdown(int index);
    void closeDropdown(bool notify);

    TextField* focusedField();
    void updateCursor();
    void updateTitle();
    void blitContent(const ui_rect_t&);
    std::string currentStatus() const;

    ui_window_t* m_window { nullptr };
    BrowserUI m_ui;
    UIModel m_model;

    std::vector<Tab*> m_tabs;
    std::vector<TabAnim> m_tabAnims;
    std::vector<std::string> m_closedTabs;
    int m_active { 0 };

    Focus m_focus { Focus::Web };
    TextField m_urlField;
    TextField m_findField;
    TextField m_dialogFields[2];
    bool m_urlEdited { false };

    Anim m_newTabHover;
    Anim m_backHover;
    Anim m_forwardHover;
    Anim m_reloadHover;
    Anim m_menuHover;
    Anim m_urlHover;
    Anim m_findHover[3];
    Anim m_dialogHover[4];
    Anim m_progressAlpha;
    double m_progress { 0 };

    Hit m_hover;
    Hit m_pressed;
    bool m_captureContent { false };
    bool m_captureField { false };
    uint8_t m_buttons { 0 };
    int m_pointerX { -1 };
    int m_pointerY { -1 };
    double m_lastClickTime { 0 };
    int m_lastClickButton { 0 };
    int m_clickCount { 0 };
    int m_clickX { 0 };
    int m_clickY { 0 };

    MenuKind m_menuKind { MenuKind::None };
    uint32_t m_menuSerial { 0 };
    int m_menuTab { -1 };
    std::string m_menuLink;
    std::string m_menuImage;
    std::string m_menuSelection;
    bool m_menuEditable { false };

    int m_dialogResult { -1 };
    int m_dialogCancel { 0 };
    int m_dialogDefault { 0 };
    Tab* m_dialogTab { nullptr };

    RefPtr<WebCore::PopupMenuMorphOS> m_popup;

    std::string m_flash;
    double m_flashUntil { 0 };
    std::string m_lastStatus;
    ui_rect_t m_lastStatusRect { 0, 0, 0, 0 };
    ui_rect_t m_overlayRect { 0, 0, 0, 0 };

    bool m_barsDirty { true };
    bool m_overlayDirty { true };
    bool m_contentDirty { true };
    bool m_focused { false };
    bool m_closing { false };
    int m_cursor { -1 };
    std::string m_windowTitle;
    double m_lastFrame { 0 };
    double m_lastSpinner { 0 };
    double m_nextBlink { 0 };
    int m_width { 0 };
    int m_height { 0 };
};
