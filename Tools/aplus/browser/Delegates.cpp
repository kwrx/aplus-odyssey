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

#include "Delegates.h"

#include "App.h"
#include "BrowserWindow.h"
#include "JSActionDelegate.h"
#include "ObserverData.h"
#include "ObserverServiceData.h"
#include "PopupMenuMorphOS.h"
#include "SharedPtr.h"
#include "Tab.h"
#include "WebDownload.h"
#include "WebDownloadDelegate.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebFrameLoadDelegate.h"
#include "WebNotificationDelegate.h"
#include "WebURLResponse.h"
#include "WebView.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static constexpr int errorCancelled = -999;

static bool isMain(WebFrame* frame)
{
    return frame && frame->webView() && frame->webView()->mainFrame() == frame;
}

static std::string frameURL(WebFrame* frame)
{
    const char* url = frame ? frame->url() : nullptr;
    std::string out = url ? url : "";
    free((void*)url);
    return out;
}

static std::string hostOf(const std::string& url)
{
    size_t sep = url.find("://");
    if (sep == std::string::npos) {
        return std::string();
    }

    std::string host = url.substr(sep + 3);
    size_t end = host.find_first_of("/?#");
    if (end != std::string::npos) {
        host = host.substr(0, end);
    }
    size_t at = host.rfind('@');
    if (at != std::string::npos) {
        host = host.substr(at + 1);
    }
    return host;
}

static std::string dialogTitle(WebFrame* frame)
{
    std::string host = hostOf(frameURL(frame));
    return host.empty() ? std::string("This page says") : host + " says";
}

namespace Delegates {

std::string formatSize(long long bytes)
{
    char buffer[32];

    if (bytes < 1024) {
        snprintf(buffer, sizeof(buffer), "%lld B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.0f KB", bytes / 1024.0);
    } else if (bytes < 1024LL * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }

    return buffer;
}

}

using Delegates::formatSize;

class FrameLoadDelegate final : public WebFrameLoadDelegate {
public:
    static TransferSharedPtr<FrameLoadDelegate> createInstance() { return new FrameLoadDelegate(); }

    void windowObjectClearNotification(WebFrame*, void*, void*) override { }

    void titleChange(WebFrame* frame, const char* title) override
    {
        if (Tab* tab = Tab::fromFrame(frame); tab && isMain(frame)) {
            tab->setTitle(title);
        }
    }

    void didStartProvisionalLoad(WebFrame* frame) override
    {
        if (Tab* tab = Tab::fromFrame(frame); tab && isMain(frame)) {
            tab->didStartLoading();
        }
    }

    void didReceiveServerRedirectForProvisionalLoadForFrame(WebFrame*) override { }
    void willPerformClientRedirectToURL(WebFrame*, const char*, double, double) override { }
    void didCancelClientRedirectForFrame(WebFrame*) override { }

    void didCommitLoad(WebFrame* frame) override
    {
        if (Tab* tab = Tab::fromFrame(frame); tab && isMain(frame)) {
            tab->didCommit(frameURL(frame));
        }
    }

    void didFinishLoad(WebFrame* frame) override
    {
        if (Tab* tab = Tab::fromFrame(frame); tab && isMain(frame)) {
            tab->didFinishLoading();
        }
    }

    void didFailProvisionalLoad(WebFrame* frame, WebError* error) override { failed(frame, error); }
    void didFailLoad(WebFrame* frame, WebError* error) override { failed(frame, error); }
    void didFinishDocumentLoadForFrame(WebFrame*) override { }
    void didHandleOnloadEventsForFrame(WebFrame*) override { }
    void didChangeLocationWithinPageForFrame(WebFrame* frame) override { didCommitLoad(frame); }
    void willCloseFrame(WebFrame*) override { }
    void didFirstLayoutInFrame(WebFrame*) override { }
    void didFirstVisuallyNonEmptyLayoutInFrame(WebFrame*) override { }
    void dispatchNotEnoughMemory(WebFrame*) override { }
    void didDisplayInsecureContent(WebFrame*) override { }
    void didRunInsecureContent(WebFrame*, WebSecurityOrigin*) override { }
    void didPushStateWithinPage(WebFrame* frame) override { didCommitLoad(frame); }
    void didReplaceStateWithinPage(WebFrame* frame) override { didCommitLoad(frame); }
    void didPopStateWithinPage(WebFrame* frame) override { didCommitLoad(frame); }

private:
    static void failed(WebFrame* frame, WebError* error)
    {
        Tab* tab = Tab::fromFrame(frame);
        if (!tab || !isMain(frame)) {
            return;
        }

        bool cancelled = !error || error->isPolicyChangeError() || error->code() == errorCancelled || error->code() == WebKitErrorPlugInWillHandleLoad;
        const char* url = error ? error->failingURL() : nullptr;
        const char* description = error ? error->localizedDescription() : nullptr;

        tab->didFail(url ? url : "", description ? description : "", cancelled);
    }
};

class JSDelegate final : public JSActionDelegate {
public:
    static TransferSharedPtr<JSDelegate> createInstance() { return new JSDelegate(); }

    void consoleMessage(WebFrame*, int line, int column, const char* message) override
    {
        static const bool enabled = getenv("ODYSSEY_CONSOLE");
        if (enabled) {
            fprintf(stderr, "odyssey: console:%d:%d: %s\n", line, column, message ? message : "");
        }
    }

    bool jsAlert(WebFrame* frame, const char* message) override
    {
        Tab* tab = Tab::fromFrame(frame);
        if (tab) {
            tab->window()->alert(tab, dialogTitle(frame), message ? message : "");
        }
        return true;
    }

    bool jsConfirm(WebFrame* frame, const char* message) override
    {
        Tab* tab = Tab::fromFrame(frame);
        return tab && tab->window()->confirm(tab, dialogTitle(frame), message ? message : "");
    }

    bool jsPrompt(WebFrame* frame, const char* message, const char* defaultValue, char** value) override
    {
        Tab* tab = Tab::fromFrame(frame);
        std::string result;

        if (!tab || !tab->window()->prompt(tab, dialogTitle(frame), message ? message : "", defaultValue ? defaultValue : "", result)) {
            return false;
        }

        *value = strdup(result.c_str());
        return true;
    }

    void exceededDatabaseQuota(WebFrame*, WebSecurityOrigin*, const char*) override { }
};

class NotificationDelegate final : public WebNotificationDelegate {
public:
    static TransferSharedPtr<NotificationDelegate> createInstance() { return new NotificationDelegate(); }

    void startLoadNotification(WebFrame* frame) override { progress(frame); }
    void progressNotification(WebFrame* frame) override { progress(frame); }
    void finishedLoadNotification(WebFrame* frame) override { progress(frame); }

private:
    static void progress(WebFrame* frame)
    {
        if (Tab* tab = Tab::fromFrame(frame)) {
            tab->updateProgress();
        }
    }
};

class DownloadDelegate final : public WebDownloadDelegate {
public:
    static TransferSharedPtr<DownloadDelegate> createInstance() { return new DownloadDelegate(); }

    void decideDestinationWithSuggestedFilename(WebDownload* download, const char* filename) override
    {
        std::string path = Delegates::downloadPath(filename && filename[0] ? filename : "download");

        Progress& p = m_downloads[download];
        p.name = path.substr(path.rfind('/') + 1);
        p.received = 0;

        download->setDestination(path.c_str(), false, false);
        report(p);
    }

    void didCancelAuthenticationChallenge(WebDownload*, WebURLAuthenticationChallenge*) override { }
    void didCreateDestination(WebDownload*, const char*) override { }

    void didFailWithError(WebDownload* download, WebError*) override
    {
        Progress& p = m_downloads[download];
        flash("Download failed: " + (p.name.empty() ? std::string("file") : p.name), 6.0);
        m_downloads.erase(download);
    }

    void didReceiveAuthenticationChallenge(WebDownload*, WebURLAuthenticationChallenge*) override { }

    void didReceiveDataOfLength(WebDownload* download, unsigned length) override
    {
        Progress& p = m_downloads[download];
        p.received += length;

        double now = App::now();
        if (now - p.reported >= 0.25) {
            p.reported = now;
            report(p);
        }
    }

    void didReceiveResponse(WebDownload* download, WebURLResponse* response) override
    {
        if (response) {
            m_downloads[download].expected = response->expectedContentLength();
        }
    }

    bool shouldDecodeSourceDataOfMIMEType(WebDownload*, const char*) override { return true; }
    void willResumeWithResponse(WebDownload*, WebURLResponse*, long long) override { }
    WebMutableURLRequest* willSendRequest(WebDownload*, WebMutableURLRequest* request, WebURLResponse*) override { return request; }
    void didBegin(WebDownload*) override { }

    void didFinish(WebDownload* download) override
    {
        Progress& p = m_downloads[download];
        flash("Downloaded " + (p.name.empty() ? std::string("file") : p.name) + " · " + formatSize(p.received), 6.0);
        m_downloads.erase(download);
    }

private:
    struct Progress {
        std::string name;
        long long received { 0 };
        long long expected { -1 };
        double reported { 0 };
    };

    static void flash(const std::string& text, double seconds)
    {
        if (BrowserWindow* window = App::activeWindow()) {
            window->flash(text, seconds);
        }
    }

    static void report(const Progress& p)
    {
        std::string text = "Downloading " + p.name + " · " + formatSize(p.received);

        if (p.expected > 0) {
            int percent = (int)std::min<long long>(100, p.received * 100 / p.expected);
            text += " of " + formatSize(p.expected) + " (" + std::to_string(percent) + "%)";
        }

        flash(text, 3.0);
    }

    std::map<WebDownload*, Progress> m_downloads;
};

class PopupObserver final : public WebCore::ObserverData {
public:
    void observe(const WTF::String& topic, const WTF::String&, void* subject) override
    {
        auto* popup = static_cast<WebCore::PopupMenuMorphOS*>(subject);
        BrowserWindow* window = App::activeWindow();

        if (!popup || !window) {
            if (popup && topic == "PopupMenuShow" && popup->client()) {
                popup->client()->popupDidHide();
            }
            return;
        }

        if (topic == "PopupMenuShow") {
            window->showDropdown(popup);
        } else if (topic == "PopupMenuHide") {
            window->hideDropdown(popup);
        }
    }
};

namespace Delegates {

static SharedPtr<DownloadDelegate>& downloads()
{
    static SharedPtr<DownloadDelegate> delegate = DownloadDelegate::createInstance();
    return delegate;
}

void attach(WebView* webView)
{
    webView->setWebFrameLoadDelegate(FrameLoadDelegate::createInstance());
    webView->setJSActionDelegate(JSDelegate::createInstance());
    webView->setWebNotificationDelegate(NotificationDelegate::createInstance());
    webView->setDownloadDelegate(TransferSharedPtr<WebDownloadDelegate>(downloads()));
}

std::string downloadPath(const std::string& filename)
{
    std::string dir = App::downloadsDirectory();
    std::string name = filename.empty() ? std::string("download") : filename;
    std::replace(name.begin(), name.end(), '/', '_');

    std::string path = dir + "/" + name;
    size_t dot = name.rfind('.');
    std::string stem = dot == std::string::npos || dot == 0 ? name : name.substr(0, dot);
    std::string ext = dot == std::string::npos || dot == 0 ? std::string() : name.substr(dot);

    for (int i = 1; access(path.c_str(), F_OK) == 0; i++) {
        path = dir + "/" + stem + " (" + std::to_string(i) + ")" + ext;
    }

    return path;
}

void download(const std::string& url)
{
    WebDownload* download = WebDownload::createInstance(URL({ }, String::fromUTF8(url.c_str())), TransferSharedPtr<WebDownloadDelegate>(downloads()));
    download->start();
}

void registerObservers()
{
    static PopupObserver observer;
    auto* service = WebCore::ObserverServiceData::createObserverService();
    service->registerObserver("PopupMenuShow", &observer);
    service->registerObserver("PopupMenuHide", &observer);
}

}
