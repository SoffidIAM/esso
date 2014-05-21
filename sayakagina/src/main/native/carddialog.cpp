#include <windows.h>
#include <resource.h>
#include <wchar.h>
#include <vector>
#include "CertificateHandler.h"
#include "sayaka.h"
#include "winwlx.h"
# include "Utils.h"

extern HWND hwndLogon;
extern HINSTANCE hSayakaDll;

static int iSelectedIndex = -1;
static std::vector<CertificateHandler*> *s_certs;

//////////////////////////////////////////////////////////
BOOL CALLBACK selectCertDialogProc(
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
			HWND hwndList = GetDlgItem (hwndDlg, IDC_LIST1);
			int selected = SendMessage(hwndList, LB_GETCURSEL, NULL, NULL);
			if (selected == LB_ERR)
			{
				MessageBox (hwndDlg, Utils::LoadResourcesString(1003).c_str(),
						Utils::LoadResourcesString(23).c_str(),
						MB_OK|MB_ICONEXCLAMATION);
			}
			else
			{
				iSelectedIndex = selected;
				EndDialog (hwndDlg, 0);
			}
		}
		if (wID == IDCANCEL) {
			iSelectedIndex = -1;
			EndDialog (hwndDlg, 1);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		HWND hwndList = GetDlgItem (hwndDlg, IDC_LIST1);

		for (std::vector<CertificateHandler*>::iterator it = s_certs->begin();
				it != s_certs->end();
				it ++)
		{
			CertificateHandler *cert = *it;
			SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)cert->getName());
		}
		SendMessage(hwndList, LB_SETCURSEL, 0, NULL);

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



int selectCert (std::vector<CertificateHandler*> &certs)
{
	s_certs = &certs;
	int result = pWinLogon->WlxDialogBox (hWlx, hSayakaDll, MAKEINTRESOURCEW (IDD_SELCERT), NULL, selectCertDialogProc);
	if (result == 0)
	{
		return iSelectedIndex;
	}
	else
		return -1;
}
