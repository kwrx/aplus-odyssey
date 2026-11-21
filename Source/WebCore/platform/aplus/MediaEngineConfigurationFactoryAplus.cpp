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
#include "MediaEngineConfigurationFactoryAplus.h"

#if ENABLE(VIDEO)

#include "ContentType.h"
#include "MediaCapabilitiesDecodingInfo.h"
#include "MediaDecodingConfiguration.h"
#include "MediaPlayer.h"

namespace WebCore {

static bool isTypeSupported(const String& contentType, bool isMediaSource)
{
    MediaEngineSupportParameters parameters;
    parameters.type = ContentType(contentType);
    parameters.isMediaSource = isMediaSource;
    return MediaPlayer::supportsType(parameters) != MediaPlayer::SupportsType::IsNotSupported;
}

void createMediaPlayerDecodingConfigurationAplus(MediaDecodingConfiguration&& configuration, WTF::Function<void(MediaCapabilitiesDecodingInfo&&)>&& callback)
{
    bool isMediaSource = configuration.type == MediaDecodingType::MediaSource;
    bool supported = true;
    bool smooth = true;

    if (configuration.video) {
        auto& video = *configuration.video;
        supported = isTypeSupported(video.contentType, isMediaSource);
        smooth = static_cast<double>(video.width) * video.height * video.framerate <= 1920.0 * 1080.0 * 30.0;
    }
    if (supported && configuration.audio) {
        supported = isTypeSupported(configuration.audio->contentType, isMediaSource);
    }

    MediaCapabilitiesDecodingInfo info;
    info.supported = supported;
    info.smooth = supported && smooth;
    info.powerEfficient = false;
    info.supportedConfiguration = WTFMove(configuration);
    callback(WTFMove(info));
}

}

#endif
