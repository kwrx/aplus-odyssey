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

#include "ActiveDOMObject.h"
#include "ScriptWrappable.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DeferredPromise;

class LockManager final : public ScriptWrappable, public RefCounted<LockManager>, public ActiveDOMObject {
    WTF_MAKE_ISO_ALLOCATED(LockManager);
public:
    static Ref<LockManager> create(ScriptExecutionContext*);
    ~LockManager();

    uint64_t newLockRequestId() { return ++m_lastRequestId; }
    void requestLock(uint64_t id, const String& name, bool exclusive, bool ifAvailable, bool steal, Ref<DeferredPromise>&&);
    void releaseLock(uint64_t id);
    Vector<String> lockSnapshot() const;

private:
    explicit LockManager(ScriptExecutionContext*);

    const char* activeDOMObjectName() const final { return "LockManager"; }
    void stop() final;
    void releaseAll();
    Vector<String> removeAllRequests();

    String m_origin;
    String m_clientId;
    uint64_t m_lastRequestId { 0 };
    bool m_stopped { false };
};

}
