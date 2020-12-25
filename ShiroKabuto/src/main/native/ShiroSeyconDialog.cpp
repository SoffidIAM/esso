/*
 * ShiroLoginDialog.cpp
 *
 *  Created on: Nov 27, 2020
 *      Author: gbuades
 */

#include "ShiroSeyconDialog.h"

ShiroSeyconDialog::~ShiroSeyconDialog() {

}

bool ShiroSeyconDialog::askCard(const char *targeta, const char *cella,
		std::wstring &result) {
	return false;
}

bool ShiroSeyconDialog::askNewPassword(const char *reason,
		std::wstring &password) {
	printf("Asking for new password\n");
	if (newPass.length() > 0 && ! passwordUsed) {
		passwordUsed = true;
		password = newPass;
		return true;
	} else {
		needsNewPassword = true;
		return false;
	}
}

DuplicateSessionAction ShiroSeyconDialog::askDuplicateSession(
		const char *details) {
	return DuplicateSessionAction::dsaCancel;
}

bool ShiroSeyconDialog::askAllowRemoteLogout() {
	return false;
}

void ShiroSeyconDialog::notify(const char *message) {
	printf("Notice: %s\n", message);
}

void ShiroSeyconDialog::progressMessage(const char *message) {
	printf("Notice: %s\n", message);
}

