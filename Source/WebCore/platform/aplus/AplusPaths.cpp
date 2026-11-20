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

#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

const char* aplusConfigDir()
{
    static char path[PATH_MAX];

    if (!path[0]) {
        const char* home = getenv("HOME");
        snprintf(path, sizeof(path), "%s/.config", home && home[0] ? home : "/tmp");
        mkdir(path, 0755);
        snprintf(path, sizeof(path), "%s/.config/odyssey", home && home[0] ? home : "/tmp");
        mkdir(path, 0755);
    }

    return path;
}

extern char** environ;

void aplusOpenExternal(const char* url)
{
    if (!url || !url[0]) {
        return;
    }

    pid_t pid;
    char* const argv[] = { (char*)"aplus-xopen", (char*)url, nullptr };
    posix_spawnp(&pid, "aplus-xopen", nullptr, nullptr, argv, environ);
}
