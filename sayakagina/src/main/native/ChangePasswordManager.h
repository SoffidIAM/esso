/*
 * ChangePasswordManager.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef CHANGEPASSWORDMANAGER_H_
#define CHANGEPASSWORDMANAGER_H_

#include "Log.h"

class ChangePasswordManager {
public:
	ChangePasswordManager();
	virtual ~ChangePasswordManager();
	bool changePassword (std::wstring &szUser, std::wstring &szDomain, std::wstring &szPassword);
	std::wstring *szUser;
	std::wstring *szDomain;
	std::wstring *szPassword;
	std::wstring szNewPassword1;
	std::wstring szNewPassword2;
private:
	Log m_log;
};

#endif /* CHANGEPASSWORDMANAGER_H_ */
