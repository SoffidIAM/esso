/*
 * logindialog.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef LOGINDIALOG_H_
#define LOGINDIALOG_H_

#include <string>

class LoginDialog
{
	public:
		LoginDialog ();
		int show ();
		std::wstring user;
		std::wstring password;
		HWND m_hWnd;
		std::wstring program;
};
#endif /* LOGINDIALOG_H_ */
