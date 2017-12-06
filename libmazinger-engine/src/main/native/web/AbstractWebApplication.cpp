#include <AbstractWebApplication.h>
#include <AbstractWebElement.h>
#include <MazingerInternal.h>

#ifdef WIN32

static char *windowClassName = NULL;
static WORD selectedAction;

static LRESULT CALLBACK MenuWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{
	if (uMsg == WM_MENUSELECT || uMsg == WM_ENTERIDLE)
	{
	}

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


static char* createMenuWndClass ()
{

	if (windowClassName == NULL)
	{
		windowClassName = "MazingerPopupWClass";
		WNDCLASSEX wcx;

		// Fill in the window class structure with parameters
		// that describe the main window.

		wcx.cbSize = sizeof(wcx);          // size of structure
		wcx.style = CS_HREDRAW |
			CS_VREDRAW;                    // redraw if size changes
		wcx.lpfnWndProc = MenuWndProc;     // points to window procedure
		wcx.cbClsExtra = 0;                // no extra class memory
		wcx.cbWndExtra = 0;                // no extra window memory
		wcx.hInstance = GetModuleHandleA(NULL);         // handle to instance
		wcx.hIcon = LoadIcon(NULL, IDI_INFORMATION);              // predefined app. icon
		wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow
		wcx.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);                  // white background brush
		wcx.lpszMenuName =  NULL;    // name of menu resource
		wcx.lpszClassName = windowClassName;  // name of window class
		wcx.hIconSm = NULL;
		// Register the window class.

		RegisterClassEx(&wcx);
	}
	return windowClassName;
}

#endif


#ifdef WIN32
static HWND hwnd = NULL;
#endif

void AbstractWebApplication::selectAction (const char * title,
		std::vector<std::string> &optionId,
		std::vector<std::string> &names,
		AbstractWebElement *element,
		WebListener *listener)
{
#ifdef WIN32
	if (hwnd == NULL)
		hwnd = CreateWindowA(createMenuWndClass(), "TITLE", WS_POPUP, 0,0, 1, 1, NULL, NULL, GetModuleHandle(NULL), (LPVOID) NULL);
	HMENU hm = CreatePopupMenu();

	AppendMenuA (hm, MF_STRING|MF_DISABLED, -1, title);
	AppendMenuA (hm, MF_SEPARATOR, -1, "");

	for (int i = 0; i < optionId.size() && i < names.size(); i++)
	{
		AppendMenuA (hm, MF_STRING|MF_ENABLED, i+1000, MZNC_utf8tostr(names[i].c_str()).c_str());
	}
	AppendMenuA (hm, MF_STRING|MF_ENABLED, 1, "Cancel");

	selectedAction = -1;
	POINT pt;
	GetCursorPos(&pt);
#if 0
	do {
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		consumeMessages();
		Sleep(500);
	} while (GetForegroundWindow() != hwnd);

	consumeMessages();
#endif
	DWORD now = GetCurrentTime();
	selectedAction = TrackPopupMenu(hm, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0,
			hwnd, NULL);
	DWORD then = GetCurrentTime();
	if (then - now < 200 /*ms*/) {
		selectedAction = TrackPopupMenu(hm, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0,
				hwnd, NULL);
	}

//	CloseWindow(hwnd);
	//DestroyWindow(hwnd);
	DestroyMenu(hm);

//	consumeMessages();

	if (selectedAction >= 1000 && selectedAction < 1000+optionId.size())
	{
		listener->onEvent("selectAction", this, element, optionId[selectedAction-1000].c_str());
	}

#endif
}
