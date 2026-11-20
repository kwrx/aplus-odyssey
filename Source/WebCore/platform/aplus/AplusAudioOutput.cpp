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
#include "AplusAudioOutput.h"

#include <aplus/audio.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <wtf/MonotonicTime.h>

namespace WebCore {

AplusAudioOutput::AplusAudioOutput(const char* name, uint32_t rate)
    : m_name(name)
    , m_rate(rate)
{
    open();
}

AplusAudioOutput::~AplusAudioOutput()
{
    close();
}

void AplusAudioOutput::open()
{
    m_retryAt = MonotonicTime::now().secondsSinceEpoch().value() + 2.0;

    audio_stream_t* stream = audio_stream_open(m_name, m_rate, 2, AUDIO_FORMAT_S16_LE);
    if (!stream) {
        return;
    }

    if (audio_stream_channels(stream) != 2 || audio_stream_format(stream) != AUDIO_FORMAT_S16_LE) {
        audio_stream_close(stream);
        return;
    }

    m_stream = stream;
    m_outputRate = audio_stream_rate(stream);
    m_step = (uint64_t(m_rate) << 32) / m_outputRate;
    m_phase = 0;
    m_last[0] = m_last[1] = 0;
}

void AplusAudioOutput::close()
{
    if (m_stream) {
        audio_stream_close(m_stream);
        m_stream = nullptr;
    }
}

bool AplusAudioOutput::idle(size_t count, const Function<bool()>& cancelled)
{
    double remaining = double(count) / double(m_rate);

    while (remaining > 0) {
        if (cancelled()) {
            return false;
        }
        double slice = std::min(remaining, 0.02);
        struct timespec ts = { 0, long(slice * 1e9) };
        nanosleep(&ts, nullptr);
        remaining -= slice;
    }

    return true;
}

bool AplusAudioOutput::send(const uint8_t* data, size_t size, const Function<bool()>& cancelled)
{
    while (size) {
        if (cancelled()) {
            return false;
        }

        ssize_t n = audio_stream_write(m_stream, data, size);
        if (n > 0) {
            data += n;
            size -= n;
            continue;
        }

        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            close();
            return false;
        }

        struct pollfd pfd = { audio_stream_fd(m_stream), audio_stream_events(m_stream), 0 };
        poll(&pfd, 1, 20);
    }

    return true;
}

bool AplusAudioOutput::write(const int16_t* frames, size_t count, const Function<bool()>& cancelled)
{
    if (!m_stream && MonotonicTime::now().secondsSinceEpoch().value() >= m_retryAt) {
        open();
    }

    if (!m_stream) {
        return idle(count, cancelled);
    }

    if (m_outputRate == m_rate) {
        return send(reinterpret_cast<const uint8_t*>(frames), count * 4, cancelled);
    }

    m_converted.shrink(0);

    uint64_t end = uint64_t(count) << 32;
    while (m_phase < end) {
        size_t index = size_t(m_phase >> 32);
        int64_t frac = int64_t(m_phase & 0xffffffff);
        for (int c = 0; c < 2; c++) {
            int64_t a = index ? frames[(index - 1) * 2 + c] : m_last[c];
            int64_t b = frames[index * 2 + c];
            m_converted.append(int16_t(a + (((b - a) * frac) >> 32)));
        }
        m_phase += m_step;
    }

    m_phase -= end;
    if (count) {
        m_last[0] = frames[(count - 1) * 2];
        m_last[1] = frames[(count - 1) * 2 + 1];
    }

    return send(reinterpret_cast<const uint8_t*>(m_converted.data()), m_converted.size() * 2, cancelled);
}

double AplusAudioOutput::latency()
{
    if (!m_stream) {
        return 0;
    }

    ssize_t delay = audio_stream_delay(m_stream);
    if (delay <= 0) {
        return 0;
    }

    return double(delay) / (4.0 * double(m_outputRate));
}

void AplusAudioOutput::reset()
{
    if (m_stream) {
        audio_stream_reset(m_stream);
    }

    m_phase = 0;
    m_last[0] = m_last[1] = 0;
}

}
