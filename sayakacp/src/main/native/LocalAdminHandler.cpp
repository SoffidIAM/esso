/*
 * LocalAdminHandler.cpp
 *
 *  Created on: 30/03/2011
 *      Author: u07286
 */

#include <windows.h>
#include "LocalAdminHandler.h"
#include <ctype.h>
#include <ssoclient.h>
#include <stdlib.h>
#include <MZNcompat.h>

# include "Utils.h"

LocalAdminHandler::LocalAdminHandler() {

}

LocalAdminHandler::~LocalAdminHandler() {
}

bool LocalAdminHandler::getAdminCredentials (std::wstring &szUser, std::wstring &szPassword) {
	SeyconCommon::updateHostAddress();

	SeyconService service;

	wchar_t wchComputerName [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW (wchComputerName, &dwSize);

	szHostName.assign (wchComputerName);


	std::wstring szHostName = MZNC_strtowstr(MZNC_getHostName());

	std::wstring pass = service.escapeString(szPassword.c_str());
	SeyconResponse * response = service.sendUrlMessage(
			L"gethostadmin?user=%ls&pass=%ls&host=%ls",
			service.escapeString(szUser.c_str()).c_str(),
			service.escapeString(pass.c_str()).c_str(),
			service.escapeString(szHostName.c_str()).c_str());

	if (response == NULL) {
		szErrorMessage.assign (
				MZNC_strtowstr(Utils::LoadResourcesString(9).c_str()).c_str());
		return false;
	}
	else
	{
		std::string status = response->getToken(0);
		if (status == "OK") {
			response->getToken(1, this->szUser);
			response->getToken(2, this->szPassword);
			delete response;
			return true;
		}
		else
		{
			response->getToken(1, szErrorMessage);
			delete response;
		    return false;
		}
	}
}
