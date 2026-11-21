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

#include <wtf/Ref.h>
#include <WebCore/PermissionController.h>

class WebPermissionController final : public WebCore::PermissionController {
public:
    static Ref<WebPermissionController> create() { return adoptRef(*new WebPermissionController); }

private:
    WebPermissionController() = default;

    WebCore::PermissionState query(WebCore::ClientOrigin&&, WebCore::PermissionDescriptor&&) final;
    void request(WebCore::ClientOrigin&&, WebCore::PermissionDescriptor&&, CompletionHandler<void(WebCore::PermissionState)>&&) final;
    void addObserver(WebCore::PermissionObserver&) final { }
    void removeObserver(WebCore::PermissionObserver&) final { }
};
