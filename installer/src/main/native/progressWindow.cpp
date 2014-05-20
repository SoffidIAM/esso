#include <windows.h>

#include <stdarg.h>
#include <string>
#include <stdio.h>
# include "resource.h"

static std::string progressMessage;
static HANDLE hProgressThread;
static HWND hwndProgressMsg;
static HWND hwndProgress;


static LRESULT CALLBACK ProgressWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
    return 0;
}


static BOOL createProgressWndClass ()
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with parameters
    // that describe the main window.

    wcx.cbSize = sizeof(wcx);          // size of structure
    wcx.style = CS_HREDRAW |
        CS_VREDRAW;                    // redraw if size changes
    wcx.lpfnWndProc = ProgressWndProc;     // points to window procedure
    wcx.cbClsExtra = 0;                // no extra class memory
    wcx.cbWndExtra = 0;                // no extra window memory
    wcx.hInstance = GetModuleHandleA(NULL);         // handle to instance
    wcx.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_SOFFID_ICON));              // predefined app. icon
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow
    wcx.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);                  // white background brush
    wcx.lpszMenuName =  NULL;    // name of menu resource
    wcx.lpszClassName = "MazingerInstallerProgressWClass";  // name of window class
    wcx.hIconSm = NULL;
    // Register the window class.

    return RegisterClassEx(&wcx);
}


static DWORD WINAPI progressMessageLoop (LPVOID param) {

    DWORD units = GetDialogBaseUnits();
    DWORD xunit = LOWORD(units);
    DWORD yunit = HIWORD(units);

    DWORD x = GetSystemMetrics(SM_CXFULLSCREEN);
    DWORD y = GetSystemMetrics(SM_CYFULLSCREEN);
    DWORD cx = xunit * 50;
    DWORD cy = yunit * 7;
    x = ( x - cx ) / 2;
    y = ( y - cy ) / 2;
    // Create the main window.

    HINSTANCE hInstance = GetModuleHandleA(NULL);

    createProgressWndClass ();

	hwndProgress = CreateWindow(
        "MazingerInstallerProgressWClass",        // name of window class
        "Progress",            // title-bar string
        WS_POPUP|WS_CAPTION, // top-level window
        x,       // default horizontal position
        y,       // default vertical position
        cx,       // default width
        cy,       // default height
        (HWND) NULL,         // no owner window
        (HMENU) NULL,        // use class menu
        hInstance,           // handle to application instance
        (LPVOID) NULL);      // no window-creation data


	SetWindowTextA (hwndProgress, "Soffid ESSO Installation");

	CreateWindow ("STATIC", "",
    		WS_CHILD|WS_VISIBLE,
    		0, 0,
    		cx, cy,
    		hwndProgress,
    		NULL,
    		hInstance,
    		NULL);



    hwndProgressMsg = CreateWindow ("STATIC", progressMessage.c_str(),
    		WS_CHILD|SS_CENTER|SS_NOPREFIX|WS_VISIBLE,
    		0, yunit*2,
    		cx, yunit,
    		hwndProgress,
    		NULL,
    		hInstance,
    		NULL);

    // Show the window and send a WM_PAINT message to the window
    // procedure.


	ShowWindow(hwndProgress, SW_SHOWNORMAL);
    UpdateWindow(hwndProgress);


	MSG msg;
	while ( GetMessageA(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	if (hProgressThread == GetCurrentThread())
		hProgressThread = NULL;

	return NULL;

}


void disableProgressWindow () {
	hwndProgressMsg = NULL;
	if (hProgressThread != NULL) {
		hProgressThread = NULL;
	}
	if (hwndProgress != NULL) {
		ShowWindow(hwndProgress, SW_HIDE);
		DestroyWindow(hwndProgress);
		hwndProgress = NULL;
	}

}

void setProgressMessage (const char *lpszMessage, ...) {
	char achMessage[2048];

	va_list v;
	va_start(v, lpszMessage);
	vsprintf(achMessage, lpszMessage, v);
	va_end(v);

	progressMessage = achMessage;
	if (hProgressThread == NULL) {
		hProgressThread = CreateThread(NULL, 0, progressMessageLoop, NULL, 0, NULL);
		Sleep(500);
	}
	if (hwndProgressMsg != NULL) {
		SetWindowTextA(hwndProgressMsg, achMessage);
	}
}


