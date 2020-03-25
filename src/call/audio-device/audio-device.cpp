/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of Liblinphone.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "audio-device.h"
#include "logger/logger.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

AudioDevice::AudioDevice(const MSSndCard *soundCard)
    :soundCard(soundCard)
{
    const char * name = ms_snd_card_get_name(soundCard);
    deviceName = name;

    unsigned int cap = ms_snd_card_get_capabilities(soundCard);
    if (cap & MS_SND_CARD_CAP_CAPTURE && cap & MS_SND_CARD_CAP_PLAYBACK) {
        capabilities = static_cast<Capabilities>(static_cast<int>(Capabilities::Record) | static_cast<int>(Capabilities::Play));
    } else if (cap & MS_SND_CARD_CAP_CAPTURE) {
        capabilities = Capabilities::Record;
    } else if (cap & MS_SND_CARD_CAP_PLAYBACK) {
        capabilities = Capabilities::Play;
    }

    // TODO: how do we get type & driverName ?
}

const MSSndCard *AudioDevice::getSoundCard() const {
    return soundCard;
}

const string& AudioDevice::getDeviceName() const {
    return deviceName;
}

const string& AudioDevice::getDriverName() const {
    return driverName;
}

const AudioDevice::Capabilities& AudioDevice::getCapabilities() const {
    return capabilities;
}

const AudioDevice::Type& AudioDevice::getType() const {
    return deviceType;
}

string AudioDevice::toString() const {
    std::ostringstream ss;
    ss << driverName << ": driver [" << driverName << "], type [";
    switch (deviceType) {
        case AudioDevice::Type::Microphone:
            ss << "Microphone";
            break;
        case AudioDevice::Type::Earpiece:
            ss << "Earpiece";
            break;
        case AudioDevice::Type::Speaker:
            ss << "Speaker";
            break;
        case AudioDevice::Type::Bluetooth:
            ss << "Bluetooth";
            break;
        default:
            ss << "Unknown";
            break;
    }
    ss << "]";
    return ss.str();
}

LINPHONE_END_NAMESPACE