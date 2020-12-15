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

#include "linphone/account_creator.h"
#include "linphone/core.h"
#include "linphone/lpconfig.h"

#include "account_creator_flexiapi.hh"

#include "c-wrapper/c-wrapper.h"
#include "dial-plan/dial-plan.h"

#include "bctoolbox/crypto.h"
#include "bctoolbox/regex.h"

#include "private.h"

#include <jsoncpp/json/json.h>
#include <functional>

using namespace LinphonePrivate;
using namespace std;

AccountCreatorFlexiAPI::AccountCreatorFlexiAPI(LinphoneCore *lc) {
    mCore = lc;
    apiKey = nullptr;
    callbacks mRequestCallbacks = *new callbacks();

    // Assign the core there as well to keep it in the callback contexts
    mRequestCallbacks.core = lc;
}

AccountCreatorFlexiAPI* AccountCreatorFlexiAPI::ping() {
    prepareRequest("ping");
    return this;
}

AccountCreatorFlexiAPI* AccountCreatorFlexiAPI::me() {
    prepareRequest("accounts/me");
    return this;
}

AccountCreatorFlexiAPI* AccountCreatorFlexiAPI::setApiKey(const char* key) {
    apiKey = key;
    return this;
}

AccountCreatorFlexiAPI* AccountCreatorFlexiAPI::then(function<void (AccountCreatorFlexiAPI::Response)> success) {
    mRequestCallbacks.success = success;
    return this;
}

AccountCreatorFlexiAPI* AccountCreatorFlexiAPI::error(function<void (AccountCreatorFlexiAPI::Response)> error) {
    mRequestCallbacks.error = error;
    return this;
}

void AccountCreatorFlexiAPI::prepareRequest(const char* path) {
    belle_http_request_listener_callbacks_t internalCallbacks = {};
    belle_http_request_listener_t *listener;
    belle_http_request_t *req;

    string uri = "https://subscribe.linphone.org/api/";

    //LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(mCore);
    //char *addr = linphone_address_as_string_uri_only(linphone_proxy_config_get_identity_address(cfg));
    const char *addr = "sip:+33667545663@sip.linphone.org";

    req = belle_http_request_create("GET", belle_generic_uri_parse(uri.append(path).c_str()),
        belle_sip_header_content_type_create("application", "json"),
        belle_sip_header_accept_create("application", "json"),
        belle_http_header_create("From", addr),
    NULL);

    if (apiKey != nullptr) {
        belle_sip_message_add_header(
            BELLE_SIP_MESSAGE(req),
            belle_http_header_create("x-api-key", apiKey)
        );
    }

    //bctbx_free(addr);

    internalCallbacks.process_response = processResponse;
    internalCallbacks.process_auth_requested = processAuthRequested;
    listener = belle_http_request_listener_create_from_callbacks(&internalCallbacks, &mRequestCallbacks);

    belle_http_provider_send_request(mCore->http_provider, req, listener);
    belle_sip_object_data_set(BELLE_SIP_OBJECT(req), "listener", listener, belle_sip_object_unref);
}

void AccountCreatorFlexiAPI::processResponse(void *ctx, const belle_http_response_event_t *event) {
    auto cb = (callbacks_t *)ctx;

    if (event->response){
        int code = belle_http_response_get_status_code(event->response);
        auto response = *new AccountCreatorFlexiAPI::Response();
        response.code = code;

        if (code >= 200 && code < 300) {
            belle_sip_body_handler_t *body = belle_sip_message_get_body_handler(BELLE_SIP_MESSAGE(event->response));
            const char *content = belle_sip_object_to_string(body);

            response.body = content;
            cb->success(response);
        } else {
            cb->error(response);
        }
    }
}

void AccountCreatorFlexiAPI::processAuthRequested(void *ctx, belle_sip_auth_event_t *event) {
    auto cb = (callbacks_t *)ctx;

    const char *username = belle_sip_auth_event_get_username(event);
    const char *domain = belle_sip_auth_event_get_domain(event);

    linphone_core_fill_belle_sip_auth_event(cb->core, event, username, domain);
}