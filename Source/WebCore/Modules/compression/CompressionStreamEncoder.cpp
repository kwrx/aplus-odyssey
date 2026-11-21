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
#include "CompressionStreamEncoder.h"

#include <JavaScriptCore/GenericTypedArrayViewInlines.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/JSGenericTypedArrayViewInlines.h>
#include <wtf/Vector.h>

namespace WebCore {

static constexpr size_t zlibChunkSize = 64 * 1024;

ZlibStream::~ZlibStream()
{
    if (!m_initialized)
        return;

    if (m_direction == Direction::Compress)
        deflateEnd(&m_stream);
    else
        inflateEnd(&m_stream);
}

ExceptionOr<void> ZlibStream::initialize(const String& format)
{
    int windowBits;
    if (format == "deflate")
        windowBits = 15;
    else if (format == "deflate-raw")
        windowBits = -15;
    else if (format == "gzip")
        windowBits = 15 + 16;
    else
        return Exception { TypeError, makeString("Unsupported compression format: '", format, "'") };

    int result;
    if (m_direction == Direction::Compress)
        result = deflateInit2(&m_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
    else
        result = inflateInit2(&m_stream, windowBits);

    if (result != Z_OK)
        return Exception { OutOfMemoryError };

    m_initialized = true;
    return { };
}

ExceptionOr<RefPtr<Uint8Array>> ZlibStream::run(const uint8_t* data, size_t length, int flush)
{
    Vector<uint8_t> output;

    m_stream.next_in = const_cast<Bytef*>(data);
    m_stream.avail_in = length;

    while (true) {
        size_t offset = output.size();
        output.grow(offset + zlibChunkSize);
        m_stream.next_out = output.data() + offset;
        m_stream.avail_out = zlibChunkSize;

        int result = m_direction == Direction::Compress ? deflate(&m_stream, flush) : inflate(&m_stream, flush);
        output.shrink(offset + zlibChunkSize - m_stream.avail_out);

        if (result == Z_STREAM_END) {
            m_ended = true;
            if (m_stream.avail_in)
                return Exception { TypeError, "Junk found after end of compressed data."_s };
            break;
        }
        if (result == Z_BUF_ERROR) {
            if (m_stream.avail_out)
                break;
            continue;
        }
        if (result != Z_OK)
            return Exception { TypeError, "The compressed data is not valid."_s };
        if (!m_stream.avail_in && m_stream.avail_out && flush == Z_NO_FLUSH)
            break;
    }

    m_stream.next_in = nullptr;
    m_stream.avail_in = 0;

    if (output.isEmpty())
        return RefPtr<Uint8Array> { };
    return Uint8Array::tryCreate(output.data(), output.size());
}

ExceptionOr<RefPtr<Uint8Array>> ZlibStream::process(const uint8_t* data, size_t length)
{
    if (!length)
        return RefPtr<Uint8Array> { };
    if (m_ended)
        return Exception { TypeError, "Junk found after end of compressed data."_s };
    return run(data, length, Z_NO_FLUSH);
}

ExceptionOr<RefPtr<Uint8Array>> ZlibStream::finish()
{
    if (m_direction == Direction::Decompress) {
        if (!m_ended)
            return Exception { TypeError, "The compressed data was truncated."_s };
        return RefPtr<Uint8Array> { };
    }
    if (m_ended)
        return RefPtr<Uint8Array> { };
    return run(nullptr, 0, Z_FINISH);
}

ExceptionOr<Ref<CompressionStreamEncoder>> CompressionStreamEncoder::create(const String& format)
{
    auto encoder = adoptRef(*new CompressionStreamEncoder);
    auto result = encoder->m_stream.initialize(format);
    if (result.hasException())
        return result.releaseException();
    return encoder;
}

ExceptionOr<RefPtr<Uint8Array>> CompressionStreamEncoder::encode(const BufferSource& chunk)
{
    return m_stream.process(chunk.data(), chunk.length());
}

ExceptionOr<RefPtr<Uint8Array>> CompressionStreamEncoder::flush()
{
    return m_stream.finish();
}

ExceptionOr<Ref<DecompressionStreamDecoder>> DecompressionStreamDecoder::create(const String& format)
{
    auto decoder = adoptRef(*new DecompressionStreamDecoder);
    auto result = decoder->m_stream.initialize(format);
    if (result.hasException())
        return result.releaseException();
    return decoder;
}

ExceptionOr<RefPtr<Uint8Array>> DecompressionStreamDecoder::decode(const BufferSource& chunk)
{
    return m_stream.process(chunk.data(), chunk.length());
}

ExceptionOr<RefPtr<Uint8Array>> DecompressionStreamDecoder::flush()
{
    return m_stream.finish();
}

}
