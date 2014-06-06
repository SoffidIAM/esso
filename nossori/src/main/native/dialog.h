/*
 * logindialog.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef LOGINDIALOG_H_
#define LOGINDIALOG_H_

#include <string>
#include <vector>

class NossoriDialog
{
	public:
		NossoriDialog ();
		int showUser ();
		int showQuestion ();
		int showPassword ();

		std::wstring user;
		std::wstring password;
		std::wstring desiredPassword;
		std::vector<std::wstring> m_questions;
		std::vector<std::wstring> m_answers;
		int questionNumber;

		HWND m_hWnd;
		enum {
			SHOW_USER, SHOW_QUESTION, SHOW_PASSWORD
		} status;

		bool onOk ();
		bool onStartup();
		bool onCancel ();
};
#endif /* LOGINDIALOG_H_ */
