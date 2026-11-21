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

#include "WebCacheStorageProvider.h"

#include <WebCore/CacheQueryOptions.h>
#include <WebCore/ClientOrigin.h>
#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>

using namespace WebCore;
using namespace WebCore::DOMCacheEngine;

namespace {

struct MemoryCache {
    WTF_MAKE_STRUCT_FAST_ALLOCATED;

    ClientOrigin origin;
    String name;
    bool removed { false };
    unsigned references { 0 };
    uint64_t nextRecordIdentifier { 0 };
    Vector<Record> records;
};

class MemoryCacheStorageConnection final : public CacheStorageConnection {
public:
    static MemoryCacheStorageConnection& singleton()
    {
        static NeverDestroyed<Ref<MemoryCacheStorageConnection>> connection = adoptRef(*new MemoryCacheStorageConnection);
        return connection.get().get();
    }

private:
    void open(const ClientOrigin& origin, const String& cacheName, CacheIdentifierCallback&& callback) final
    {
        auto& identifiers = m_origins.ensure(origin, [] { return Vector<uint64_t> { }; }).iterator->value;
        for (auto identifier : identifiers) {
            if (m_caches.get(identifier)->name == cacheName) {
                reply(WTFMove(callback), CacheIdentifierOperationResult { identifier, false });
                return;
            }
        }

        auto identifier = ++m_nextCacheIdentifier;
        auto cache = makeUnique<MemoryCache>();
        cache->origin = origin;
        cache->name = cacheName;
        m_caches.add(identifier, WTFMove(cache));
        identifiers.append(identifier);
        ++m_updateCounter;
        reply(WTFMove(callback), CacheIdentifierOperationResult { identifier, false });
    }

    void remove(uint64_t cacheIdentifier, CacheIdentifierCallback&& callback) final
    {
        auto* cache = m_caches.get(cacheIdentifier);
        if (!cache || cache->removed) {
            reply(WTFMove(callback), CacheIdentifierOperationResult { 0, false });
            return;
        }

        auto iterator = m_origins.find(cache->origin);
        if (iterator != m_origins.end()) {
            iterator->value.removeFirst(cacheIdentifier);
        }
        cache->removed = true;
        ++m_updateCounter;
        if (!cache->references) {
            m_caches.remove(cacheIdentifier);
        }
        reply(WTFMove(callback), CacheIdentifierOperationResult { cacheIdentifier, false });
    }

    void retrieveCaches(const ClientOrigin& origin, uint64_t updateCounter, CacheInfosCallback&& callback) final
    {
        Vector<CacheInfo> infos;
        if (updateCounter != m_updateCounter) {
            auto iterator = m_origins.find(origin);
            if (iterator != m_origins.end()) {
                for (auto identifier : iterator->value) {
                    infos.append(CacheInfo { identifier, m_caches.get(identifier)->name });
                }
            }
        }
        reply(WTFMove(callback), CacheInfos { WTFMove(infos), m_updateCounter });
    }

    void retrieveRecords(uint64_t cacheIdentifier, const RetrieveRecordsOptions& options, RecordsCallback&& callback) final
    {
        auto* cache = m_caches.get(cacheIdentifier);
        if (!cache) {
            reply(WTFMove(callback), makeUnexpected(Error::Internal));
            return;
        }

        Vector<Record> result;
        bool matchAll = options.request.url().isNull();
        if (matchAll || options.ignoreMethod || options.request.httpMethod() == "GET") {
            CacheQueryOptions queryOptions { options.ignoreSearch, options.ignoreMethod, options.ignoreVary, { } };
            for (auto& record : cache->records) {
                if (!matchAll && !queryCacheMatch(options.request, record.request, record.response, queryOptions)) {
                    continue;
                }
                auto copy = record.copy();
                if (!options.shouldProvideResponse) {
                    copy.response = { };
                    copy.responseBody = nullptr;
                    copy.responseBodySize = 0;
                }
                result.append(WTFMove(copy));
            }
        }
        reply(WTFMove(callback), WTFMove(result));
    }

    void batchDeleteOperation(uint64_t cacheIdentifier, const ResourceRequest& request, CacheQueryOptions&& options, RecordIdentifiersCallback&& callback) final
    {
        auto* cache = m_caches.get(cacheIdentifier);
        if (!cache) {
            reply(WTFMove(callback), makeUnexpected(Error::Internal));
            return;
        }

        Vector<uint64_t> deleted;
        if (options.ignoreMethod || request.httpMethod() == "GET") {
            cache->records.removeAllMatching([&](auto& record) {
                if (!queryCacheMatch(request, record.request, record.response, options)) {
                    return false;
                }
                deleted.append(record.identifier);
                return true;
            });
        }
        reply(WTFMove(callback), WTFMove(deleted));
    }

    void batchPutOperation(uint64_t cacheIdentifier, Vector<Record>&& records, RecordIdentifiersCallback&& callback) final
    {
        auto* cache = m_caches.get(cacheIdentifier);
        if (!cache) {
            reply(WTFMove(callback), makeUnexpected(Error::Internal));
            return;
        }

        Vector<uint64_t> identifiers;
        CacheQueryOptions options;
        for (auto& record : records) {
            auto position = cache->records.findMatching([&](auto& existing) {
                return queryCacheMatch(record.request, existing.request, existing.response, options);
            });
            if (position == notFound) {
                record.identifier = ++cache->nextRecordIdentifier;
                record.updateResponseCounter = 0;
                identifiers.append(record.identifier);
                cache->records.append(WTFMove(record));
                continue;
            }
            auto& existing = cache->records[position];
            record.identifier = existing.identifier;
            record.updateResponseCounter = existing.updateResponseCounter + 1;
            identifiers.append(record.identifier);
            existing = WTFMove(record);
        }
        reply(WTFMove(callback), WTFMove(identifiers));
    }

    void reference(uint64_t cacheIdentifier) final
    {
        if (auto* cache = m_caches.get(cacheIdentifier)) {
            ++cache->references;
        }
    }

    void dereference(uint64_t cacheIdentifier) final
    {
        auto* cache = m_caches.get(cacheIdentifier);
        if (!cache || !cache->references) {
            return;
        }
        if (!--cache->references && cache->removed) {
            m_caches.remove(cacheIdentifier);
        }
    }

    template<typename Callback, typename Value>
    static void reply(Callback&& callback, Value&& value)
    {
        callOnMainThread([callback = WTFMove(callback), value = WTFMove(value)]() mutable {
            callback(WTFMove(value));
        });
    }

    HashMap<ClientOrigin, Vector<uint64_t>> m_origins;
    HashMap<uint64_t, std::unique_ptr<MemoryCache>> m_caches;
    uint64_t m_nextCacheIdentifier { 0 };
    uint64_t m_updateCounter { 0 };
};

}

Ref<CacheStorageConnection> WebCacheStorageProvider::createCacheStorageConnection()
{
    return MemoryCacheStorageConnection::singleton();
}
