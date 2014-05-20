
#include "sayaka.h"
#include "winwlx.h"
#include "ssoclient.h"

# include "Utils.h"



//////////////////////////////////////////////////////////
static BOOL CALLBACK shutdownDialogProc(
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
		if (lParam != 0) {
			WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
			if (wID == IDC_OK)
			{
				HWND hwndList = GetDlgItem (hwndDlg, IDC_COMBO1);
				int selected = SendMessage(hwndList, CB_GETCURSEL, NULL, NULL);
				if (selected == CB_ERR)
				{
					MessageBox (hwndDlg, Utils::LoadResourcesString(1003).c_str(),
							Utils::LoadResourcesString(39).c_str(),
							MB_OK|MB_ICONEXCLAMATION);
				}
				else
				{
					switch ( selected )
					{
					case 0:
						EndDialog (hwndDlg, WLX_SAS_ACTION_SHUTDOWN);
						break;
					case 1:
						EndDialog (hwndDlg, WLX_SAS_ACTION_SHUTDOWN_REBOOT);
						break;
					case 2:
						EndDialog (hwndDlg, WLX_SAS_ACTION_SHUTDOWN_SLEEP);
						break;
					case 3:
						EndDialog (hwndDlg, WLX_SAS_ACTION_SHUTDOWN_HIBERNATE);
						break;
					}
				}
			}
			if (wID == IDC_CANCEL)
			{
				EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
			}
		}
		else  if (LOWORD(wParam) == IDCANCEL){
			EndDialog(hwndDlg, WLX_SAS_ACTION_NONE);
		}

		return 0;
	}
	case WM_INITDIALOG:
	{
		HWND hwndList = GetDlgItem (hwndDlg, IDC_COMBO1);

		SendMessage(hwndList, CB_ADDSTRING, 0,
				(LPARAM) Utils::LoadResourcesString(1004).c_str());
		SendMessage(hwndList, CB_ADDSTRING, 0,
				(LPARAM) Utils::LoadResourcesString(1005).c_str());
		SendMessage(hwndList, CB_ADDSTRING, 0,
				(LPARAM) Utils::LoadResourcesString(1006).c_str());
		SendMessage(hwndList, CB_ADDSTRING, 0,
				(LPARAM) Utils::LoadResourcesString(1007).c_str());
		SendMessage(hwndList, CB_SETCURSEL, 0, (LPARAM) NULL);

		return 0;
	}

	default:
		return 0;
	}
}

int shutdownDialog () {
	return pWinLogon->WlxDialogBox (hWlx, hSayakaDll,
			MAKEINTRESOURCEW (IDD_SHUTDOWN), NULL, shutdownDialogProc);
}
