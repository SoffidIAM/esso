/*
 * pam.cpp
 *
 *  Created on: 19/05/2011
 *      Author: u07286
 */

#include "PamHandler.h"
#include "Log.h"
#include <MZNcompat.h>
#include <stdlib.h>
#include <stdio.h>
#include <ssoclient.h>
#include <string.h>

extern "C"
int pam_sm_authenticate(pam_handle_t *pamh, int flags,
		int argc, const char **argv)
{
	PamHandler::getPamHandler(pamh)->parse(argc, argv);

	return PamHandler::getPamHandler(pamh)->authenticate();
}

extern "C"
int pam_sm_setcred(pam_handle_t *pamh,int flags,int argc,
		const char **argv)
{
	return (PAM_SUCCESS);
}

extern "C"
int pam_sm_open_session(pam_handle_t *pamh, int flags,
		       int argc, const char **argv) {
	PamHandler::getPamHandler(pamh)->parse(argc, argv);

	return PamHandler::getPamHandler(pamh)->createSession();
}

extern "C"
int pam_sm_close_session(pam_handle_t *pamh, int flags,
			int argc, const char **argv) {

	return PamHandler::getPamHandler(pamh)->closeSession();
}


extern "C"
int pam_sm_chauthtok(pam_handle_t *pamh, int flags,
				int argc, const char **argv) {
	PamHandler::getPamHandler(pamh)->parse(argc, argv);

	if (flags & PAM_UPDATE_AUTHTOK)
		return PamHandler::getPamHandler(pamh)->changePassword();
	else
		return PAM_SUCCESS;
}
