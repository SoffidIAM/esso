#include <windows.h>
#include <resource.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>
# include <MZNcompat.h>

# include "Utils.h"

extern HWND hwndLogon;
extern HINSTANCE hKojiInstance;

static HWND hwndDialog = NULL;
static wchar_t szAnswer[200];

static const wchar_t *szQuestion;
static const wchar_t *szBitmap;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT_PTR CALLBACK credits1DialogProc(
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
			szAnswer[0] = L'\0';
			GetDlgItemTextW (hwndDlg, IDC_TEXT, szAnswer, sizeof szAnswer - 1);
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
		HBITMAP hBitmap = (HBITMAP) LoadImageW (hKojiInstance, szBitmap,
				IMAGE_BITMAP, 0, 0, 0);
		HWND wndstatic = GetDlgItem(hwndDlg, IDC_BITMAP);
		SendMessage (wndstatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hBitmap);
		SetDlgItemTextW(hwndDlg, IDC_QUESTIO, szQuestion);
	}
	case WM_NEXTDLGCTL:
	{
		return 0;
	}
	default:
		return 0;
	}
}

//////////////////////////////////////////////////////////
static INT_PTR CALLBACK credits2DialogProc(
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
			EndDialog (hwndDlg, 0);
		}
		if (wID == IDCANCEL) {
			EndDialog (hwndDlg, 1);
		}
		return 0;
	}
	case WM_INITDIALOG:
	{
		HBITMAP hBitmap = (HBITMAP) LoadImage (hKojiInstance,
				Utils::LoadResourcesString(1).c_str(), IMAGE_BITMAP, 0, 0, 0);
		HWND wndstatic = GetDlgItem(hwndDlg, IDC_BITMAP);
		SendMessage (wndstatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hBitmap);
	}
	case WM_NEXTDLGCTL:
	{
		return 0;
	}
	default:
		return 0;
	}
}

void creditsDialog1 ()
{
	int i = rand () % 3;
	switch (i) {
	case 0:
		szBitmap = MZNC_strtowstr(Utils::LoadResourcesString(2).c_str()).c_str();
		szQuestion = MZNC_strtowstr(Utils::LoadResourcesString(3).c_str()).c_str();
		break;
	case 1:
		szBitmap = MZNC_strtowstr(Utils::LoadResourcesString(4).c_str()).c_str();
		szQuestion = MZNC_strtowstr(Utils::LoadResourcesString(5).c_str()).c_str();
		break;
	case 2:
		szBitmap = MZNC_strtowstr(Utils::LoadResourcesString(6).c_str()).c_str();
		szQuestion = MZNC_strtowstr(Utils::LoadResourcesString(7).c_str()).c_str();
		break;
	}
	int result = DialogBox (hKojiInstance, MAKEINTRESOURCE (IDD_CREDITS1),
			NULL, credits1DialogProc);
	if (result == 0) {
		bool ok = false;
		for (int j = 0; szAnswer[j]; j++)
			szAnswer[j] = towlower (szAnswer[j]);
		switch (i) {
		case 0:
			if (wcsstr(szAnswer, L"doppler") != NULL ||
					wcsstr (szAnswer, L"d\u00f2ppler")!= NULL ||
					wcsstr (szAnswer, L"d\u00f3ppler")!= NULL)
				ok = true;
			break;
		case 1:
			if (wcsstr(szAnswer, L"rafael") != NULL ||
				wcsstr (szAnswer, L"raphael") != NULL ||
				wcsstr(szAnswer, L"garrido") != NULL )
				ok = true;
			break;
		case 2:
			if (wcsstr(szAnswer, L"cp/m") != NULL)
				ok = true;
			break;
		}
		if (ok)
			DialogBox (hKojiInstance, MAKEINTRESOURCE (IDD_CREDITS2),
					NULL, credits2DialogProc);
		else
			MessageBox ( NULL,
					Utils::LoadResourcesString(8).c_str(),
					Utils::LoadResourcesString(1000).c_str(),
					MB_OK|MB_ICONEXCLAMATION);
	}
}
