#include "sayaka.h"

#include <resource.h>
#include <wchar.h>
#include "logindialog.h"
#include "Utils.h"
#include "lm.h"
#include "winwlx.h"
#include <windowsx.h>

#include <MZNcompat.h>
#include <string.h>

#include "nossori.h"
#include "ssoclient.h"

extern HWND hwndLogon;
extern HINSTANCE hSayakaDll;

static LoginDialog *loginDialog;


//////////////////////////////////////////////////////////
static BOOL CALLBACK loginDialogProc(
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	Log log("LoginDialogProc");
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			Utils::getDlgCtrlText(hwndDlg, IDC_USER, loginDialog->user);
			Utils::getDlgCtrlText(hwndDlg, IDC_PASSWORD1, loginDialog->password);
			if (! IsDlgButtonChecked (hwndDlg, IDC_ADMIN_CHECK) &&
					loginDialog->domainName.length() > 0)
				loginDialog->domain = loginDialog->domainName;
			else
				loginDialog->domain = loginDialog->hostName;

			WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
			if (wID == IDOK)
			{
				EndDialog (hwndDlg, WLX_SAS_ACTION_LOGON);
			}
			if (wID == IDCANCEL)
			{
				EndDialog (hwndDlg, WLX_SAS_ACTION_NONE);
			}
			if (wID == IDC_SHUTDOWN_BUTTON)
			{
				DWORD dw = shutdownDialog ();
				if (dw >= 0 && dw != WLX_SAS_ACTION_NONE)
					EndDialog (hwndDlg, dw);
			}
			if (wID == IDI_LIFERING)
			{
				ShowWindow(hwndDlg, SW_HIDE);
				NossoriHandler().perform();
				ShowWindow(hwndDlg, SW_SHOW);
			}
			return true;
		}
		case WM_INITDIALOG:
		{
			const char *hostname = MZNC_getHostName ();
			SendMessage (GetDlgItem(hwndDlg, IDC_PASSWORD1), EM_SETPASSWORDCHAR, '*', 0);
			loginDialog->m_hWnd = hwndDlg;

			std::string t = Utils::LoadResourcesString (1);

			t += hostname;
			SetWindowTextA(hwndDlg, t.c_str());

			if (loginDialog->domainName.size() == 0)
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADMIN_CHECK), false);
			else if (loginDialog->domain==loginDialog->hostName)
				CheckDlgButton(hwndDlg, IDC_ADMIN_CHECK, true);
			else
				CheckDlgButton(hwndDlg, IDC_ADMIN_CHECK, false);

			if (loginDialog->user.empty())
			{
				SetFocus (GetDlgItem(hwndDlg, IDC_USER));
			}
			else
			{
				SetDlgItemTextW (hwndDlg, IDC_USER, loginDialog->user.c_str());
				SetFocus (GetDlgItem(hwndDlg, IDC_PASSWORD1));
				ShowWindow(GetDlgItem(hwndDlg, IDC_SHUTDOWN_BUTTON), SW_HIDE);
			}

		    SeyconCommon::updateConfig("addon.retrieve-password.right_number");
		    std::string v;
		    if (!SeyconCommon::readProperty("addon.retrieve-password.right_number", v) ||
		    		v.empty())
		    {
		    	ShowWindow (GetDlgItem(hwndDlg, IDI_LIFERING), SW_HIDE);
		    }
			return false;
		}

		case WM_CLOSE:
		{
			printf ("Received wm_close");
			EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
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

LoginDialog::LoginDialog () {
	m_hWnd = NULL;
}

int LoginDialog::show () {
	Log log ("LoginDialog");

	loginDialog = this;
	int result = pWinLogon->WlxDialogBox (hWlx, hSayakaDll,
			MAKEINTRESOURCEW (IDD_LOGINDIALOG), NULL, loginDialogProc);
	m_hWnd = NULL;

	return result;
}
