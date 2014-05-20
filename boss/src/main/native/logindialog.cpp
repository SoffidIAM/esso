#include "boss.h"
#include <wchar.h>
#include "logindialog.h"
#include "Utils.h"

#include <MZNcompat.h>
#include <string.h>
#include <stdio.h>

//////////////////////////////////////////////////////////
static BOOL CALLBACK loginDialogProc (HWND hwndDlg,	// handle to dialog box
		UINT uMsg,	// message
		WPARAM wParam,	// first message parameter
		LPARAM lParam 	// second message parameter
		)
{
	LoginDialog *loginDialog = (LoginDialog*) GetWindowLongA(hwndDlg, GWL_USERDATA);
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			Utils::getDlgCtrlText(hwndDlg, IDC_USER, loginDialog->user);
			Utils::getDlgCtrlText(hwndDlg, IDC_PASSWORD, loginDialog->password);

			WORD wID = LOWORD(wParam);        // item, control, or accelerator identifier
			if (wID == IDOK)
			{
				EndDialog(hwndDlg, IDOK);
			}
			if (wID == IDCANCEL)
			{
				EndDialog(hwndDlg, IDCANCEL);
			}
			return true;
		}
		case WM_INITDIALOG:
		{
			loginDialog = (LoginDialog*) lParam;
			SetWindowLongA(hwndDlg, GWL_USERDATA, (LPARAM) loginDialog);
			const char *hostname = MZNC_getHostName();
			SendMessage(GetDlgItem(hwndDlg, IDC_PASSWORD), EM_SETPASSWORDCHAR, '*', 0);
			loginDialog->m_hWnd = hwndDlg;

			std::string t = Utils::LoadResourcesString(2);
			t += hostname;
			SetWindowTextA(hwndDlg, t.c_str());

			SetDlgItemTextW(hwndDlg, IDC_PROGRAM, loginDialog->program.c_str());

			if (loginDialog->user.empty())
			{
				SetFocus(GetDlgItem(hwndDlg, IDC_USER));
			}
			else
			{
				SetDlgItemTextW(hwndDlg, IDC_USER, loginDialog->user.c_str());
				SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD));
			}
			return false;

		}
		case WM_CLOSE:
		{
			EndDialog(hwndDlg, IDCANCEL);
			return true;
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

LoginDialog::LoginDialog ()
{
	m_hWnd = NULL;
}

int LoginDialog::show ()
{
	int result = DialogBoxParamA(g_hInstance, MAKEINTRESOURCE (IDD_LOGINDIALOG), NULL,
			loginDialogProc, (LPARAM) this);
	m_hWnd = NULL;
	printf("Result dlg = %d (hinstance = %d)\n", result, g_hInstance);

	return result;
}
