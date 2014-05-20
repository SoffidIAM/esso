/*
 * TokenHandler.cpp
 *
 *  Created on: 09/02/2011
 *      Author: u07286
 */

#include "TokenHandler.h"
#include <windows.h>
#include "Utils.h"

TokenHandler::TokenHandler() {
	m_handler = NULL;
}

TokenHandler::~TokenHandler() {
}


const char *TokenHandler::getTokenDescription () {
	if (strcmp ((char*) m_tokenManufacturer, "DGP-FNMT") == 0)
		return Utils::LoadResourcesString(20).c_str();
	if (strncmp ((char*) m_tokenManufacturer, "ST Incard", 9) == 0 &&
			strncmp ((char*) m_tokenModel, "T&S DS/2048", 11) == 0 )
		return Utils::LoadResourcesString(21).c_str();
	char achToken[33];
	strncpy (achToken, (const char*)m_tokenManufacturer, sizeof m_tokenManufacturer);
	achToken[32] = '\0';
	m_name = Utils::LoadResourcesString(22);
	m_name.append(achToken);
	return m_name.c_str();
}

bool TokenHandler::isDNIe () {
	if (strcmp ((char*) m_tokenManufacturer, "DGP-FNMT") == 0)
		return true;
	else
		return true;
}
