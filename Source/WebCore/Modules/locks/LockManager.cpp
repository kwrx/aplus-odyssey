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
#include "LockManager.h"

#include "JSDOMPromiseDeferred.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include <wtf/HashMap.h>
#include <wtf/IsoMallocInlines.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/UUID.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(LockManager);

namespace {

struct LockRequest {
    LockManager* manager;
    uint64_t id;
    bool exclusive;
    String clientId;
    RefPtr<DeferredPromise> promise;
};

struct LockResource {
    Vector<LockRequest> held;
    Vector<LockRequest> pending;
};

using LockResourceMap = HashMap<String, LockResource>;

HashMap<String, LockResourceMap>& lockRegistry()
{
    static NeverDestroyed<HashMap<String, LockResourceMap>> registry;
    return registry;
}

bool isGrantable(const LockResource& resource, bool exclusive)
{
    if (exclusive)
        return resource.held.isEmpty();
    for (auto& held : resource.held) {
        if (held.exclusive)
            return false;
    }
    return true;
}

void processLockResource(const String& origin, const String& name)
{
    auto originIterator = lockRegistry().find(origin);
    if (originIterator == lockRegistry().end())
        return;

    auto resourceIterator = originIterator->value.find(name);
    if (resourceIterator == originIterator->value.end())
        return;

    Vector<Ref<DeferredPromise>> granted;
    auto& resource = resourceIterator->value;
    while (!resource.pending.isEmpty() && isGrantable(resource, resource.pending.first().exclusive)) {
        auto request = WTFMove(resource.pending.first());
        resource.pending.remove(0);
        if (auto promise = std::exchange(request.promise, nullptr))
            granted.append(promise.releaseNonNull());
        resource.held.append(WTFMove(request));
    }

    if (resource.held.isEmpty() && resource.pending.isEmpty()) {
        originIterator->value.remove(resourceIterator);
        if (originIterator->value.isEmpty())
            lockRegistry().remove(originIterator);
    }

    for (auto& promise : granted)
        promise->resolve<IDLBoolean>(true);
}

}

Ref<LockManager> LockManager::create(ScriptExecutionContext* context)
{
    auto manager = adoptRef(*new LockManager(context));
    manager->suspendIfNeeded();
    return manager;
}

LockManager::LockManager(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_clientId(createCanonicalUUIDString())
{
    if (context && context->securityOrigin())
        m_origin = context->securityOrigin()->toString();
}

LockManager::~LockManager()
{
    auto touched = removeAllRequests();
    if (touched.isEmpty())
        return;

    callOnMainThread([origin = m_origin.isolatedCopy(), touched = WTFMove(touched)] {
        for (auto& name : touched)
            processLockResource(origin, name);
    });
}

void LockManager::requestLock(uint64_t id, const String& name, bool exclusive, bool ifAvailable, bool steal, Ref<DeferredPromise>&& promise)
{
    ASSERT(isMainThread());

    if (m_stopped || m_origin.isNull()) {
        promise->reject(InvalidStateError);
        return;
    }

    auto& resource = lockRegistry().ensure(m_origin, [] {
        return LockResourceMap { };
    }).iterator->value.ensure(name, [] {
        return LockResource { };
    }).iterator->value;

    if (ifAvailable && (!resource.pending.isEmpty() || !isGrantable(resource, exclusive))) {
        processLockResource(m_origin, name);
        promise->resolve<IDLBoolean>(false);
        return;
    }

    LockRequest request { this, id, exclusive, m_clientId, WTFMove(promise) };
    if (steal) {
        resource.held.clear();
        resource.pending.insert(0, WTFMove(request));
    } else
        resource.pending.append(WTFMove(request));

    processLockResource(m_origin, name);
}

void LockManager::releaseLock(uint64_t id)
{
    auto originIterator = lockRegistry().find(m_origin);
    if (originIterator == lockRegistry().end())
        return;

    String found;
    for (auto& entry : originIterator->value) {
        auto matches = [&](const LockRequest& request) {
            return request.manager == this && request.id == id;
        };
        if (entry.value.held.removeFirstMatching(matches) || entry.value.pending.removeFirstMatching(matches)) {
            found = entry.key;
            break;
        }
    }

    if (!found.isNull())
        processLockResource(m_origin, found);
}

Vector<String> LockManager::lockSnapshot() const
{
    Vector<String> snapshot;
    auto originIterator = lockRegistry().find(m_origin);
    if (originIterator == lockRegistry().end())
        return snapshot;

    auto append = [&](const char* kind, const String& name, const LockRequest& request) {
        snapshot.append(kind);
        snapshot.append(name);
        snapshot.append(request.exclusive ? "exclusive"_s : "shared"_s);
        snapshot.append(request.clientId);
    };

    for (auto& entry : originIterator->value) {
        for (auto& request : entry.value.held)
            append("held", entry.key, request);
        for (auto& request : entry.value.pending)
            append("pending", entry.key, request);
    }
    return snapshot;
}

void LockManager::stop()
{
    m_stopped = true;
    releaseAll();
}

void LockManager::releaseAll()
{
    for (auto& name : removeAllRequests())
        processLockResource(m_origin, name);
}

Vector<String> LockManager::removeAllRequests()
{
    Vector<String> touched;
    auto originIterator = lockRegistry().find(m_origin);
    if (originIterator == lockRegistry().end())
        return touched;

    for (auto& entry : originIterator->value) {
        auto matches = [&](const LockRequest& request) {
            return request.manager == this;
        };
        if (entry.value.held.removeAllMatching(matches) + entry.value.pending.removeAllMatching(matches))
            touched.append(entry.key);
    }
    return touched;
}

}
