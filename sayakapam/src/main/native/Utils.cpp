/*
 * Utils.cpp
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */


#include "sayaka.h"
#include "Utils.h"
#include "Log.h"
#include <stdlib.h>
#include <string.h>

Utils::Utils() {
}

Utils::~Utils() {
}



bool Utils::duplicateString (char* &szResult, const char* sz) {
	if (sz == NULL) {
		szResult = NULL;
		return false;
	}
	else
	{
		szResult = (char*)  malloc (strlen(sz)+1);
		strcpy (szResult, sz);
		return true;
	}

}

void Utils::freeString (char* &wsz) {
	if (wsz != NULL) {
		memset (wsz, 0, strlen(wsz));
		free (wsz);
		wsz = NULL;
	}
}


bool Utils::changePassword (char *achUser, char*achPass, char *achNewPassword,
		std::string& szStatusText)
{
	return false;
}
