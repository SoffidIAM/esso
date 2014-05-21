#include <windows.h>
#include <resource.h>
#include <wchar.h>
#include <string.h>
#include <ssoclient.h>
#include "KojiDialog.h"
#include <MZNcompat.h>
#include <stdio.h>

# include "Utils.h"
# include "KojiKabuto.h"

wchar_t achCardValue[100];
static std::wstring strPassValue;
static std::wstring reasonToDisplay;
static std::wstring lpszTargeta;
static std::wstring lpszCella;

extern HWND hwndLogon;

HWND hwndDialog = NULL;

//////////////////////////////////////////////////////////
INT_PTR CALLBACK cardDialogProc(
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
//		WORD wNotifyCode = HIWORD(wParam); // notification code
		WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
//		HWND hwndCtl = (HWND) lParam;      // handle of control
		if (wID == IDOK)
		{
			wchar_t ach[100];
			ach[0] = L'\0';
			GetDlgItemTextW (hwndDlg, IDC_VALOR, ach, sizeof ach - 1);
			if (wcslen (ach) == 2) wcscpy (achCardValue, ach);
			hwndDialog = NULL;
			EndDialog (hwndDlg, 0);
		}
		if (wID == IDCANCEL) {
			hwndDialog = NULL;
			EndDialog (hwndDlg, 1);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		SetDlgItemTextW (hwndDlg, IDC_TARGETA, lpszTargeta.c_str());
		SetDlgItemTextW (hwndDlg, IDC_CELLA, lpszCella.c_str());
		SetActiveWindow (hwndDlg);
		SetFocus (GetDlgItem (hwndDlg, IDC_VALOR));
		hwndDialog = hwndDlg;
		return 0;
	}
	case WM_NEXTDLGCTL:
	{
		ShowWindow(hwndDlg, SW_SHOWNORMAL);
		return 0;
	}
	default:
		return 0;
	}
}


//////////////////////////////////////////////////////////
DWORD CALLBACK cancelDialogThread (LPVOID param) {
	HWND hwnd = (HWND) param;
	char ach[20];
	for (int i = 30; i > 0; i--) {
		sprintf (ach, "%d", i);
		SetDlgItemTextA(hwnd, IDC_CONTADOR, ach);
		Sleep(1000);
		if (!IsWindowVisible(hwnd)) {
			return 0;
		}
	}
	EndDialog (hwnd, IDOK);
	return 0;
}


INT_PTR CALLBACK remoteLogouDialogProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
		if (wID == IDOK || wID == IDCANCEL)
		{
			hwndDialog = NULL;
			EndDialog (hwndDlg, wID);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		hwndDialog = hwndDlg;
		CreateThread(NULL, 0, cancelDialogThread, (LPVOID) hwndDlg, 0, NULL);
		return 0;
	}
	case WM_NEXTDLGCTL:
	{
		ShowWindow(hwndDlg, SW_SHOWNORMAL);
		return 0;
	}
	default:
		return 0;
	}
}


//////////////////////////////////////////////////////////


INT_PTR CALLBACK duplicateSessionDialogProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	switch (uMsg)
	{
	case WM_COMMAND:
	{
		WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
		if (wID == IDC_CLOSE_REMOTE || wID == IDC_WAITBUTTON || wID == IDCANCEL)
		{
			EndDialog (hwndDlg, wID);
			hwndDialog = NULL;
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		hwndDialog = hwndDlg;
		return 0;
	}
	case WM_NEXTDLGCTL:
	{
		ShowWindow(hwndDlg, SW_SHOWNORMAL);
		return 0;
	}
	default:
		return 0;
	}
}

/////////////////////////////////////////////////////////////////////
/** @brief Check password
 *
 * Implements the functionality to check a correct password and the confirmation.
 * @param newPass New password to check.
 * @param repeatPass New password confirmation to check.
 * @return
 * <ul>
 * 		<li>
 * 			@code TRUE @endcode
 * 			If new password and the confirmation are equals.
 * 		</li>
 * 		<li>
 * 			@code TRUE @endcode
 * 			If new password and the confirmation are not equals or are void.
 * 		</li>
 * </ul>
 */
bool CheckNewPassword(std::wstring newPass, std::wstring repeatPass)
{
	bool passOK = true;	// Check password status

	// Check void passwords
	if ((newPass.size() == 0) || (repeatPass.size() == 0))
	{
		MessageBox(NULL, Utils::LoadResourcesString(23).c_str(),
				Utils::LoadResourcesString(1000).c_str(), MB_OK | MB_ICONERROR);

		passOK = false;
	}

	else
	{
		// Check equals passwords
		if (newPass != repeatPass)
		{
			MessageBox(NULL, Utils::LoadResourcesString(24).c_str(),
					Utils::LoadResourcesString(1000).c_str(), MB_OK | MB_ICONERROR);

			passOK = false;
		}
	}

	return passOK;
}

/** @brief Ask new password dialog
 *
 * Implements the functionality to ask new password and check if correctly.
 * @param hwndDlg	Handle to dialog box.
 * @param uMsg			Dialog message.
 * @param wParam		First message parameter.
 * @param lParam		Second message parameter
 */
INT_PTR CALLBACK askNewPasswordDialogProc(HWND hwndDlg,	UINT uMsg,
		WPARAM wParam, LPARAM lParam)
{
	WORD wID = LOWORD(wParam);	// item, control, or accelerator identifier
	std::wstring newPass;						// New password
	std::wstring repeatPass;					// Repeated new password
	bool passOK = false;					// Check password status

	switch (uMsg)
	{
		case WM_INITDIALOG:	// Start dialog

			SetDlgItemTextW (hwndDlg, IDC_REASON, reasonToDisplay.c_str());

			SetFocus (GetDlgItem(hwndDlg, IDC_PASSWORD));

			return false;
			break;

		case WM_COMMAND:
		{
			// Check 'OK' button pushed
			if (wID == IDOK)
			{
				Utils::getDlgCtrlText(hwndDlg, IDC_PASSWORD, newPass);
				Utils::getDlgCtrlText(hwndDlg, IDC_REPEATPASSWORD, repeatPass);

				passOK = CheckNewPassword(newPass, repeatPass);

				strPassValue = newPass;

				EndDialog (hwndDlg, !passOK);
			}

			// Check 'Cancel' button pushed
			if (wID == IDCANCEL)
			{
				MessageBox(NULL, Utils::LoadResourcesString(25).c_str(),
						Utils::LoadResourcesString(1000).c_str(), MB_OK | MB_ICONINFORMATION);

				EndDialog(hwndDlg, -1);
			}

			return true;

			break;
		}

		default:
			return 0;
	}
}

DWORD CALLBACK raiseLoop (LPVOID param)
{
	while (hwndDialog == NULL)
		Sleep (1000);

	while (hwndDialog != NULL)
	{
		ShowWindow(hwndDialog, SW_SHOWNORMAL);
		Sleep (1000);
	}

	return 0;
}

bool KojiDialog::askNewPassword (const char* reason, std::wstring &password)
{
	int changePassResult = -1;	// Change password result
	bool changeOK = false;		// Change password status

	CreateThread(NULL, 0, raiseLoop, NULL, 0, NULL);

	reasonToDisplay = MZNC_strtowstr(reason);

	do
	{
		changePassResult = DialogBox(NULL, MAKEINTRESOURCE (IDD_NEWPASS_DIALOG),
				NULL, askNewPasswordDialogProc);

		// Check change password dialog result
		if (changePassResult == 0)
		{
			password.assign(strPassValue);

			changeOK = true;

//			MessageBox(NULL, Utils::LoadResourcesString(26).c_str(),
//					Utils::LoadResourcesString(1000).c_str(), MB_OK | MB_ICONINFORMATION);
		}
	} while ((!changeOK) && (changePassResult != -1));

	hwndDialog = NULL;

	return changeOK;
}

bool KojiDialog::askCard (const char* targeta, const char* cella,
		std::wstring &result)
{
	lpszTargeta = MZNC_strtowstr(targeta);
	lpszCella = MZNC_strtowstr(cella);
	if (strlen (targeta) > 0)
	{
		hwndDialog = NULL;
		CreateThread(NULL, 0, raiseLoop, NULL, 0, NULL);
		int status = DialogBox (NULL, MAKEINTRESOURCE (IDD_CARD),
				NULL, cardDialogProc);
		hwndDialog = NULL;
		if (status == 0)
		{
			result.assign (achCardValue);
			return true;
		}
	}
	return false;
}


DuplicateSessionAction KojiDialog::askDuplicateSession (const char *details) {
	hwndDialog = NULL;
	CreateThread(NULL, 0, raiseLoop, NULL, 0, NULL);
	int result =  DialogBox(NULL, MAKEINTRESOURCE(IDD_TOOMANYSESSIONS),
			NULL, duplicateSessionDialogProc);
	hwndDialog = NULL;
	switch (result) {
	case IDC_WAITBUTTON:
		return dsaWait;
	case IDC_CLOSE_REMOTE:
		return dsaCloseOther;
	default:
		return dsaCancel;
	}
}

bool KojiDialog::askAllowRemoteLogout ()  {
	hwndDialog = NULL;
	CreateThread(NULL, 0, raiseLoop, NULL, 0, NULL);
	int result =  DialogBox(NULL, MAKEINTRESOURCE(IDD_CLOSEDIALOG), NULL,
			remoteLogouDialogProc);
	hwndDialog = NULL;
	if (result == IDOK) {
		ExitWindowsEx(EWX_LOGOFF|EWX_FORCE, 0);
	}
	return result == IDOK;
}

void KojiDialog::notify (const char *message) {
	MessageBoxA (NULL, message,
			Utils::LoadResourcesString(1000).c_str(),
			MB_OK|MB_ICONEXCLAMATION);
}

void KojiDialog::progressMessage (const char *message) {
	KojiKabuto::displayProgressMessage(message);
}
