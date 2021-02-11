/*
 * Copyright (c) 2010-2021 Belledonne Communications SARL.
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

#ifndef _SAL_STREAM_DESCRIPTION_H_
#define _SAL_STREAM_DESCRIPTION_H_

#include <map>
#include <vector>

#include <belle-sip/potential_config_graph.hh>

#include "linphone/utils/general.h"
#include "c-wrapper/internal/c-sal.h"
#include "ortp/rtpsession.h"
#include "sal/sal_stream_configuration.h"

LINPHONE_BEGIN_NAMESPACE

class SalMediaDescription;
class SalStreamConfiguration;
class MediaSessionPrivate;
class MS2Stream;
class IceService;
class SalCallOp;
class OfferAnswerEngine;

struct SalConfigurationCmp {
	bool operator()(const bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type& lhs, const bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type& rhs) const;
};

class LINPHONE_PUBLIC SalStreamDescription {
	friend class MediaSessionPrivate;
	friend class MS2Stream;
	friend class IceService;
	friend class SalCallOp;
	friend class SalMediaDescription;
	friend class OfferAnswerEngine;

	public:

		struct raw_capability_negotiation_attrs_t {
			bellesip::SDP::SDPPotentialCfgGraph::media_description_acap acaps;
			bellesip::SDP::SDPPotentialCfgGraph::media_description_base_cap tcaps;
			bellesip::SDP::SDPPotentialCfgGraph::media_description_config cfgs;
		};

		using acap_t = std::pair<std::string, std::string>;
		using acap_map_t = std::map<unsigned int, acap_t>;
		using tcap_map_t = std::map<unsigned int, std::string>;

		static unsigned int getFreeIdx(const std::list<unsigned int> & l);

		// Map of the available configurations
		// TODO: handle multiple cfgs with same index
		using cfg_map = std::map<bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type, SalStreamConfiguration, SalConfigurationCmp>;

		static bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type actualConfigurationIndex;

		SalStreamDescription();
		SalStreamDescription(const SalMediaDescription * salMediaDesc, const belle_sdp_media_description_t *media_desc);
		SalStreamDescription(const SalMediaDescription * salMediaDesc, const belle_sdp_media_description_t *media_desc, const SalStreamDescription::raw_capability_negotiation_attrs_t & attrs);
		SalStreamDescription(const SalStreamDescription & other);
		virtual ~SalStreamDescription();
		SalStreamDescription &operator=(const SalStreamDescription& other);
		int compareToChosenConfiguration(const SalStreamDescription & other) const;
		int compareToActualConfiguration(const SalStreamDescription & other) const;
		int equal(const SalStreamDescription & other) const;
		bool operator==(const SalStreamDescription & other) const;
		bool operator!=(const SalStreamDescription & other) const;
		belle_sdp_media_description_t * toSdpMediaDescription(const SalMediaDescription * salMediaDesc, belle_sdp_session_description_t *session_desc) const;
		bool enabled() const;
		void disable();

		// This function returns the index of the chosen configuration for this stream. It should be called only if capability negotiation functionality described in rfc5939 is supported.
		// It returns 0 if the current chosen configuration is the actual configuration or if rfc5939 is not supported.
		// On the offerer side, its value is valid only if it received the answer to it offer
		// On the answerer side, it is valid only after it has chosen the valid configuration.
		// Returned values:
		// - 0: actual configuration
		// - 1 to 2^31-1: configuration number as received in the SDP acfg or pcfg attribute
		const bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type & getChosenConfigurationIndex() const;
		const bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type & getActualConfigurationIndex() const;

		const SalStreamConfiguration & getConfigurationAtIndex(const bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type & index) const;
		const SalStreamConfiguration & getActualConfiguration() const;
		const SalStreamConfiguration & getChosenConfiguration() const;

		void addActualConfiguration(const SalStreamConfiguration & cfg);
		void addConfigurationAtIndex(const bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type & idx, const SalStreamConfiguration & cfg);

		/*these are switch case, so that when a new proto is added we can't forget to modify this function*/
		bool hasAvpf() const;
		bool hasIpv6() const;
		bool hasImplicitAvpf() const;

		/*these are switch case, so that when a new proto is added we can't forget to modify this function*/
		bool hasSrtp() const;
		bool hasDtls() const;
		bool hasZrtp() const;
		const std::string & getRtcpAddress() const;
		const int & getRtcpPort() const;
		const std::string & getRtpAddress() const;
		const int & getRtpPort() const;
		const std::list<PayloadType*> & getPayloads() const;
		bool isBundleOnly() const;

		const SalStreamType & getType() const;
		const std::string getTypeAsString() const;
		void setProto(const SalMediaProto & newProto);
		const SalMediaProto & getProto() const;
		const std::string getProtoAsString() const;
		SalStreamDir getDirection() const;
		const int & getMaxRate() const;
		SalCustomSdpAttribute * getCustomSdpAttributes() const;

		const cfg_map getAllCfgs() const;

		void setZrtpHash(const uint8_t enable, uint8_t* zrtphash = NULL);

		const std::vector<SalSrtpCryptoAlgo> & getCryptos() const;
		const SalSrtpCryptoAlgo & getCryptoAtIndex(const size_t & idx) const;

		void addTcap(const unsigned int & idx, const std::string & value);
		const std::string & getTcap(const unsigned int & idx) const;

		void addAcap(const unsigned int & idx, const std::string & name, const std::string & value);
		const acap_t & getAcap(const unsigned int & idx) const;

		const acap_map_t & getAcaps() const;
		const tcap_map_t & getTcaps() const;

		unsigned int getFreeCfgIdx() const;

		std::string name; /*unique name of stream, in order to ease offer/answer model algorithm*/
		SalStreamType type = SalAudio;
		std::string typeother;
		std::string rtp_addr;
		std::string rtcp_addr;
		int rtp_port = 0;
		int rtcp_port = 0;
		std::list<PayloadType*> already_assigned_payloads; /**<list of PayloadType offered in the past, used for correct allocation of payload type numbers*/
		int bandwidth = 0;
		SalMulticastRole multicast_role = SalMulticastInactive;

	private:

		mutable bellesip::SDP::SDPPotentialCfgGraph::media_description_config::key_type cfgIndex = 0;

		cfg_map cfgs;
		acap_map_t acaps;
		tcap_map_t tcaps;

		void fillStreamDescriptionFromSdp(const SalMediaDescription * salMediaDesc, const belle_sdp_media_description_t *media_desc);
		void fillStreamDescriptionFromSdp(const SalMediaDescription * salMediaDesc, const belle_sdp_media_description_t *media_desc, const raw_capability_negotiation_attrs_t & attrs);

		// Potential configurations
		void fillPotentialConfigurationsFromPotentialCfgGraph(const bellesip::SDP::SDPPotentialCfgGraph::media_description_config & sdpCfgs);
		void createPotentialConfiguration(const unsigned int & idx, const tcap_map_t & protoMap, const std::list<acap_map_t> & attrList, const bool delete_session_attributes, const bool delete_media_attributes);

		void createActualCfg(const SalMediaDescription * salMediaDesc, const belle_sdp_media_description_t *media_desc);
		void setProtoInCfg(SalStreamConfiguration & cfg, const std::string & str);

		void addDtlsAttributesToMediaDesc(const SalStreamConfiguration & cfg, belle_sdp_media_description_t *media_desc) const;

		void setupRtcpFb(const bool nackEnabled, const bool tmmbrEnabled, const bool implicitRtcpFb);
		void setupRtcpXr(const OrtpRtcpXrConfiguration & rtcpXr);
		void setDirection(const SalStreamDir & newDir);
		void setBundleOnly(const bool enable);
		void setDtls(const SalDtlsRole role, const std::string & fingerprint = std::string());
		void setPtime(const int & ptime = -1, const int & maxptime = -1);
		void setCrypto(const size_t & idx, const SalSrtpCryptoAlgo & newCrypto);

		void addIceRemoteCandidatesToSdp(const SalStreamConfiguration & cfg, belle_sdp_media_description_t *md) const;
		void addIceCandidatesToSdp(const SalStreamConfiguration & cfg, belle_sdp_media_description_t *md) const;
		void addRtcpFbAttributesToSdp(const SalStreamConfiguration & cfg, belle_sdp_media_description_t *media_desc) const;
		bool isRtcpFbTrrIntTheSameForAllPayloads(const SalStreamConfiguration & cfg, uint16_t *trr_int) const;
		void addMidAttributesToSdp(const SalStreamConfiguration & cfg, belle_sdp_media_description_t *media_desc) const;
		void applyRtcpFbAttributeToPayload(SalStreamConfiguration & cfg, belle_sdp_rtcp_fb_attribute_t *fb_attribute, PayloadType *pt);
		bool sdpParseRtcpFbParameters(SalStreamConfiguration & cfg, const belle_sdp_media_description_t *media_desc);
		void sdpParsePayloadTypes(SalStreamConfiguration & cfg, const belle_sdp_media_description_t *media_desc) const;
		void sdpParseMediaCryptoParameters(SalStreamConfiguration & cfg, const belle_sdp_media_description_t *media_desc) const;
		void sdpParseMediaIceParameters(SalStreamConfiguration & cfg, const belle_sdp_media_description_t *media_desc);

		int globalEqual(const SalStreamDescription & other) const;
		int compareConfigurations(const SalStreamDescription & other, const SalStreamDescription::cfg_map::key_type & thisKey, const SalStreamDescription::cfg_map::key_type & otherKey) const;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SAL_STREAM_DESCRIPTION_H_
