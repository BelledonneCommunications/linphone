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

#ifndef _L_LOCAL_CONFERENCE_H_
#define _L_LOCAL_CONFERENCE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "conference.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class LINPHONE_PUBLIC LocalConference :
	public Conference {
	friend class ServerGroupChatRoomPrivate;
public:
	LocalConference (const std::shared_ptr<Core> &core, const IdentityAddress &myAddress, CallSessionListener *listener, const ConferenceParams *params = nullptr);
	virtual ~LocalConference ();

	void subscribeReceived (LinphoneEvent *event);

	std::shared_ptr<ConferenceParticipantEvent> notifyParticipantAdded (time_t creationTime,  const bool isFullState, const Address &addr);
	std::shared_ptr<ConferenceParticipantEvent> notifyParticipantRemoved (time_t creationTime,  const bool isFullState, const Address &addr);
	std::shared_ptr<ConferenceParticipantEvent> notifyParticipantSetAdmin (time_t creationTime,  const bool isFullState, const Address &addr, bool isAdmin);
	std::shared_ptr<ConferenceSubjectEvent> notifySubjectChanged (time_t creationTime, const bool isFullState, const std::string subject);
	std::shared_ptr<ConferenceParticipantDeviceEvent> notifyParticipantDeviceAdded (time_t creationTime,  const bool isFullState, const Address &addr, const Address &gruu, const std::string name = "");
	std::shared_ptr<ConferenceParticipantDeviceEvent> notifyParticipantDeviceRemoved (time_t creationTime,  const bool isFullState, const Address &addr, const Address &gruu);

	void notifyFullState ();

protected:
#ifdef HAVE_ADVANCED_IM
	std::shared_ptr<LocalConferenceEventHandler> eventHandler;
#endif

private:

	L_DISABLE_COPY(LocalConference);

};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_LOCAL_CONFERENCE_H_
