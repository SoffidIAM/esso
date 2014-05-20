/*
 * ChangePasswordManager.cpp
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "Utils.h"
#include <windows.h>
#include <lm.h>
#include "ChangePasswordManager.h"
#include "winwlx.h"

ChangePasswordManager::ChangePasswordManager():
	m_log ("ChangePasswordManager")
{
}

ChangePasswordManager::~ChangePasswordManager() {
}

static DWORD __stdcall ChangePasswordDialogProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam
) {
	ChangePasswordManager *init = (ChangePasswordManager*) GetWindowLongA(hwnd, DWL_USER);
    switch (message)
    {
    case WM_INITDIALOG:
		{
			init = (ChangePasswordManager*) lParam;
			SetWindowLong (hwnd, DWL_USER, (DWORD) init);
			HWND hwnd2 = GetDlgItem (hwnd, IDC_DOMINI);
			SetWindowTextW (hwnd2, init->szUser->c_str());
			hwnd2 = GetDlgItem(hwnd, IDC_PASSWORD0);
			if (init->szPassword->empty()) {
				SetFocus (hwnd2);
			} else {
				SetWindowTextW (hwnd2, init->szPassword->c_str());
				LONG style = GetWindowLong (hwnd2, GWL_STYLE);
				style = style | WS_DISABLED;
				SetWindowLong (hwnd2, GWL_STYLE, style);
				SetFocus (GetDlgItem(hwnd, IDC_PASSWORD1));
			}
			SendMessage (GetDlgItem(hwnd, IDC_PASSWORD0), EM_SETPASSWORDCHAR, '*', 0);
			SendMessage (GetDlgItem(hwnd, IDC_PASSWORD1), EM_SETPASSWORDCHAR, '*', 0);
			SendMessage (GetDlgItem(hwnd, IDC_PASSWORD2), EM_SETPASSWORDCHAR, '*', 0);
		}
        return TRUE;
    case WM_COMMAND:
    	if (wParam == IDOK)
    	{
    		Utils::getDlgCtrlText(hwnd, IDC_PASSWORD1, init->szNewPassword1);
    		Utils::getDlgCtrlText(hwnd, IDC_PASSWORD2, init->szNewPassword2);
    		Utils::getDlgCtrlText(hwnd, IDC_PASSWORD0, * init->szPassword);
			if (init->szNewPassword1 == init->szNewPassword2)
				EndDialog(hwnd, 0);
			else
				MessageBox(
						hwnd,
						Utils::LoadResourcesString(8).c_str(),
						Utils::LoadResourcesString(1000).c_str(),
						MB_OK| MB_ICONWARNING);
    	}
    	if (wParam == IDCANCEL)
			EndDialog(hwnd, 1);
        return TRUE;
    case WM_HSCROLL:
        return 0;
    case WM_DESTROY:
        return TRUE;
    case WM_CLOSE:
		EndDialog(hwnd, 1);
        DestroyWindow (hwnd);
        return TRUE;
    }
    return FALSE;
}

bool ChangePasswordManager::changePassword(std::wstring & szUser,
		std::wstring & szDomain, std::wstring & szPassword)
{
	this->szDomain = &szDomain;
	this->szUser = &szUser;
	this->szPassword = &szPassword;

	int ok = 0;
	while (!ok) {
		DWORD result = DialogBoxParam (hSayakaDll, (LPCSTR) IDD_CHANGEPASS, NULL,
				(DLGPROC) ChangePasswordDialogProc, (LPARAM) this);

		if (result != 0)
			return false;
		else
		{
			m_log.info (L"Changing password on %s for %s: %s -> %s",
					szDomain.c_str(),
					szUser.c_str(),
					szPassword.c_str(),
					szNewPassword1.c_str());
			result = NetUserChangePassword(
					szDomain.c_str(),
					szUser.c_str(),
					szPassword.c_str(),
					szNewPassword1.c_str());

			WLX_MPR_NOTIFY_INFO ni;

			ni.pszUserName = (wchar_t*) malloc (sizeof(wchar_t) * (1+szUser.length()));
			wcscpy (ni.pszUserName, szUser.c_str());
			ni.pszDomain = (wchar_t*) malloc (sizeof(wchar_t) * (1+szDomain.length()));
			wcscpy (ni.pszDomain, szDomain.c_str());
			ni.pszPassword = (wchar_t*) malloc (sizeof(wchar_t) * (1+szNewPassword1.length()));
			wcscpy (ni.pszPassword, szNewPassword1.c_str());
			ni.pszOldPassword = (wchar_t*) malloc (sizeof(wchar_t) * (1+szPassword.length()));
			wcscpy (ni.pszOldPassword, szPassword.c_str());

			m_log.info ("Changed");

			pWinLogon->WlxChangePasswordNotify(hWlx, &ni, 1 /*WN_VALID_LOGON_ACCOUNT*/);

			m_log.info ("Notified");

			if (result == NERR_Success)
			{
				m_log.info ("OK");
				szPassword = szNewPassword1;
				ok = 1;
			}
			else if (result == ERROR_ACCESS_DENIED)
			{
				ok = 0;
				MessageBox(NULL,
						Utils::LoadResourcesString(9).c_str(),
						Utils::LoadResourcesString(1000).c_str(),
						MB_OK| MB_ICONWARNING);
			}
			else if (result == ERROR_INVALID_PASSWORD)
			{
				ok = 0;
				MessageBox(NULL,
						Utils::LoadResourcesString(10).c_str(),
						Utils::LoadResourcesString(1000).c_str(),
						MB_OK| MB_ICONWARNING);
				szPassword.clear ();

			}
			else if (result == NERR_InvalidComputer)
			{
				ok = 0;
				MessageBox(NULL,
						Utils::LoadResourcesString(11).c_str(),
						Utils::LoadResourcesString(1000).c_str(),
						MB_OK| MB_ICONWARNING);
			}
			else if (result == NERR_NotPrimary)
			{
				ok = 0;
				MessageBox(NULL,
						Utils::LoadResourcesString(12).c_str(),
						Utils::LoadResourcesString(1000).c_str(),
						MB_OK| MB_ICONWARNING);
			}
			else if (result == NERR_UserNotFound)
			{
				ok = 0;
				MessageBox(	NULL,
						Utils::LoadResourcesString(13).c_str(),
						Utils::LoadResourcesString(1000).c_str(),
						MB_OK| MB_ICONWARNING);
			}

			else if (result == NERR_PasswordTooShort)
			{
				ok = 0;
				MessageBox(NULL,
						Utils::LoadResourcesString(14).c_str(),
						Utils::LoadResourcesString(1001).c_str(),
						MB_OK| MB_ICONWARNING);
			}

			else
			{
				std::string msg;
				Utils::getErrorMessage(msg, result);
				char ach[100];
				sprintf (ach, "\n(Error %x)", (int) result);
				msg.append(ach);
				ok = 0;
				m_log.warn(msg.c_str());
				MessageBox(NULL,
						msg.c_str(),
						Utils::LoadResourcesString(1002).c_str(),
						MB_OK| MB_ICONWARNING);
			}
		}
	}

	return true;
}
