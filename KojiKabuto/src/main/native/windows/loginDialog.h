/*
 * logindialog.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef LOGINDIALOG_H_
#define LOGINDIALOG_H_

#include <resource.h>
#include <string>
#include <windows.h>
#include <wchar.h>
#include "loginDialog.h"
#include "Utils.h"
#include "lm.h"
#include "winwlx.h"
#include <windowsx.h>

#include <MZNcompat.h>
#include <string.h>

class LoginDialog
{

	private:

		/** @brief User ID
		 *
		 */
		std::wstring userID;

		/** @brief User password
		 *
		 */
		std::wstring userPassword;

		/** @brief Window handler
		 *
		 */
		HWND m_hWnd;


	public:

		/** @brief Default constructor
		 *
		 */
		LoginDialog ();

		/** @brief Get user ID
		 *
		 * @return User ID
		 */
		const std::wstring& getUserId() const;

		/** @brief Set user ID
		 *
		 * @param userId User ID
		 */
		void setUserId(const std::wstring& userId);

		/** @brief Get user password
		 *
		 * @return User password
		 */
		const std::wstring& getUserPassword() const;

		/** @brief Set user password
		 *
		 * @param userPassword User password
		 */
		void setUserPassword(const std::wstring& userPassword);

		/** @brief Show dialog
		 *
		 * Method that implements the functionality to show login dialog for manual access.
		 * @return Result of dialog process.
		 */
		int show ();

		/** @brief Login dialog process
		 *
		 * Method that implements the functionality for login dialog process.
		 * @param hwndDlg	Handle to dialog box
		 * @param uMsg 		Dialog message
		 * @param wParam		First message parameter
		 * @param lParam		Second message parameter
		 */
		static INT_PTR CALLBACK loginDialogProc(HWND hwndDlg, UINT uMsg,
				WPARAM wParam, LPARAM lParam);

		/** @brief Raise loop
		 *
		 * Implements the functionality to show dialog if it is minimized.
		 *  @param param	Handler to loop dialog.
		 */
		static DWORD CALLBACK raiseLoop (LPVOID param);

		bool hidden;
};
#endif /* LOGINDIALOG_H_ */
