#include <windows.h>
#include <resource.h>
#include <ssoclient.h>
# include "Utils.h"
#include "KojiKabuto.h"

HWND hwnd;


LRESULT CALLBACK KojiWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{

    switch (uMsg)
    {
    case WM_ENDSESSION:
    	SeyconCommon::info("Shutting down");
    	if (KojiKabuto::session != NULL)
    		KojiKabuto::session->close();

/*        case WM_CREATE:
            // Initialize the window.
            return 0;

        case WM_PAINT:
            // Paint the window's client area.
            return 0;

        case WM_SIZE:
            // Set the size and position of the window.
            return 0;

        case WM_DESTROY:
            // Clean up window-specific data objects.
            return 0;
*/
        //
        // Process other messages.
        //

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


BOOL createKojiWndClass (HINSTANCE hinstance)
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with parameters
    // that describe the main window.

    wcx.cbSize = sizeof(wcx);          // size of structure
    wcx.style = CS_HREDRAW |
        CS_VREDRAW;                    // redraw if size changes
    wcx.lpfnWndProc = KojiWndProc;     // points to window procedure
    wcx.cbClsExtra = 0;                // no extra class memory
    wcx.cbWndExtra = 0;                // no extra window memory
    wcx.hInstance = hinstance;         // handle to instance
    wcx.hIcon = LoadIcon(NULL,(LPCSTR)IDI_ICON2);              // predefined app. icon
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow
    wcx.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);                  // white background brush
    wcx.lpszMenuName =  NULL;    // name of menu resource
    wcx.lpszClassName = "KojiProgresWClass";  // name of window class
    wcx.hIconSm = NULL;
    /*LoadImage(hinstance, // small class icon
        MAKEINTRESOURCE(5),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
*/
    // Register the window class.

    return RegisterClassEx(&wcx);
}


HWND createKojiWindow(HINSTANCE hInstance)
{


    createKojiWndClass(hInstance);

    DWORD units = GetDialogBaseUnits();
    DWORD xunit = LOWORD(units);
    DWORD yunit = HIWORD(units);

    DWORD x = GetSystemMetrics(SM_CXFULLSCREEN);
    DWORD y = GetSystemMetrics(SM_CYFULLSCREEN);
    DWORD cx = xunit * 50;
    DWORD cy = yunit * 5;
    x = ( x - cx ) / 2;
    y = ( y - cy ) / 2;
    // Create the main window.

    hwnd = CreateWindow(
        "KojiProgresWClass",        // name of window class
        Utils::LoadResourcesString(22).c_str(),            // title-bar string
        WS_POPUP, // top-level window
        x,       // default horizontal position
        y,       // default vertical position
        cx,       // default width
        cy,       // default height
        (HWND) NULL,         // no owner window
        (HMENU) NULL,        // use class menu
        hInstance,           // handle to application instance
        (LPVOID) NULL);      // no window-creation data

    CreateWindow ("STATIC", "",
    		WS_CHILD|WS_VISIBLE,
    		0, 0,
    		cx, cy,
    		hwnd,
    		NULL,
    		hInstance,
    		NULL);


    HWND hWndStatic = CreateWindow ("STATIC", "",
    		WS_CHILD|SS_CENTER|SS_NOPREFIX|WS_VISIBLE,
    		0, yunit*2,
    		cx, yunit,
    		hwnd,
    		NULL,
    		hInstance,
    		NULL);

    SetWindowLongA(hWndStatic, GWL_ID, IDC_TEXT);


    if (!hwnd)
        return FALSE;

    // Show the window and send a WM_PAINT message to the window
    // procedure.

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

//	CreateThread(NULL, 0, dlgLoop, NULL, 0, NULL);

    return hwnd;

}
