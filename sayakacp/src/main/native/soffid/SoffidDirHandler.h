/*
 * LocalAdminHandler.h
 *
 *  Created on: 30/03/2011
 *      Author: u07286
 */

#ifndef SOFFIDDIRNHANDLER_H_
#define SOFFIDDIRHANDLER_H_

#include <string>
#include "../Log.h"

class SoffidDirHandler {
public:
	SoffidDirHandler();
	virtual ~SoffidDirHandler();
	bool valid;
	bool error;
	bool mustChange;

	std::wstring szUser;
	std::wstring szOldPassword;
	std::wstring szPassword;
	std::wstring szErrorMessage;
	std::wstring szHostName;

	void validate (std::wstring &szUser, std::wstring &szPassword);

	void validate (std::wstring &szUser, std::wstring &szPassword, std::wstring &szNewPassword);
private:
	void writeLine(SOCKET socket, const std::wstring &line) ;
	std::wstring readLine(SOCKET socket);
	void storeCredentials();
	SOCKET connect ();
	Log m_log;
};

#endif /* LOCALADMINHANDLER_H_ */
