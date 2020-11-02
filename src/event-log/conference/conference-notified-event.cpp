/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
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

#include <sstream>

#include "conference-notified-event-p.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

ConferenceNotifiedEvent::ConferenceNotifiedEvent (
	Type type,
	time_t creationTime,
	const ConferenceId &conferenceId
) : ConferenceEvent(*new ConferenceNotifiedEventPrivate, type, creationTime, conferenceId) {
}

ConferenceNotifiedEvent::ConferenceNotifiedEvent (
	ConferenceNotifiedEventPrivate &p,
	Type type,
	time_t creationTime,
	const ConferenceId &conferenceId
) : ConferenceEvent(p, type, creationTime, conferenceId) {
}

unsigned int ConferenceNotifiedEvent::getNotifyId () const {
	return notifyId;
}

std::string ConferenceNotifiedEvent::toString () const {
	std::stringstream ss;
	ss << ConferenceEvent::toString();
	ss << " with notify ID " << getNotifyId() << " and fullState " << std::boolalpha << getFullState();
	return ss.str();
}

LINPHONE_END_NAMESPACE
