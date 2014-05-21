#include "sayaka.h"
#include <resource.h>
#include <wchar.h>
#include "winwlx.h"
# include "Utils.h"

extern HWND hwndLogon;

static char achPin[512];
extern HINSTANCE hSayakaDll;

//////////////////////////////////////////////////////////
BOOL CALLBACK pinDialogProc(
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
		if (wID == IDOK)
		{
			char ach[100];
			ach[0] = L'\0';
			GetDlgItemTextA (hwndDlg, IDC_PIN, ach, sizeof ach - 1);
			if (strlen (ach) > 0)
			{
				strcpy (achPin, ach);
				EndDialog (hwndDlg, 0);
			}
			else
			{
				MessageBox (hwndDlg, Utils::LoadResourcesString(1003).c_str(),
						Utils::LoadResourcesString(40).c_str(),
						MB_OK|MB_ICONEXCLAMATION);
			}
		}
		if (wID == IDCANCEL) {
			EndDialog (hwndDlg, 1);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		SendMessage (GetDlgItem(hwndDlg, IDC_PIN), EM_SETPASSWORDCHAR, '*', 0);
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



LPCSTR enterPin()
{
	int result = pWinLogon->WlxDialogBox (hWlx, hSayakaDll,
			MAKEINTRESOURCEW (IDD_ENTERPIN), NULL, pinDialogProc);
	if (result == 0)
	{
		return achPin;
	}
	else
		return NULL;
}

void clearPin()
{
	memset (achPin, 0, sizeof achPin);
}
