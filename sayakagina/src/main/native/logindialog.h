/*
 * logindialog.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef LOGINDIALOG_H_
#define LOGINDIALOG_H_

#include <string>
class LoginDialog {
public:
	LoginDialog ();
	int show ();
	std::wstring user;
	std::wstring password;
	std::wstring domain;
	std::wstring hostName;
	std::wstring domainName;
	HWND m_hWnd;
};
#endif /* LOGINDIALOG_H_ */
