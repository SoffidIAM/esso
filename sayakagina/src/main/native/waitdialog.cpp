
# include "sayaka.h"
# include "Log.h"

static const wchar_t *pchMessage;
static HANDLE hMessageThread = NULL;

struct dlgParam
{
		wchar_t *pchMessage;
		HDESK hDesktop;
};

//////////////////////////////////////////////////////////
static BOOL CALLBACK waitDialogProc (HWND hwndDlg,	// handle to dialog box
		UINT uMsg,	// message
		WPARAM wParam,	// first message parameter
		LPARAM lParam 	// second message parameter
		)
{
	return 0;
}

static DWORD WINAPI dlgLoop (LPVOID param)
{
	struct dlgParam *p = (struct dlgParam*) param;	// Dialog params
	MSG msg;																	// Dialog message
	HWND hwnd;																// Dialog handler

	SetThreadDesktop(p->hDesktop);
	pchMessage = p->pchMessage;

	hwnd = CreateDialog (hSayakaDll, MAKEINTRESOURCE (IDD_MESSAGE),
			NULL, waitDialogProc);

	ShowWindow(hwnd, SW_NORMAL);
	SetDlgItemTextW(hwnd, IDC_MESSAGE, p->pchMessage);

	while (pchMessage == p->pchMessage)
	{
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);

		Sleep(500);
	}

	EndDialog(hwnd, 0);

	while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
		DispatchMessage(&msg);

	DestroyWindow(hwnd);

	free(p->pchMessage);
	free(p);

	return 0;
}

void displayMessage (HDESK hDesktopParam, const wchar_t *lpszMessage)
{

	Log log("waitdialog");

	log.info("Preparing message %ls\n", lpszMessage);
	struct dlgParam *p = (struct dlgParam*) malloc(sizeof *p);
	HDESK hDesk = GetThreadDesktop(GetCurrentThreadId());
	wchar_t wchName[512];

	if (GetUserObjectInformationW(hDesk, UOI_NAME, wchName, sizeof wchName, NULL))
	{
		log.info("On desktop %ls\n", wchName);

	}
	if (hDesktopParam == NULL)
		hDesktopParam = GetThreadDesktop(GetCurrentThreadId());
	p->hDesktop = hDesktopParam;
	p->pchMessage = wcsdup(lpszMessage);

	hMessageThread = CreateThread(NULL, 0, dlgLoop, p, 0, NULL);
}

void cancelMessage ()
{
	if (pchMessage != NULL)
	{
		Log log("waitdialog");

		log.info("Closing message %ls\n", pchMessage);
		pchMessage = NULL;
		if (hMessageThread)
			WaitForSingleObject(hMessageThread, INFINITE);
	}
}
