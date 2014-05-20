#include "sayaka.h"

#include "WaitDialog.h"
#include "resource.h"
#include "Log.h"

//////////////////////////////////////////////////////////
static INT_PTR CALLBACK waitDialogProc (HWND hwndDlg,	// handle to dialog box
		UINT uMsg,	// message
		WPARAM wParam,	// first message parameter
		LPARAM lParam 	// second message parameter
		)
{
	return 0;
}

const wchar_t *pchMessage;

int iOrder = 0;

static DWORD __stdcall dlgLoop (LPVOID param)
{
	WaitDialog *d = (WaitDialog *) param;		// Dialog param
	LPCWSTR pchMessage2 = pchMessage;	// Dialog message
	MSG msg;													// Dialog message
	HWND hwnd;												// Dialog handler

	Log log("WaitDialog");

	hwnd = CreateDialog (hSayakaInstance, MAKEINTRESOURCE (IDD_MESSAGE),
			d->getParentWindow(), waitDialogProc);

	ShowWindow(hwnd, SW_NORMAL);
	SetDlgItemTextW(hwnd, IDC_MESSAGE, pchMessage);

	log.info(L"Created window with message %s", pchMessage);

	while (pchMessage == pchMessage2)
	{
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);

		Sleep(500);
	}

	log.info(L"End message dialog");

	EndDialog(hwnd, 0);

	while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
		DispatchMessage(&msg);

	DestroyWindow(hwnd);

	return 0;
}

WaitDialog::WaitDialog (HWND hwnd)
{
	m_hwndParent = hwnd;
}

WaitDialog::~WaitDialog ()
{
	cancelMessage();
}

void WaitDialog::displayMessage (const wchar_t *lpszMessage)
{
	Log log("WaitDialog");
	log.info(L"Trying to show message %s", lpszMessage);
	m_szMessage.assign(lpszMessage);
	pchMessage = m_szMessage.c_str();

	CreateThread(NULL, 0, dlgLoop, this, 0, NULL);
}

void WaitDialog::cancelMessage ()
{
	pchMessage = NULL;
}
