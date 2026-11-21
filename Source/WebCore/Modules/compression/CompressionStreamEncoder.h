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

#include "BufferSource.h"
#include "ExceptionOr.h"
#include <JavaScriptCore/Uint8Array.h>
#include <wtf/RefCounted.h>
#include <zlib.h>

namespace WebCore {

class ZlibStream {
    WTF_MAKE_NONCOPYABLE(ZlibStream);
public:
    enum class Direction { Compress, Decompress };

    ZlibStream(Direction direction)
        : m_direction(direction)
    {
    }
    ~ZlibStream();

    ExceptionOr<void> initialize(const String& format);
    ExceptionOr<RefPtr<Uint8Array>> process(const uint8_t*, size_t);
    ExceptionOr<RefPtr<Uint8Array>> finish();

private:
    ExceptionOr<RefPtr<Uint8Array>> run(const uint8_t*, size_t, int flush);

    Direction m_direction;
    z_stream m_stream { };
    bool m_initialized { false };
    bool m_ended { false };
};

class CompressionStreamEncoder : public RefCounted<CompressionStreamEncoder> {
public:
    static ExceptionOr<Ref<CompressionStreamEncoder>> create(const String& format);

    ExceptionOr<RefPtr<Uint8Array>> encode(const BufferSource&);
    ExceptionOr<RefPtr<Uint8Array>> flush();

private:
    CompressionStreamEncoder() = default;

    ZlibStream m_stream { ZlibStream::Direction::Compress };
};

class DecompressionStreamDecoder : public RefCounted<DecompressionStreamDecoder> {
public:
    static ExceptionOr<Ref<DecompressionStreamDecoder>> create(const String& format);

    ExceptionOr<RefPtr<Uint8Array>> decode(const BufferSource&);
    ExceptionOr<RefPtr<Uint8Array>> flush();

private:
    DecompressionStreamDecoder() = default;

    ZlibStream m_stream { ZlibStream::Direction::Decompress };
};

}
