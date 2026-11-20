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

#include <stddef.h>
#include <stdint.h>
#include <wtf/Function.h>
#include <wtf/Vector.h>

struct audio_stream;

namespace WebCore {

class AplusAudioOutput {
    WTF_MAKE_FAST_ALLOCATED;
public:
    AplusAudioOutput(const char* name, uint32_t rate);
    ~AplusAudioOutput();

    bool isOpen() const { return m_stream; }
    uint32_t rate() const { return m_rate; }

    bool write(const int16_t* frames, size_t count, const Function<bool()>& cancelled);
    double latency();
    void reset();

private:
    void open();
    void close();
    bool send(const uint8_t* data, size_t size, const Function<bool()>& cancelled);
    bool idle(size_t count, const Function<bool()>& cancelled);

    const char* m_name;
    uint32_t m_rate;
    uint32_t m_outputRate { 0 };
    struct audio_stream* m_stream { nullptr };
    double m_retryAt { 0 };
    uint64_t m_step { 0 };
    uint64_t m_phase { 0 };
    int16_t m_last[2] { 0, 0 };
    Vector<int16_t> m_converted;
};

}
