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

#include "object/clonable-object-p.h"
#include "object/clonable-object.h"

#include "FlexiAPIClient.hh"

#include "liblinphone_tester.h"
#include "tester_utils.h"

#include <jsoncpp/json/json.h>

using namespace Json;

static void flexiapiPing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");

	auto flexiAPIClient = new FlexiAPIClient(marie->lc);

	const char *resolvedContent;
	int code = 0;
	int fetched = 0;

	flexiAPIClient
		->ping()
		->then([&resolvedContent, &code, &fetched](FlexiAPIClient::Response response) -> void {
			resolvedContent = response.body;
			code = response.code;
			fetched = 1;
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 2000);

	BC_ASSERT_STRING_EQUAL(resolvedContent, "pong");
	BC_ASSERT_EQUAL(code, 200, int, "%d");

	linphone_core_manager_destroy(marie);
	delete flexiAPIClient;
}

static void flexiapiAccounts() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");

	auto flexiAPIClient = new FlexiAPIClient(marie->lc);

	int code = 0;
	int fetched = 0;
	string resolvedDomain;

	// Unauthenticated
	flexiAPIClient
		->me()
		->then([&code, &fetched](FlexiAPIClient::Response response) -> void {
			code = response.code;
			fetched = 1;
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 2000);

	// The internal resolver will handle a 401 and then try to re-send
	// the request with a proper DIGEST authentication
	BC_ASSERT_EQUAL(code, 200, int, "%d");

	code = 0;
	fetched = 0;

	// Authenticated
	flexiAPIClient
		->me()
		->then([&code, &fetched, &resolvedDomain](FlexiAPIClient::Response response) -> void {
			code = response.code;
			resolvedDomain = response.json()["domain"].asString();
			fetched = 1;
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 2000);
	BC_ASSERT_EQUAL(code, 200, int, "%d");
	BC_ASSERT_STRING_EQUAL(resolvedDomain.c_str(), "sip.example.org");

	linphone_core_manager_destroy(marie);
	delete flexiAPIClient;
}

static void flexiapiChangeEmail() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie2_rc");

	auto flexiAPIClient = new FlexiAPIClient(marie->lc);

	int code = 0;
	int fetched = 0;
	string resolvedDomain;

	flexiAPIClient
		->emailChange("changed@test.com")
		->then([&code, &fetched](FlexiAPIClient::Response response) -> void {
			code = response.code;
			fetched = 1;
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 15000);
	BC_ASSERT_EQUAL(code, 200, int, "%d");

	linphone_core_manager_destroy(marie);
	delete flexiAPIClient;
}

/**
 * This test is only passing if the setting "everyone_is_admin" is set to true
 * on the API
 */
static void flexiapiCreateAccount() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie2_rc");

	auto flexiAPIClient = new FlexiAPIClient(marie->lc);

	int code = 0;
	int fetched = 0;
	int id = 0;
	string username = string("test_").append(sal_get_random_token(6));
	string resolvedDomain;
	bool activated = true;

	// Create the account
	flexiAPIClient
		->createAccount(username, "test", "MD5", activated)
		->then([&code, &fetched, &id](FlexiAPIClient::Response response) -> void {
			code = response.code;
			fetched = 1;
			id = response.json()["id"].asInt();
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 3000);
	BC_ASSERT_EQUAL(code, 200, int, "%d");

	code = 0;
	fetched = 0;
	string resolvedUsername;
	bool resolvedActivated;

	// Request it
	flexiAPIClient
		->account(id)
		->then([&code, &fetched, &resolvedUsername, &resolvedActivated](FlexiAPIClient::Response response) -> void {
			code = response.code;
			fetched = 1;
			resolvedUsername = response.json()["username"].asString();
			resolvedActivated = response.json()["activated"].asBool();
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 3000);
	BC_ASSERT_EQUAL(code, 200, int, "%d");
	BC_ASSERT_TRUE(resolvedActivated);
	BC_ASSERT_STRING_EQUAL(resolvedUsername.c_str(), username.c_str());

	code = 0;
	fetched = 0;

	// Destroy it
	flexiAPIClient
		->accountDelete(id)
		->then([&code, &fetched](FlexiAPIClient::Response response) -> void {
			code = response.code;
			fetched = 1;
		});

	wait_for_until(marie->lc, NULL, &fetched, 1, 3000);
	BC_ASSERT_EQUAL(code, 200, int, "%d");

	linphone_core_manager_destroy(marie);
	delete flexiAPIClient;
}

static void flexiapiChangePassword() {
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");

	// Resolve the password
	const LinphoneAddress *identityAddress = linphone_address_new(linphone_core_get_identity(pauline->lc));
	const LinphoneAuthInfo *authInfo = linphone_core_find_auth_info(
		pauline->lc,
		linphone_address_get_domain(identityAddress),
		linphone_address_get_username(identityAddress),
		NULL
	);

	const char* password = linphone_auth_info_get_password(authInfo);
	BC_ASSERT_PTR_NOT_NULL(password);

	auto flexiAPIClient = new FlexiAPIClient(pauline->lc);

	int code = 0;
	int fetched = 0;
	string resolvedDomain;

	flexiAPIClient
		->passwordChange("MD5", "new_password", password)
		->then([&code, &fetched](FlexiAPIClient::Response response) -> void {
			code = response.code;
			fetched = 1;
		});

	wait_for_until(pauline->lc, NULL, &fetched, 1, 3000);
	BC_ASSERT_EQUAL(code, 200, int, "%d");

	linphone_core_manager_destroy(pauline);
	delete flexiAPIClient;
}

test_t account_creator_flexiapi_tests[] = {
	TEST_NO_TAG("Ping", flexiapiPing),
	TEST_NO_TAG("Create Account", flexiapiCreateAccount),
	TEST_NO_TAG("Accounts", flexiapiAccounts),
	TEST_NO_TAG("Change Email", flexiapiChangeEmail),
	TEST_NO_TAG("Change Password", flexiapiChangePassword),
};

test_suite_t account_creator_flexiapi_suite = {
	"FlexiAPI Client", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(account_creator_flexiapi_tests) / sizeof(account_creator_flexiapi_tests[0]), account_creator_flexiapi_tests
};
