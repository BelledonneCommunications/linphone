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

#include "linphone/core.h"
#include "liblinphone_tester.h"
#include "tester_utils.h"

static void linphone_session_timer_disabled(void)
{
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new("pauline_rc");

	LinphoneCall* out_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	linphone_call_accept(pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallConnected, 1));

	// Marie INVITE
	const LinphoneCallParams *marie_params = linphone_call_get_remote_params(pauline_call);

	BC_ASSERT_PTR_NULL(linphone_call_params_get_custom_header(marie_params, "Session-Expires"));
	BC_ASSERT_PTR_NULL(linphone_call_params_get_custom_header(marie_params, "Min-SE"));

	const char *marie_value_require = linphone_call_params_get_custom_header(marie_params, "Require");
	BC_ASSERT_PTR_NULL(marie_value_require);


	const char *marie_value_supported = linphone_call_params_get_custom_header(marie_params, "Supported");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_supported)) {
		BC_ASSERT_PTR_NULL(strstr(marie_value_supported, "timer"));
	}

	// Pauline 200 OK
	const LinphoneCallParams *pauline_params = linphone_call_get_remote_params(out_call);

	BC_ASSERT_PTR_NULL(linphone_call_params_get_custom_header(pauline_params, "Session-Expires"));
	BC_ASSERT_PTR_NULL(linphone_call_params_get_custom_header(pauline_params, "Min-SE"));

	const char *value_require = linphone_call_params_get_custom_header(pauline_params, "Require");
	BC_ASSERT_PTR_NULL(value_require);

	const char *value_supported = linphone_call_params_get_custom_header(pauline_params, "Supported");
	if (BC_ASSERT_PTR_NOT_NULL(value_supported)) {
		BC_ASSERT_PTR_NULL(strstr(value_supported, "timer"));
	}

	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void linphone_session_timer_invite_interval_too_small(void)
{
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_set_session_expires_value(marie->lc, 300);

	pauline = linphone_core_manager_new("pauline_rc");
	linphone_core_set_session_expires_value(pauline->lc, 1000);

	LinphoneCall* out_call = linphone_core_invite_address(marie->lc, pauline->identity);
	linphone_call_ref(out_call); // Keep the reference, we'll clean it later

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));

	// Test the intermediary package
	//BC_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallError, 1, 1000));

	//BC_ASSERT_EQUAL(linphone_call_get_reason(out_call), LinphoneReasonSessionIntervalTooSmall, int, "%d");

	//const LinphoneErrorInfo *errorInfo = linphone_call_get_error_info(out_call);
	//BC_ASSERT_PTR_NOT_NULL(errorInfo);
	//if (errorInfo){
	//	BC_ASSERT_EQUAL(linphone_error_info_get_protocol_code(errorInfo), 422, int, "%d");
	//	BC_ASSERT_PTR_NOT_NULL(linphone_error_info_get_phrase(errorInfo));
	//	BC_ASSERT_STRING_EQUAL(linphone_error_info_get_phrase(errorInfo), "Session Interval Too Small");
	//	BC_ASSERT_STRING_EQUAL(linphone_error_info_get_protocol(errorInfo), "SIP");
	//}
	// Test presence of the Min-SE as well

	pauline_call = linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	linphone_call_accept(pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallConnected, 1));

	// Pauline 200 OK
	const LinphoneCallParams *pauline_params = linphone_call_get_remote_params(out_call);

	const char *value_session_expires = linphone_call_params_get_custom_header(pauline_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(value_session_expires, "1000;refresher=uas") == 0);
	}

	const char *value_require = linphone_call_params_get_custom_header(pauline_params, "Require");
	if (BC_ASSERT_PTR_NOT_NULL(value_require)) {
		BC_ASSERT_TRUE(strcmp(value_require, "timer") == 0);
	}

	const char *value_supported = linphone_call_params_get_custom_header(pauline_params, "Supported");
	if (BC_ASSERT_PTR_NOT_NULL(value_supported)) {
		BC_ASSERT_PTR_NOT_NULL(strstr(value_supported, "timer"));
	}

	linphone_call_unref(out_call);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void linphone_session_timer_invite_interval_ok_refresher_none_uas(void)
{
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_set_session_expires_value(marie->lc, 300);

	pauline = linphone_core_manager_new("pauline_rc");
	linphone_core_set_session_expires_value(pauline->lc, 200);

	LinphoneCall* out_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	linphone_call_accept(pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallConnected, 1));

	// Marie INVITE
	const LinphoneCallParams *marie_params = linphone_call_get_remote_params(pauline_call);

	const char *marie_value_session_expires = linphone_call_params_get_custom_header(marie_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(marie_value_session_expires, "300") == 0);
	}

	const char *marie_value_supported = linphone_call_params_get_custom_header(marie_params, "Supported");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_supported)) {
		BC_ASSERT_PTR_NOT_NULL(strstr(marie_value_supported, "timer"));
	}

	// Min-SE = 90 by default
	const char *marie_value_minse = linphone_call_params_get_custom_header(marie_params, "Min-SE");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_minse)) {
		BC_ASSERT_TRUE(strcmp(marie_value_minse, "90") == 0);
	}

	// Pauline 200 OK
	const LinphoneCallParams *pauline_params = linphone_call_get_remote_params(out_call);

	const char *value_session_expires = linphone_call_params_get_custom_header(pauline_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(value_session_expires, "300;refresher=uas") == 0);
	}

	const char *value_require = linphone_call_params_get_custom_header(pauline_params, "Require");
	if (BC_ASSERT_PTR_NOT_NULL(value_require)) {
		BC_ASSERT_TRUE(strcmp(value_require, "timer") == 0);
	}

	const char *value_supported = linphone_call_params_get_custom_header(pauline_params, "Supported");
	if (BC_ASSERT_PTR_NOT_NULL(value_supported)) {
		BC_ASSERT_PTR_NOT_NULL(strstr(value_supported, "timer"));
	}

	linphone_call_terminate(out_call);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void linphone_session_timer_invite_interval_ok_refresher_uac_uac(void)
{
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_set_session_expires_value(marie->lc, 300);
	linphone_core_set_session_expires_refresher_value(marie->lc, (int)BELLE_SIP_HEADER_SESSION_EXPIRES_UAC);
	linphone_core_set_session_expires_min_value(marie->lc, 100);

	pauline = linphone_core_manager_new("pauline_rc");
	linphone_core_set_session_expires_value(pauline->lc, 200);

	LinphoneCall* out_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	linphone_call_accept(pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallConnected, 1));

	// Marie INVITE
	const LinphoneCallParams *marie_params = linphone_call_get_remote_params(pauline_call);

	const char *marie_value_session_expires = linphone_call_params_get_custom_header(marie_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(marie_value_session_expires, "300;refresher=uac") == 0);
	}

	const char *marie_value_minse = linphone_call_params_get_custom_header(marie_params, "Min-SE");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_minse)) {
		BC_ASSERT_TRUE(strcmp(marie_value_minse, "100") == 0);
	}

	// Pauline 200 OK
	const LinphoneCallParams *pauline_params = linphone_call_get_remote_params(out_call);

	const char *value_session_expires = linphone_call_params_get_custom_header(pauline_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(value_session_expires, "300;refresher=uac") == 0);
	}

	linphone_call_terminate(out_call);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void linphone_session_timer_invite_interval_ok_refresher_uas_uas(void)
{
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_set_session_expires_value(marie->lc, 300);
	linphone_core_set_session_expires_refresher_value(marie->lc, (int)BELLE_SIP_HEADER_SESSION_EXPIRES_UAS);

	pauline = linphone_core_manager_new("pauline_rc");
	linphone_core_set_session_expires_value(pauline->lc, 200);

	LinphoneCall* out_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	linphone_call_accept(pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallConnected, 1));

	// Marie INVITE
	const LinphoneCallParams *marie_params = linphone_call_get_remote_params(pauline_call);

	const char *marie_value_session_expires = linphone_call_params_get_custom_header(marie_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(marie_value_session_expires, "300;refresher=uas") == 0);
	}

	// Pauline 200 OK
	const LinphoneCallParams *pauline_params = linphone_call_get_remote_params(out_call);

	const char *value_session_expires = linphone_call_params_get_custom_header(pauline_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(value_session_expires, "300;refresher=uas") == 0);
	}

	linphone_call_terminate(out_call);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

static void linphone_session_timer_invite_interval_ok_refresher_none_uac(void)
{
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;
	LinphoneCall *pauline_call;

	marie = linphone_core_manager_new("marie_rc");
	linphone_core_set_session_expires_value(marie->lc, 300);

	pauline = linphone_core_manager_new("pauline_rc");
	linphone_core_set_session_expires_value(pauline->lc, 200);
	linphone_core_set_session_expires_refresher_value(pauline->lc, (int)BELLE_SIP_HEADER_SESSION_EXPIRES_UAC);

	LinphoneCall* out_call = linphone_core_invite_address(marie->lc, pauline->identity);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallIncomingReceived, 1));

	pauline_call = linphone_core_get_current_call(pauline->lc);
	linphone_call_accept(pauline_call);

	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallConnected, 1));

	// Marie INVITE
	const LinphoneCallParams *marie_params = linphone_call_get_remote_params(pauline_call);

	const char *marie_value_session_expires = linphone_call_params_get_custom_header(marie_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(marie_value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(marie_value_session_expires, "300") == 0);
	}

	// Pauline 200 OK
	const LinphoneCallParams *pauline_params = linphone_call_get_remote_params(out_call);

	const char *value_session_expires = linphone_call_params_get_custom_header(pauline_params, "Session-Expires");
	if (BC_ASSERT_PTR_NOT_NULL(value_session_expires)) {
		BC_ASSERT_TRUE(strcmp(value_session_expires, "300;refresher=uac") == 0);
	}

	linphone_call_terminate(out_call);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(marie);
}

test_t session_timers_tests[] = {
	TEST_ONE_TAG("Session timer disabled", linphone_session_timer_disabled, "Session Timer"),
	TEST_ONE_TAG("Session timer invite 422", linphone_session_timer_invite_interval_too_small, "Session Timer"),

	// https://tools.ietf.org/html/rfc4028#page-15
	TEST_ONE_TAG("Session timer invite OK, c = none, s = auto", linphone_session_timer_invite_interval_ok_refresher_none_uas, "Session Timer"),
	TEST_ONE_TAG("Session timer invite OK, c = uac, s = auto", linphone_session_timer_invite_interval_ok_refresher_uac_uac, "Session Timer"),
	TEST_ONE_TAG("Session timer invite OK, c = uas, s = auto", linphone_session_timer_invite_interval_ok_refresher_uas_uas, "Session Timer"),
	TEST_ONE_TAG("Session timer invite OK, c = none, s = uac", linphone_session_timer_invite_interval_ok_refresher_none_uac, "Session Timer"),
};

test_suite_t session_timers_test_suite = {"Session Timers", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
										  sizeof(session_timers_tests) / sizeof(session_timers_tests[0]),
										  session_timers_tests};
