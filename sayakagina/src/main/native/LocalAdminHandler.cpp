/*
 * LocalAdminHandler.cpp
 *
 *  Created on: 30/03/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "LocalAdminHandler.h"
#include <ctype.h>
#include <ssoclient.h>
#include <stdlib.h>
#include "winwlx.h"
#include <MZNcompat.h>
# include "Utils.h"

LocalAdminHandler::LocalAdminHandler() {
	// TODO Auto-generated constructor stub

}

LocalAdminHandler::~LocalAdminHandler() {
	// TODO Auto-generated destructor stub
}

bool LocalAdminHandler::getAdminCredentials (std::wstring &szUser,
	std::wstring &szPassword, std::wstring &szHostName)
{
	SeyconService service;
	Log l("LocalAdminHandler");

	SeyconCommon::updateHostAddress();

	std::wstring szHost = MZNC_strtowstr(MZNC_getHostName());

	std::wstring wPass = service.escapeString (szPassword.c_str());
	l.info ("Asking for admin user %ls (%ls)", szUser.c_str(), szHostName.c_str());
	SeyconResponse *response = service.sendUrlMessage (
		L"gethostadmin?user=%ls&pass=%ls&host=%ls", szUser.c_str(), wPass.c_str(), szHost.c_str());

	if (response == NULL)
		return false;
	else
	{
		std::string token = response -> getToken(0);
		if (token == "OK") {
			response -> getToken (1, this->szUser);
			response -> getToken (2, this->szPassword);
			l.info ("Admin user = %s", this->szUser.c_str());
			delete response;
			return true;
		}
		else
		{
			std::wstring cause;
			response->getToken(1, cause);

			l.info ("Response = %ls", cause.c_str());
			pWinLogon->WlxMessageBox(hWlx, NULL, cause.c_str(),
					MZNC_strtowstr(Utils::LoadResourcesString(19).c_str()).c_str(),
					MB_OK|MB_ICONEXCLAMATION);

			delete response;
		    return false;
		}
	}
}
