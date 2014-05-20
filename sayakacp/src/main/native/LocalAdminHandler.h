/*
 * LocalAdminHandler.h
 *
 *  Created on: 30/03/2011
 *      Author: u07286
 */

#ifndef LOCALADMINHANDLER_H_
#define LOCALADMINHANDLER_H_

#include <string>

class LocalAdminHandler {
public:
	LocalAdminHandler();
	virtual ~LocalAdminHandler();

	std::wstring szUser;
	std::wstring szPassword;
	std::wstring szHostName;
	std::wstring szErrorMessage;

	bool getAdminCredentials (std::wstring &szUser, std::wstring &szPassword);
};

#endif /* LOCALADMINHANDLER_H_ */
