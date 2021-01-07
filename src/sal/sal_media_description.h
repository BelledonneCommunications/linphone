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

#ifndef _SAL_MEDIA_DESCRIPTION_H_
#define _SAL_MEDIA_DESCRIPTION_H_

#include <list>
#include <string>
#include <vector>

#include "c-wrapper/internal/c-sal.h"
#include "sal/sal_stream_description.h"
#include "sal/sal_stream_bundle.h"
#include "ortp/rtpsession.h"

class SalStreamDescription;

class LINPHONE_PUBLIC SalMediaDescription {
	public:
		SalMediaDescription();
		SalMediaDescription(const SalMediaDescription & other);
		virtual ~SalMediaDescription();
		void init();
		void destroy();

		void addNewBundle(const SalStreamBundle & bundle);

		int lookupMid(const std::string mid) const;
		const std::list<SalStreamBundle>::const_iterator getBundleFromMid(const std::string mid) const;
		int getIndexOfTransportOwner(const SalStreamDescription & sd) const;

		const std::vector<SalStreamDescription>::const_iterator findStream(SalMediaProto proto, SalStreamType type) const;
		unsigned int nbActiveStreamsOfType(SalStreamType type) const;
		const std::vector<SalStreamDescription>::const_iterator getActiveStreamOfType(SalStreamType type, unsigned int idx) const;
		const std::vector<SalStreamDescription>::const_iterator findSecureStreamOfType(SalStreamType type) const;
		const std::vector<SalStreamDescription>::const_iterator findBestStream(SalStreamType type) const;

		bool isEmpty() const;

		void setDir(SalStreamDir stream_dir);

		int getNbActiveStreams() const;

		bool hasDir(const SalStreamDir & stream_dir) const;
		bool hasAvpf() const;
		bool hasImplicitAvpf() const;
		bool hasSrtp() const;
		bool hasDtls() const;
		bool hasZrtp() const;
		bool hasIpv6() const;

		bool operator==(const SalMediaDescription & other) const;
		int equal(const SalMediaDescription & otherMd) const;
		int globalEqual(const SalMediaDescription & otherMd) const;

		static const std::string printDifferences(int result);

		size_t getNbStreams() const;
		const std::string & getAddress() const;
		const std::vector<SalStreamDescription>::const_iterator getStreamIdx(unsigned int idx) const;

	std::string name;
	std::string addr;
	std::string username;
	int bandwidth = 0;
	unsigned int session_ver = 0;
	unsigned int session_id = 0;
	SalStreamDir dir = SalStreamSendRecv;
	std::vector<SalStreamDescription> streams;
	SalCustomSdpAttribute *custom_sdp_attributes = nullptr;
	OrtpRtcpXrConfiguration rtcp_xr;
	std::string ice_ufrag;
	std::string ice_pwd;
	std::list<SalStreamBundle> bundles;
	bool ice_lite = false;
	bool set_nortpproxy = false;
	bool accept_bundles = false; /* Set to true if RTP bundles can be accepted during offer answer. This field has no appearance on the SDP.*/
	std::vector<bool> pad;

	private:
		/*check for the presence of at least one stream with requested direction */
		bool containsStreamWithDir(const SalStreamDir & stream_dir) const; 

		bool isNullAddress(const std::string & addr) const;

};

#endif // ifndef _SAL_MEDIA_DESCRIPTION_H_
