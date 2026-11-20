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

#include "Pages.h"

#include "Draw.h"

#include <cstdio>

namespace Pages {

static std::string css(const ui_color_t& color, double alpha = 1.0)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "rgba(%d,%d,%d,%.3f)", (int)(color.r * 255.0 + 0.5), (int)(color.g * 255.0 + 0.5), (int)(color.b * 255.0 + 0.5), color.a * alpha);
    return buffer;
}

std::string escape(const std::string& text)
{
    std::string out;
    out.reserve(text.size());

    for (char c : text) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '\'':
            out += "&#39;";
            break;
        default:
            out += c;
            break;
        }
    }

    return out;
}

static std::string style()
{
    const ui_theme_t& theme = Draw::theme();
    std::string s;

    s += "<style>";
    s += ":root{";
    s += "--bg:" + css(theme.background) + ";";
    s += "--surface:" + css(theme.surface) + ";";
    s += "--sunken:" + css(theme.surface_sunken) + ";";
    s += "--primary:" + css(theme.primary) + ";";
    s += "--on-primary:" + css(theme.on_primary) + ";";
    s += "--text:" + css(theme.text) + ";";
    s += "--muted:" + css(theme.text_muted) + ";";
    s += "--border:" + css(theme.border) + ";";
    s += "--hover:" + css(theme.hover) + ";";
    s += "--ring:" + css(theme.focus_ring) + ";";
    s += "--soft:" + css(theme.primary, 0.14) + ";";
    s += "}";
    s += "*{box-sizing:border-box}";
    s += "html,body{margin:0;height:100%}";
    s += "body{background:var(--bg);color:var(--text);font-family:'Noto Sans',sans-serif;font-size:14px;";
    s += "display:flex;align-items:center;justify-content:center;-webkit-user-select:none}";
    s += "a{color:inherit;text-decoration:none}";
    s += "</style>";
    return s;
}

std::string startPage()
{
    struct Link {
        const char* name;
        const char* url;
        const char* letter;
    };

    static const Link links[] = {
        { "DuckDuckGo", "https://duckduckgo.com/html/", "D" },
        { "Wikipedia", "https://en.wikipedia.org/", "W" },
        { "Hacker News", "https://news.ycombinator.com/", "Y" },
        { "GitHub", "https://github.com/", "G" },
        { "aplus", "https://github.com/kwrx/aplus", "a" },
        { "Lobsters", "https://lobste.rs/", "L" },
    };

    std::string html;
    html += "<!DOCTYPE html><html><head><meta charset='utf-8'><title>New Tab</title>";
    html += style();
    html += "<style>";
    html += ".wrap{width:560px;max-width:calc(100% - 48px);margin-top:-6vh;text-align:center}";
    html += ".logo{display:inline-flex;align-items:center;gap:14px;margin-bottom:30px}";
    html += ".logo h1{margin:0;font-size:34px;font-weight:600;letter-spacing:-0.5px}";
    html += ".search{display:flex;align-items:center;height:48px;padding:0 6px 0 18px;border-radius:24px;";
    html += "background:var(--surface);border:1px solid var(--border);box-shadow:0 6px 24px rgba(0,0,0,.18)}";
    html += ".search:focus-within{border-color:var(--ring);box-shadow:0 0 0 1px var(--ring),0 6px 24px rgba(0,0,0,.18)}";
    html += ".search svg{flex:none;opacity:.6}";
    html += ".search input{flex:1;height:100%;border:0;outline:0;background:transparent;color:var(--text);";
    html += "font:inherit;font-size:15px;padding:0 12px}";
    html += ".search input::placeholder{color:var(--muted)}";
    html += ".search button{height:36px;padding:0 18px;border:0;border-radius:18px;background:var(--primary);";
    html += "color:var(--on-primary);font:inherit;font-weight:500;cursor:pointer}";
    html += ".tiles{display:grid;grid-template-columns:repeat(6,1fr);gap:8px;margin-top:34px}";
    html += ".tile{display:flex;flex-direction:column;align-items:center;gap:10px;padding:14px 4px 12px;border-radius:12px}";
    html += ".tile:hover{background:var(--hover)}";
    html += ".badge{width:46px;height:46px;border-radius:14px;display:flex;align-items:center;justify-content:center;";
    html += "background:var(--soft);color:var(--primary);font-size:19px;font-weight:600}";
    html += ".tile span{font-size:12.5px;color:var(--muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;max-width:100%}";
    html += ".hint{margin-top:30px;color:var(--muted);font-size:12px;opacity:.8}";
    html += "kbd{font:inherit;padding:1px 6px;border-radius:5px;border:1px solid var(--border);background:var(--surface)}";
    html += "</style></head><body><div class='wrap'>";
    html += "<div class='logo'><svg width='44' height='44' viewBox='0 0 44 44'>";
    html += "<circle cx='22' cy='22' r='19' fill='none' stroke='var(--primary)' stroke-width='3'/>";
    html += "<path d='M22 9 L27 22 L22 35 L17 22 Z' fill='var(--primary)'/>";
    html += "<circle cx='22' cy='22' r='3' fill='var(--bg)'/></svg><h1>Odyssey</h1></div>";
    html += "<form class='search' action='https://duckduckgo.com/html/' method='get'>";
    html += "<svg width='18' height='18' viewBox='0 0 18 18'><circle cx='7.5' cy='7.5' r='5.5' fill='none' stroke='currentColor' stroke-width='1.8'/>";
    html += "<path d='M11.6 11.6 L16 16' stroke='currentColor' stroke-width='1.8' stroke-linecap='round'/></svg>";
    html += "<input name='q' placeholder='Search the web' autocomplete='off'><button type='submit'>Search</button></form>";
    html += "<div class='tiles'>";

    for (const auto& link : links) {
        html += "<a class='tile' href='";
        html += link.url;
        html += "'><div class='badge'>";
        html += link.letter;
        html += "</div><span>";
        html += link.name;
        html += "</span></a>";
    }

    html += "</div><div class='hint'><kbd>Ctrl</kbd> <kbd>L</kbd> address bar &nbsp;·&nbsp; <kbd>Ctrl</kbd> <kbd>T</kbd> new tab";
    html += " &nbsp;·&nbsp; <kbd>Ctrl</kbd> <kbd>F</kbd> find</div>";
    html += "</div></body></html>";
    return html;
}

std::string errorPage(const std::string& url, const std::string& description)
{
    std::string host = url;
    size_t sep = host.find("://");
    if (sep != std::string::npos) {
        host = host.substr(sep + 3);
    }
    size_t end = host.find_first_of("/?#");
    if (end != std::string::npos) {
        host = host.substr(0, end);
    }

    std::string html;
    html += "<!DOCTYPE html><html><head><meta charset='utf-8'><title>";
    html += escape(host.empty() ? url : host);
    html += "</title>";
    html += style();
    html += "<style>";
    html += ".card{width:520px;max-width:calc(100% - 48px);margin-top:-8vh}";
    html += ".icon{width:56px;height:56px;border-radius:16px;background:var(--soft);display:flex;align-items:center;justify-content:center;margin-bottom:24px}";
    html += "h1{margin:0 0 10px;font-size:24px;font-weight:600}";
    html += "p{margin:0 0 8px;color:var(--muted);line-height:1.55}";
    html += "code{font-family:'JetBrains Mono',monospace;font-size:12.5px;padding:2px 6px;border-radius:6px;background:var(--sunken);color:var(--text);word-break:break-all}";
    html += ".actions{margin-top:28px;display:flex;gap:10px}";
    html += "button{height:38px;padding:0 20px;border-radius:10px;border:0;font:inherit;font-weight:500;cursor:pointer}";
    html += ".primary{background:var(--primary);color:var(--on-primary)}";
    html += ".secondary{background:var(--surface);color:var(--text);border:1px solid var(--border)}";
    html += "</style></head><body><div class='card'>";
    html += "<div class='icon'><svg width='28' height='28' viewBox='0 0 28 28' fill='none' stroke='var(--primary)' stroke-width='2.2' stroke-linecap='round'>";
    html += "<path d='M4 11 C10 5 18 5 24 11'/><path d='M8 15.5 C11.5 12 16.5 12 20 15.5'/><circle cx='14' cy='20.5' r='1.6' fill='var(--primary)'/>";
    html += "<path d='M5 4 L23 24'/></svg></div>";
    html += "<h1>This page can&#39;t be reached</h1>";
    html += "<p><code>";
    html += escape(host.empty() ? url : host);
    html += "</code></p><p>";
    html += escape(description.empty() ? std::string("The connection failed.") : description);
    html += "</p><div class='actions'><button class='primary' data-url='";
    html += escape(url);
    html += "' onclick='location.href=this.dataset.url'>Try again</button><button class='secondary' onclick='history.back()'>Go back</button></div>";
    html += "</div></body></html>";
    return html;
}

}
