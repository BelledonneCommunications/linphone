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

#include "c-wrapper/c-wrapper.h"
#include "call-p.h"
#include "chat/chat-room/abstract-chat-room-p.h"
#include "conference/params/media-session-params-p.h"
#include "conference/session/call-session-p.h"
#include "conference/session/media-session-p.h"
#include "core/core-p.h"
#include "logger/logger.h"

#include "conference_private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

shared_ptr<AbstractChatRoom> CallPrivate::getChatRoom () {
	L_Q();
	if ((q->getState() != CallSession::State::End) && (q->getState() != CallSession::State::Released)) {
		chatRoom = q->getCore()->getOrCreateBasicChatRoom(q->getRemoteAddress());
		if (chatRoom) {
			const char *callId = linphone_call_log_get_call_id(q->getLog());
			lInfo() << "Setting call id [" << callId << "] to ChatRoom [" << chatRoom << "]";
			chatRoom->getPrivate()->setCallId(callId);
		}
	}
	return chatRoom;
}

LinphoneProxyConfig *CallPrivate::getDestProxy () const {
	return getActiveSession()->getPrivate()->getDestProxy();
}

/* This a test-only method.*/
IceSession *CallPrivate::getIceSession () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getIceSession();
	return nullptr;
}

unsigned int CallPrivate::getAudioStartCount () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getAudioStartCount();
}

unsigned int CallPrivate::getVideoStartCount () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getVideoStartCount();
}

unsigned int CallPrivate::getTextStartCount () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getTextStartCount();
}

MediaStream *CallPrivate::getMediaStream (LinphoneStreamType type) const {
	auto ms = static_pointer_cast<MediaSession>(getActiveSession())->getPrivate();
	StreamsGroup & sg = ms->getStreamsGroup();
	MS2Stream *s = nullptr;
	switch(type){
		case LinphoneStreamTypeAudio:
			s = sg.lookupMainStreamInterface<MS2Stream>(SalAudio);
		break;
		case LinphoneStreamTypeVideo:
			s = sg.lookupMainStreamInterface<MS2Stream>(SalVideo);
		break;
		case LinphoneStreamTypeText:
			s = sg.lookupMainStreamInterface<MS2Stream>(SalText);
		break;
		default:
		break;
	}
	if (!s){
		lError() << "CallPrivate::getMediaStream() : no stream with type " << type;
		return nullptr;
	}
	return s->getMediaStream();
}

SalCallOp * CallPrivate::getOp () const {
	return getActiveSession()->getPrivate()->getOp();
}

bool CallPrivate::getSpeakerMuted () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getSpeakerMuted();
}

void CallPrivate::setSpeakerMuted (bool muted) {
	static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->setSpeakerMuted(muted);
}

bool CallPrivate::getMicrophoneMuted () const {
	return static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getMicrophoneMuted();
}

void CallPrivate::setMicrophoneMuted (bool muted) {
	static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->setMicrophoneMuted(muted);
}

LinphoneCallStats *CallPrivate::getStats (LinphoneStreamType type) const {
	return static_pointer_cast<const MediaSession>(getActiveSession())->getPrivate()->getStats(type);
}

// -----------------------------------------------------------------------------

void CallPrivate::initiateIncoming () {
	getActiveSession()->initiateIncoming();
}

bool CallPrivate::initiateOutgoing () {
	shared_ptr<CallSession> session = getActiveSession();
	bool defer = session->initiateOutgoing();
	session->getPrivate()->createOp();
	return defer;
}

void CallPrivate::iterate (time_t currentRealTime, bool oneSecondElapsed) {
	getActiveSession()->iterate(currentRealTime, oneSecondElapsed);
}

void CallPrivate::startIncomingNotification () {
	getActiveSession()->startIncomingNotification();
}

void CallPrivate::pauseForTransfer () {
	static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->pauseForTransfer();
}

int CallPrivate::startInvite (const Address *destination) {
	return getActiveSession()->startInvite(destination, "");
}

shared_ptr<Call> CallPrivate::startReferredCall (const MediaSessionParams *params) {
	L_Q();
	if (q->getState() != CallSession::State::Paused) {
		pauseForTransfer();
	}
	MediaSessionParams msp;
	if (params)
		msp = *params;
	else {
		msp.initDefault(q->getCore(), LinphoneCallOutgoing);
		msp.enableAudio(q->getCurrentParams()->audioEnabled());
		msp.enableVideo(q->getCurrentParams()->videoEnabled());
	}
	lInfo() << "Starting new call to referred address " << q->getReferTo();
	L_GET_PRIVATE(&msp)->setReferer(getActiveSession());
	L_GET_PRIVATE(getActiveSession())->setReferPending(false);
	LinphoneCallParams *lcp = L_GET_C_BACK_PTR(&msp);
	LinphoneCall *newCall = linphone_core_invite_with_params(q->getCore()->getCCore(), q->getReferTo().c_str(), lcp);
	if (newCall) {
		getActiveSession()->getPrivate()->setTransferTarget(L_GET_PRIVATE_FROM_C_OBJECT(newCall)->getActiveSession());
		L_GET_PRIVATE_FROM_C_OBJECT(newCall)->getActiveSession()->getPrivate()->notifyReferState();
	}
	return L_GET_CPP_PTR_FROM_C_OBJECT(newCall);
}

// -----------------------------------------------------------------------------

void CallPrivate::createPlayer () const {
	L_Q();
	player = linphone_call_build_player(L_GET_C_BACK_PTR(q));
}

// -----------------------------------------------------------------------------

void CallPrivate::initializeMediaStreams () {
	
}

void CallPrivate::stopMediaStreams () {
	static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->stopStreams();
}

// -----------------------------------------------------------------------------


void CallPrivate::startRemoteRing () {
	L_Q();
	LinphoneCore *lc = q->getCore()->getCCore();
	if (!lc->sound_conf.play_sndcard)
		return;

	MSSndCard *ringCard = lc->sound_conf.lsd_card ? lc->sound_conf.lsd_card : lc->sound_conf.play_sndcard;
	SalMediaDescription *md = static_pointer_cast<MediaSession>(getActiveSession())->getPrivate()->getLocalDesc();
	if (md){
		int maxRate = md->streams[0].max_rate;
		if (maxRate > 0)
			ms_snd_card_set_preferred_sample_rate(ringCard, maxRate);
	}
	if (lc->sound_conf.remote_ring) {
		ms_snd_card_set_stream_type(ringCard, MS_SND_CARD_STREAM_VOICE);
		lc->ringstream = ring_start(lc->factory, lc->sound_conf.remote_ring, 2000, ringCard);
	}
}

void CallPrivate::terminateBecauseOfLostMedia () {
	L_Q();
	lInfo() << "Call [" << q << "]: Media connectivity with " << q->getRemoteAddress().asString()
		<< " is lost, call is going to be terminated";
	static_pointer_cast<MediaSession>(getActiveSession())->terminateBecauseOfLostMedia();
	q->getCore()->getPrivate()->getToneManager()->startNamedTone(getActiveSession(), LinphoneToneCallLost);
}

// -----------------------------------------------------------------------------

void CallPrivate::onAckBeingSent (const shared_ptr<CallSession> &session, LinphoneHeaders *headers) {
	L_Q();
	linphone_call_notify_ack_processing(L_GET_C_BACK_PTR(q), headers, false);
}

void CallPrivate::onAckReceived (const shared_ptr<CallSession> &session, LinphoneHeaders *headers) {
	L_Q();
	linphone_call_notify_ack_processing(L_GET_C_BACK_PTR(q), headers, true);
}

void CallPrivate::onBackgroundTaskToBeStarted (const shared_ptr<CallSession> &session) {
	L_Q();
	bgTask.start(q->getCore(),30);
}

void CallPrivate::onBackgroundTaskToBeStopped (const shared_ptr<CallSession> &session) {
	bgTask.stop();
}

bool CallPrivate::onCallSessionAccepted (const shared_ptr<CallSession> &session) {
	L_Q();
	LinphoneCore *lc = q->getCore()->getCCore();
	bool wasRinging = false;

	if (q->getCore()->getCurrentCall() != q->getSharedFromThis())
		linphone_core_preempt_sound_resources(lc);

	return wasRinging;
}

void CallPrivate::onCallSessionConferenceStreamStarting (const shared_ptr<CallSession> &session, bool mute) {
	L_Q();
	LinphoneCall *call = L_GET_C_BACK_PTR(q);
	LinphoneConference *conf = linphone_call_get_conference(call);
	if (conf) {
		linphone_conference_on_call_stream_starting(conf, call, mute);
	}
}

void CallPrivate::onCallSessionConferenceStreamStopping (const shared_ptr<CallSession> &session) {
	L_Q();
	LinphoneCall *call = L_GET_C_BACK_PTR(q);
	LinphoneConference *conf = linphone_call_get_conference(call);
	if (conf && _linphone_call_get_endpoint(call))
		linphone_conference_on_call_stream_stopping(conf, call);
}

void CallPrivate::onCallSessionEarlyFailed (const shared_ptr<CallSession> &session, LinphoneErrorInfo *ei) {
	L_Q();
	LinphoneCallLog *log = session->getLog();
	linphone_core_report_early_failed_call(q->getCore()->getCCore(),
		linphone_call_log_get_dir(log),
		linphone_address_clone(linphone_call_log_get_from(log)),
		linphone_address_clone(linphone_call_log_get_to(log)),
		ei,
		log->call_id);
	linphone_call_unref(L_GET_C_BACK_PTR(q));
}

void CallPrivate::onCallSessionSetReleased (const shared_ptr<CallSession> &session) {
	L_Q();
	linphone_call_unref(L_GET_C_BACK_PTR(q));
}

void CallPrivate::onCallSessionSetTerminated (const shared_ptr<CallSession> &session) {
	L_Q();
	LinphoneCore *core = q->getCore()->getCCore();
	LinphoneCall *call = L_GET_C_BACK_PTR(q);
	LinphoneConference *conf = linphone_call_get_conference(call);
	
	if (q->getSharedFromThis() == q->getCore()->getCurrentCall()) {
		lInfo() << "Resetting the current call";
		q->getCore()->getPrivate()->setCurrentCall(nullptr);
	}
	if (q->getCore()->getPrivate()->removeCall(q->getSharedFromThis()) != 0)
		lError() << "Could not remove the call from the list!!!";
	if (conf)
		linphone_conference_on_call_terminating(conf, call);
#if 0
	if (lcall->chat_room)
		linphone_chat_room_set_call(lcall->chat_room, nullptr);
#endif // if 0
	if (!q->getCore()->getPrivate()->hasCalls())
		ms_bandwidth_controller_reset_state(core->bw_controller);
}

void CallPrivate::onCallSessionStartReferred (const shared_ptr<CallSession> &session) {
	startReferredCall(nullptr);
}

void CallPrivate::onCallSessionStateChanged (const shared_ptr<CallSession> &session, CallSession::State state, const string &message) {
	L_Q();
	q->getCore()->getPrivate()->getToneManager()->update(session);

	switch(state){
		case CallSession::State::OutgoingInit:
		case CallSession::State::IncomingReceived:
			getPlatformHelpers(q->getCore()->getCCore())->acquireWifiLock();
			getPlatformHelpers(q->getCore()->getCCore())->acquireMcastLock();
			getPlatformHelpers(q->getCore()->getCCore())->acquireCpuLock();
			break;
		case CallSession::State::Released:
			getPlatformHelpers(q->getCore()->getCCore())->releaseWifiLock();
			getPlatformHelpers(q->getCore()->getCCore())->releaseMcastLock();
			getPlatformHelpers(q->getCore()->getCCore())->releaseCpuLock();
			break;
		default:
			break;
	}
	linphone_call_notify_state_changed(L_GET_C_BACK_PTR(q), static_cast<LinphoneCallState>(state), message.c_str());
}

void CallPrivate::onCallSessionTransferStateChanged (const shared_ptr<CallSession> &session, CallSession::State state) {
	L_Q();
	linphone_call_notify_transfer_state_changed(L_GET_C_BACK_PTR(q), static_cast<LinphoneCallState>(state));
}

void CallPrivate::onCheckForAcceptation (const shared_ptr<CallSession> &session) {
	L_Q();
	list<shared_ptr<Call>> calls = q->getCore()->getCalls();
	shared_ptr<Call> currentCall = q->getSharedFromThis();
	for (const auto &call : calls) {
		if (call == currentCall)
			continue;
		switch (call->getState()) {
			case CallSession::State::OutgoingInit:
			case CallSession::State::OutgoingProgress:
			case CallSession::State::OutgoingRinging:
			case CallSession::State::OutgoingEarlyMedia:
				lInfo() << "Already existing call [" << call << "] in state [" << Utils::toString(call->getState())
					<< "], canceling it before accepting new call [" << currentCall << "]";
				call->terminate();
				break;
			default:
				break; // Nothing to do
		}
	}
}

void CallPrivate::onDtmfReceived (const shared_ptr<CallSession> &session, char dtmf) {
	L_Q();
	linphone_call_notify_dtmf_received(L_GET_C_BACK_PTR(q), dtmf);
}

void CallPrivate::onIncomingCallSessionNotified (const shared_ptr<CallSession> &session) {
	L_Q();
	/* The call is acceptable so we can now add it to our list */
	q->getCore()->getPrivate()->addCall(q->getSharedFromThis());
}

void CallPrivate::onIncomingCallSessionStarted (const shared_ptr<CallSession> &session) {
	L_Q();
	if (linphone_core_get_calls_nb(q->getCore()->getCCore()) == 1 && !q->isInConference()) {
		L_GET_PRIVATE_FROM_C_OBJECT(q->getCore()->getCCore())->setCurrentCall(q->getSharedFromThis());
	}
	q->getCore()->getPrivate()->getToneManager()->startRingtone(session);
}

void CallPrivate::onIncomingCallSessionTimeoutCheck (const shared_ptr<CallSession> &session, int elapsed, bool oneSecondElapsed) {
	L_Q();
	if (oneSecondElapsed)
		lInfo() << "Incoming call ringing for " << elapsed << " seconds";
	if (elapsed > q->getCore()->getCCore()->sip_conf.inc_timeout) {
		lInfo() << "Incoming call timeout (" << q->getCore()->getCCore()->sip_conf.inc_timeout << ")";
		auto config = linphone_core_get_config(q->getCore()->getCCore());
		int statusCode = linphone_config_get_int(config, "sip", "inc_timeout_status_code", 486);
		getActiveSession()->declineNotAnswered(linphone_error_code_to_reason(statusCode));
	}
}

void CallPrivate::onInfoReceived (const shared_ptr<CallSession> &session, const LinphoneInfoMessage *im) {
	L_Q();
	linphone_call_notify_info_message_received(L_GET_C_BACK_PTR(q), im);
}

void CallPrivate::onLossOfMediaDetected (const shared_ptr<CallSession> &session) {
	terminateBecauseOfLostMedia();
}

void CallPrivate::onEncryptionChanged (const shared_ptr<CallSession> &session, bool activated, const string &authToken) {
	L_Q();
	linphone_call_notify_encryption_changed(L_GET_C_BACK_PTR(q), activated, authToken.empty() ? nullptr : authToken.c_str());
}

void CallPrivate::onCallSessionStateChangedForReporting (const shared_ptr<CallSession> &session) {
	L_Q();
	linphone_reporting_call_state_updated(L_GET_C_BACK_PTR(q));
}

void CallPrivate::onRtcpUpdateForReporting (const shared_ptr<CallSession> &session, SalStreamType type) {
	L_Q();
	linphone_reporting_on_rtcp_update(L_GET_C_BACK_PTR(q), type);
}

void CallPrivate::onStatsUpdated (const shared_ptr<CallSession> &session, const LinphoneCallStats *stats) {
	L_Q();
	linphone_call_notify_stats_updated(L_GET_C_BACK_PTR(q), stats);
}

void CallPrivate::onUpdateMediaInfoForReporting (const shared_ptr<CallSession> &session, int statsType) {
	L_Q();
	linphone_reporting_update_media_info(L_GET_C_BACK_PTR(q), statsType);
}

void CallPrivate::onResetCurrentSession (const shared_ptr<CallSession> &session) {
	L_Q();
	q->getCore()->getPrivate()->setCurrentCall(nullptr);
}

void CallPrivate::onSetCurrentSession (const shared_ptr<CallSession> &session) {
	L_Q();
	q->getCore()->getPrivate()->setCurrentCall(q->getSharedFromThis());
}

void CallPrivate::onFirstVideoFrameDecoded (const shared_ptr<CallSession> &session) {
	L_Q();
	if (nextVideoFrameDecoded._func) {
		nextVideoFrameDecoded._func(L_GET_C_BACK_PTR(q), nextVideoFrameDecoded._user_data);
		nextVideoFrameDecoded._func = nullptr;
		nextVideoFrameDecoded._user_data = nullptr;
	}
	linphone_call_notify_next_video_frame_decoded(L_GET_C_BACK_PTR(q));
}

void CallPrivate::onResetFirstVideoFrameDecoded (const shared_ptr<CallSession> &session) {
	/*we are called here by the MediaSession when the stream start to know whether there is the deprecated nextVideoFrameDecoded callback set,
	 * so that we can request the notification of the next frame decoded.*/
#ifdef VIDEO_ENABLED
	if (nextVideoFrameDecoded._func)
		requestNotifyNextVideoFrameDecoded();
#endif // ifdef VIDEO_ENABLED
}

void CallPrivate::requestNotifyNextVideoFrameDecoded(){
	static_pointer_cast<MediaSession>(getActiveSession())->requestNotifyNextVideoFrameDecoded();
}

void CallPrivate::onCameraNotWorking (const std::shared_ptr<CallSession> &session, const char *camera_name) {
	L_Q();
	linphone_call_notify_camera_not_working(L_GET_C_BACK_PTR(q), camera_name);
}

void CallPrivate::onRingbackToneRequested (const shared_ptr<CallSession> &session, bool requested) {
	L_Q();
	if (requested && linphone_core_get_remote_ringback_tone(q->getCore()->getCCore()))
		playingRingbackTone = true;
	else if (!requested)
		playingRingbackTone = false;
}

bool CallPrivate::areSoundResourcesAvailable (const shared_ptr<CallSession> &session) {
	L_Q();
	LinphoneCore *lc = q->getCore()->getCCore();
	shared_ptr<Call> currentCall = q->getCore()->getCurrentCall();
	return !linphone_core_is_in_conference(lc) && (!currentCall || (currentCall == q->getSharedFromThis()));
}

bool CallPrivate::isPlayingRingbackTone (const shared_ptr<CallSession> &session) {
	return playingRingbackTone;
}

void CallPrivate::onRealTimeTextCharacterReceived (const shared_ptr<CallSession> &session, RealtimeTextReceivedCharacter *data) {
	L_Q();
	shared_ptr<AbstractChatRoom> chatRoom = getChatRoom();
	if (chatRoom) {
		chatRoom->getPrivate()->realtimeTextReceived(data->character, q->getSharedFromThis());
	} else {
		lError()<<"CallPrivate::onRealTimeTextCharacterReceived: no chatroom.";
	}
}

void CallPrivate::onTmmbrReceived (const shared_ptr<CallSession> &session, int streamIndex, int tmmbr) {
	L_Q();
	linphone_call_notify_tmmbr_received(L_GET_C_BACK_PTR(q), streamIndex, tmmbr);
}

void CallPrivate::onSnapshotTaken(const shared_ptr<CallSession> &session, const char *file_path) {
	L_Q();
	linphone_call_notify_snapshot_taken(L_GET_C_BACK_PTR(q), file_path);
}

// =============================================================================

Call::Call (CallPrivate &p, shared_ptr<Core> core) : Object(p), CoreAccessor(core) {
	L_D();
	d->nextVideoFrameDecoded._func = nullptr;
	d->nextVideoFrameDecoded._user_data = nullptr;

	d->bgTask.setName("Liblinphone call notification");
}

// -----------------------------------------------------------------------------

LinphoneStatus Call::accept (const MediaSessionParams *msp) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->accept(msp);
}

LinphoneStatus Call::acceptEarlyMedia (const MediaSessionParams *msp) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->acceptEarlyMedia(msp);
}

LinphoneStatus Call::acceptUpdate (const MediaSessionParams *msp) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->acceptUpdate(msp);
}

void Call::cancelDtmfs () {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->cancelDtmfs();
}

LinphoneStatus Call::decline (LinphoneReason reason) {
	L_D();
	return d->getActiveSession()->decline(reason);
}

LinphoneStatus Call::decline (const LinphoneErrorInfo *ei) {
	L_D();
	return d->getActiveSession()->decline(ei);
}

LinphoneStatus Call::deferUpdate () {
	L_D();
	return d->getActiveSession()->deferUpdate();
}

bool Call::hasTransferPending () const {
	L_D();
	return d->getActiveSession()->hasTransferPending();
}

void Call::oglRender () const {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->getPrivate()->oglRender();
}

LinphoneStatus Call::pause () {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->pause();
}

LinphoneStatus Call::redirect (const string &redirectUri) {
	L_D();
	return d->getActiveSession()->redirect(redirectUri);
}

LinphoneStatus Call::resume () {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->resume();
}

LinphoneStatus Call::sendDtmf (char dtmf) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->sendDtmf(dtmf);
}

LinphoneStatus Call::sendDtmfs (const string &dtmfs) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->sendDtmfs(dtmfs);
}

void Call::sendVfuRequest () {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->sendVfuRequest();
}

void Call::startRecording () {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->startRecording();
}

void Call::stopRecording () {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->stopRecording();
}

bool Call::isRecording () {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->isRecording();
}

LinphoneStatus Call::takePreviewSnapshot (const string &file) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->takePreviewSnapshot(file);
}

LinphoneStatus Call::takeVideoSnapshot (const string &file) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->takeVideoSnapshot(file);
}

LinphoneStatus Call::terminate (const LinphoneErrorInfo *ei) {
	L_D();
	return d->getActiveSession()->terminate(ei);
}

LinphoneStatus Call::transfer (const shared_ptr<Call> &dest) {
	L_D();
	return d->getActiveSession()->transfer(dest->getPrivate()->getActiveSession());
}

LinphoneStatus Call::transfer (const string &dest) {
	L_D();
	return d->getActiveSession()->transfer(dest);
}

LinphoneStatus Call::update (const MediaSessionParams *msp) {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->update(msp);
}

void Call::zoomVideo (float zoomFactor, float *cx, float *cy) {
	zoomVideo(zoomFactor, *cx, *cy);
}

void Call::zoomVideo (float zoomFactor, float cx, float cy) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->zoomVideo(zoomFactor, cx, cy);
}

// -----------------------------------------------------------------------------

bool Call::cameraEnabled () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->cameraEnabled();
}

bool Call::echoCancellationEnabled () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->echoCancellationEnabled();
}

bool Call::echoLimiterEnabled () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->echoLimiterEnabled();
}

void Call::enableCamera (bool value) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->enableCamera(value);
}

void Call::enableEchoCancellation (bool value) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->enableEchoCancellation(value);
}

void Call::enableEchoLimiter (bool value) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->enableEchoLimiter(value);
}

bool Call::getAllMuted () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getAllMuted();
}

LinphoneCallStats *Call::getAudioStats () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getAudioStats();
}

string Call::getAuthenticationToken () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getAuthenticationToken();
}

bool Call::getAuthenticationTokenVerified () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getAuthenticationTokenVerified();
}

float Call::getAverageQuality () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getAverageQuality();
}

const MediaSessionParams *Call::getCurrentParams () const {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->getCurrentParams();
}

float Call::getCurrentQuality () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getCurrentQuality();
}

LinphoneCallDir Call::getDirection () const {
	L_D();
	return d->getActiveSession()->getDirection();
}

const Address &Call::getDiversionAddress () const {
	L_D();
	return d->getActiveSession()->getDiversionAddress();
}

int Call::getDuration () const {
	L_D();
	return d->getActiveSession()->getDuration();
}

const LinphoneErrorInfo *Call::getErrorInfo () const {
	L_D();
	return d->getActiveSession()->getErrorInfo();
}

const Address &Call::getLocalAddress () const {
	L_D();
	return d->getActiveSession()->getLocalAddress();
}

LinphoneCallLog *Call::getLog () const {
	L_D();
	return d->getActiveSession()->getLog();
}

RtpTransport *Call::getMetaRtcpTransport (int streamIndex) const {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->getMetaRtcpTransport(streamIndex);
}

RtpTransport *Call::getMetaRtpTransport (int streamIndex) const {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->getMetaRtpTransport(streamIndex);
}

float Call::getMicrophoneVolumeGain () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getMicrophoneVolumeGain();
}

void *Call::getNativeVideoWindowId () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getNativeVideoWindowId();
}

const MediaSessionParams *Call::getParams () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getMediaParams();
}

LinphonePlayer *Call::getPlayer () const {
	L_D();
	if (!d->player)
		d->createPlayer();
	return d->player;
}

float Call::getPlayVolume () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getPlayVolume();
}

LinphoneReason Call::getReason () const {
	L_D();
	return d->getActiveSession()->getReason();
}

float Call::getRecordVolume () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getRecordVolume();
}

shared_ptr<Call> Call::getReferer () const {
	L_D();
	shared_ptr<CallSession> referer = d->getActiveSession()->getReferer();
	if (!referer)
		return nullptr;
	for (const auto &call : getCore()->getCalls()) {
		if (call->getPrivate()->getActiveSession() == referer)
			return call;
	}
	return nullptr;
}

string Call::getReferTo () const {
	L_D();
	return d->getActiveSession()->getReferTo();
}

const Address &Call::getRemoteAddress () const {
	L_D();
	return d->getActiveSession()->getRemoteAddress();
}

string Call::getRemoteContact () const {
	L_D();
	return d->getActiveSession()->getRemoteContact();
}

const MediaSessionParams *Call::getRemoteParams () const {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->getRemoteParams();
}

string Call::getRemoteUserAgent () const {
	L_D();
	return d->getActiveSession()->getRemoteUserAgent();
}

shared_ptr<Call> Call::getReplacedCall () const {
	L_D();
	shared_ptr<CallSession> replacedCallSession = d->getActiveSession()->getReplacedCallSession();
	if (!replacedCallSession)
		return nullptr;
	for (const auto &call : getCore()->getCalls()) {
		if (call->getPrivate()->getActiveSession() == replacedCallSession)
			return call;
	}
	return nullptr;
}

float Call::getSpeakerVolumeGain () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getSpeakerVolumeGain();
}

CallSession::State Call::getState () const {
	L_D();
	return d->getActiveSession()->getState();
}

LinphoneCallStats *Call::getStats (LinphoneStreamType type) const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getStats(type);
}

int Call::getStreamCount () const {
	L_D();
	return static_pointer_cast<MediaSession>(d->getActiveSession())->getStreamCount();
}

MSFormatType Call::getStreamType (int streamIndex) const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getStreamType(streamIndex);
}

LinphoneCallStats *Call::getTextStats () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getTextStats();
}

const Address &Call::getToAddress () const {
	L_D();
	return d->getActiveSession()->getToAddress();
}

string Call::getToHeader (const string &name) const {
	L_D();
	return d->getActiveSession()->getToHeader(name);
}

CallSession::State Call::getTransferState () const {
	L_D();
	return d->getActiveSession()->getTransferState();
}

shared_ptr<Call> Call::getTransferTarget () const {
	L_D();
	shared_ptr<CallSession> transferTarget = d->getActiveSession()->getTransferTarget();
	if (!transferTarget)
		return nullptr;
	for (const auto &call : getCore()->getCalls()) {
		if (call->getPrivate()->getActiveSession() == transferTarget)
			return call;
	}
	return nullptr;
}

LinphoneCallStats *Call::getVideoStats () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->getVideoStats();
}

bool Call::isInConference () const {
	L_D();
	return d->getActiveSession()->getPrivate()->isInConference();
}

bool Call::mediaInProgress () const {
	L_D();
	return static_pointer_cast<const MediaSession>(d->getActiveSession())->mediaInProgress();
}

void Call::setAudioRoute (LinphoneAudioRoute route) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->setAudioRoute(route);
}

void Call::setAuthenticationTokenVerified (bool value) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->setAuthenticationTokenVerified(value);
}

void Call::setMicrophoneVolumeGain (float value) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->setMicrophoneVolumeGain(value);
}

void Call::setNativeVideoWindowId (void *id) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->setNativeVideoWindowId(id);
}

void Call::setNextVideoFrameDecodedCallback (LinphoneCallCbFunc cb, void *user_data) {
	L_D();
	d->nextVideoFrameDecoded._func = cb;
	d->nextVideoFrameDecoded._user_data = user_data;
	d->requestNotifyNextVideoFrameDecoded();
}

void Call::requestNotifyNextVideoFrameDecoded (){
	L_D();
	d->requestNotifyNextVideoFrameDecoded();
}

void Call::setParams (const MediaSessionParams *msp) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->setParams(msp);
}

void Call::setSpeakerVolumeGain (float value) {
	L_D();
	static_pointer_cast<MediaSession>(d->getActiveSession())->setSpeakerVolumeGain(value);
}

LINPHONE_END_NAMESPACE
