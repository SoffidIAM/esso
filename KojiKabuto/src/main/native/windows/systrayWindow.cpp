#include <windows.h>
#include <windowsx.h>
#include <resource.h>
#include <ssoclient.h>
#include <MazingerHook.h>
#include <stdio.h>
#include <MZNcompat.h>
#include <ssoclient.h>
#include <MazingerEnv.h>
#include <SecretStore.h>
#include <winsock.h>
#include <userenv.h>

#include <MazingerInternal.h>

# include "KojiKabuto.h"
# include "Utils.h"
# include <stdio.h>

#define WM_APP_SYSTRAY (WM_APP + 1)

#define INTERNAL_WND_PREFIX "$$KojiKabutoUser$$ "

static HMENU _hMenu;
static HINSTANCE _hInstance;
static HWND _hwnd;
static int defaultMenuEntries = 0;

bool sessionStarted;	// Session started status
bool startingSession;	// Session is starting

void creditsDialog1 ();

static void displayError(const char *header, DWORD error) {
	char* buffer;
	char ach[1000];
	if (FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			error, 0, (LPSTR) & buffer, 0, NULL) > 0) {
		sprintf (ach, "%s. Error %d: %s", header, error, buffer);
	} else {
		sprintf (ach, "%s. Error %d", header, error);
	}
	MessageBox (NULL, ach, "Soffid ESSO", MB_OK);
}

static void displayError(const char *header) {
	displayError(header, GetLastError());
}

static void playSound ()
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
			0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		static char achPath[4096] = "XXXX";
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		size = sizeof achPath;
		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType, (LPBYTE) achPath, &size);
		RegCloseKey(hKey);
		strcat(achPath, "\\SoffidESSO\\planeador.wav");
//		PlaySoundA(achPath, NULL, SND_FILENAME);
	}
}

struct EnumDesktopAction
{
	bool updateMenu ;
	bool execute ;
	bool openMenu ;
	bool forwardAction;
	bool highlight;
	HMENU hm;
	int nextId;
	int idToExeucte;
	HDESK currentDesktop;

	EnumDesktopAction ()
	{
		updateMenu = false;
		execute = false;
		openMenu = false;
		forwardAction = false;
		highlight = false;
		idToExeucte = 0;
		nextId = 0;
		hm = NULL;
		currentDesktop = NULL;
	}
};

static BOOL CALLBACK enumWindowProc(
  HWND hwnd,
  LPARAM lParam
)
{
	EnumDesktopAction *action = (EnumDesktopAction*) lParam;
	char achText[1000] = "";
	GetWindowText (hwnd, achText, sizeof achText);

	int prefixLen = strlen (INTERNAL_WND_PREFIX);
	if (strncmp (achText, INTERNAL_WND_PREFIX, prefixLen ) == 0)
	{
		if ( action->forwardAction)
		{
			PostMessageA(hwnd, WM_COMMAND, IDM_USER_NEW_DESKTOP, 0L);
		}
		if ( action->updateMenu )
		{
			char hotkey[10];
			if (action->nextId == IDM_USER_DESKTOP)
				AppendMenuW (action->hm, MF_SEPARATOR, -1, L"");
			sprintf(hotkey, "&%d.", (action->nextId - IDM_USER_DESKTOP + 1));
			std::string s = hotkey;
			s +=  &achText[prefixLen];
			DWORD style = action->highlight ? MF_STRING|MF_HILITE : MF_STRING;
			AppendMenuA (action->hm, style , action->nextId, s.c_str());
		}
		if ( action->openMenu )
		{
			PostMessageA(hwnd, WM_COMMAND, IDM_OPEN_MENU, 0L);
		}
		if (action-> execute && action->nextId == action->idToExeucte)
		{
			SwitchDesktop(action->currentDesktop);
		}
		action -> nextId ++;
		return 0;
	}


	return 1;
}


static BOOL CALLBACK enumDesktopProc(
		  LPTSTR lpszDesktop,
		  LPARAM lParam
		)
{

	EnumDesktopAction *action = (EnumDesktopAction*) lParam;
	char desktopName[1024] = "";

	HDESK old = GetThreadDesktop (GetCurrentThreadId());
	GetUserObjectInformation (old,
		UOI_NAME,
		desktopName, sizeof desktopName,
		NULL);
	printf ("Desktop %s (Current = %s)\n", lpszDesktop, desktopName);
//	if (strcmp (lpszDesktop, desktopName) == 0)
//		return true;

	action->highlight = strcmp (lpszDesktop, desktopName) == 0;

	HDESK hd = OpenDesktop (lpszDesktop, 0, false, GENERIC_ALL);
	if (hd != NULL)
	{
		action->currentDesktop = hd;
		EnumDesktopWindows (hd, enumWindowProc, lParam);
	}

	return true;
}

static void hotKey ()
{
	HDESK hDesktop = OpenInputDesktop(0, false, GENERIC_ALL);
	if (hDesktop != NULL)
	{
		EnumDesktopAction eda ;
		eda.execute = false;
		eda.openMenu = true;
		eda.updateMenu = false;
		eda.forwardAction = false;
		eda.nextId = IDM_USER_DESKTOP;
		eda.currentDesktop = hDesktop;
		EnumDesktopWindows(hDesktop, enumWindowProc, (LPARAM) &eda);
		CloseDesktop(hDesktop);
	} else {
		displayError("Unable to open default desktop");
	}
}


static void createDesktop ()
{
	if (KojiKabuto::desktopSession)
	{
		HDESK hDesktop = OpenDesktopA("Default", 0, false, GENERIC_ALL);
		if (hDesktop != NULL)
		{
			EnumDesktopAction eda ;
			eda.execute = false;
			eda.updateMenu = false;
			eda.forwardAction = true;
			eda.nextId = IDM_USER_DESKTOP;
			eda.currentDesktop = hDesktop;
			EnumDesktopWindows(hDesktop, enumWindowProc, (LPARAM) &eda);
			CloseDesktop(hDesktop);
		}
	}
	else
	{

		for (int i = 0; i < 49; i++)
		{
			char ach[20];
			sprintf (ach, "Soffid_%d", i);
			HDESK h = OpenDesktop (ach  ,
				0,
				0,
				GENERIC_ALL);
			if (h == NULL)
			{
				printf ("Desktop %s is free\n", ach);
				h = CreateDesktop (ach,
					NULL,
					NULL,
					0,
					GENERIC_ALL,
					NULL);
				if (h != NULL)
				{
					char achFilename[1000];
					if ( GetModuleFileName(NULL, achFilename, sizeof achFilename))
					{
						PROCESS_INFORMATION p;
						STARTUPINFO si;
						DWORD dwExitStatus;
						char achParams[1000];
						strcpy (achParams, "newDesktop");

						memset(&si, 0, sizeof si);
						si.cb = sizeof si;
						si.wShowWindow = SW_NORMAL;
						si.lpDesktop = ach;

						memset (&p, 0, sizeof p);

						if (CreateProcessA(achFilename, achParams,
								NULL, // Process Atributes
								NULL, // Thread Attributes
								FALSE, // bInheritHandles
								NORMAL_PRIORITY_CLASS, NULL, // Environment,
								NULL, // Current directory
								&si, // StartupInfo,
								&p) == 0)
						{
							displayError ("Error creating process");
						}
						else
						{
							Sleep(5000);
							CloseDesktop(h);
							return;
						}
					}
				}
			}
			else
				CloseDesktop(h);
		}
	}
}


static void ContextMenu (HWND hwnd)
{
	std::string enabledLocalAccounts;
	bool ok = MZNIsStarted(MZNC_getUserName());	// User authenticated status
	std::string closeByUser;							// Enable close session by user
	HMENU hm;															// Menu handler
	MENUITEMINFO mii;												// Menu item handler
	POINT pt;														// Cursor position

	mii.cbSize = sizeof mii;
	mii.fMask = MIIM_STATE;

	sessionStarted = sessionStarted && KojiKabuto::session->isOpen();

	// Check login information
	if (ok && sessionStarted)
	{
		mii.fState = MFS_UNHILITE;
		ModifyMenu(_hMenu, IDM_USER_ESSO, MF_BYCOMMAND, IDM_USER_ESSO,
				KojiKabuto::session->getSoffidUser());
	}
	else
	{
		mii.fState = MFS_DISABLED;
		ModifyMenu(_hMenu, IDM_USER_ESSO, MF_BYCOMMAND, IDM_USER_ESSO,
				Utils::LoadResourcesString(27).c_str());
	}

	SetMenuItemInfo(_hMenu, IDM_USER_ESSO, false, &mii);
	SetMenuItemInfo(_hMenu, IDM_UPDATE, false, &mii);
	SetMenuItemInfo(_hMenu, IDM_DISABLE_ESSO, false, &mii);

	SeyconCommon::readProperty("enableLocalAccounts", enabledLocalAccounts);
	if (enabledLocalAccounts != "true")
		mii.fState = MFS_DISABLED;
	SetMenuItemInfo(_hMenu, IDM_USER_NEW_DESKTOP, false, &mii);

	mii.fState = (ok || !sessionStarted) ? MFS_DISABLED : MFS_UNHILITE;
	SetMenuItemInfo(_hMenu, IDM_ENABLE_ESSO, false, &mii);

	// Enable login option
	mii.fState = (startingSession || sessionStarted) ? MFS_DISABLED : MFS_DEFAULT;
	SetMenuItemInfo(_hMenu, IDM_LOGIN, false, &mii);

	// Enable logoff option
	SeyconCommon::readProperty("EnableCloseSession", closeByUser);
	mii.fState =
			(sessionStarted && (closeByUser == "true")) ? MFS_UNHILITE : MFS_DISABLED;
	SetMenuItemInfo(_hMenu, IDM_LOGOFF, false, &mii);

	SetForegroundWindow(_hwnd);

	GetCursorPos(&pt);

	hm = GetSubMenu(_hMenu, 0);

	while (GetMenuItemCount(hm) > defaultMenuEntries)
	{
		DeleteMenu (hm, GetMenuItemCount(hm)-1, MF_BYPOSITION);
	}

	EnumDesktopAction eda ;
	eda.execute = false;
	eda.updateMenu = true;
	eda.forwardAction = false;
	eda.nextId = IDM_USER_DESKTOP;
	eda.hm = hm;
	EnumDesktops (GetProcessWindowStation(), enumDesktopProc, (LPARAM) &eda);

	TrackPopupMenu(hm, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0,
			_hwnd, NULL);
}

static LRESULT CALLBACK SystrayWndProc (HWND hwnd,        	// handle to window
		UINT uMsg,        		// message identifier
		WPARAM wParam,	// first message parameter
		LPARAM lParam)    	// second message parameter
{
	switch (uMsg)
	{
		case WM_HOTKEY:
			hotKey ();
			break;

		case WM_APP_SYSTRAY:
		{
			switch (lParam)
			{
				case WM_CONTEXTMENU:
				case WM_RBUTTONDOWN:
				case WM_LBUTTONUP:
				{
					ContextMenu(hwnd);
					fflush (stdout);

					return 0;
				}
			}
			break;
		}

		case WM_COMMAND:
		{
			printf ("Received command %d\n", wParam);
			fflush (stdout);
			switch (wParam)
			{
				case IDM_OPEN_MENU:
				{
					ContextMenu(hwnd);
					break;
				}
				case IDM_ENABLE_ESSO:
				{
					MZNStart(MZNC_getUserName());

					if (MZNIsStarted(MZNC_getUserName()))
					{
						playSound();
					}

					break;
				}

				case IDM_LOGIN:	// User login
				{
					if (!sessionStarted)
					{
						KojiKabuto::createProgressWindow(_hInstance);
						CreateThread(NULL, 0, KojiKabuto::loginThread, NULL, 0, NULL);
					}

					break;
				}

				case IDM_LOGOFF:		// User log off
				{
					if (sessionStarted)
					{
						KojiKabuto::CloseSession();
					}

					break;
				}

				case IDM_DISABLE_ESSO:
				{
					MZNStop(MZNC_getUserName());

					break;
				}

				case IDM_UPDATE:
				{
					if (KojiKabuto::session != NULL)
					{
						KojiKabuto::session->updateMazingerConfig();

						MessageBox(NULL, Utils::LoadResourcesString(18).c_str(),
								Utils::LoadResourcesString(1002).c_str(), MB_OK);
					}
					break;
				}

				case IDM_USER_NEW_DESKTOP:
				{
					createDesktop();
					break;
				}
				default:
					if (wParam >= IDM_USER_DESKTOP && wParam <= IDM_USER_DESKTOP + 16)
					{
						EnumDesktopAction eda ;
						eda.execute = true;
						eda.updateMenu = false;
						eda.nextId = IDM_USER_DESKTOP;
						eda.forwardAction = false;
						eda.idToExeucte = wParam;
						EnumDesktops (GetProcessWindowStation(), enumDesktopProc, (LPARAM) &eda);

					}
//				case IDM_CREDITS:
//				{
//					creditsDialog1();
//				}
			}

			break;
		}

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

BOOL createSystrayWndClass (HINSTANCE hinstance)
{
	WNDCLASSEX wcx;

	// Fill in the window class structure with parameters
	// that describe the main window.
	wcx.cbSize = sizeof(wcx);          // size of structure
	wcx.style = CS_HREDRAW | CS_VREDRAW;                    // redraw if size changes
	wcx.lpfnWndProc = SystrayWndProc;     // points to window procedure
	wcx.cbClsExtra = 0;                // no extra class memory
	wcx.cbWndExtra = 0;                // no extra window memory
	wcx.hInstance = hinstance;         // handle to instance
	wcx.hIcon = LoadIcon(NULL, (LPCSTR) IDI_ICON2);              // predefined app. icon
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW );                    // predefined arrow
	wcx.hbrBackground = (HBRUSH) GetStockObject(HOLLOW_BRUSH);  // white background brush
	wcx.lpszMenuName = NULL;    // name of menu resource
	wcx.lpszClassName = "KojiSystrayWClass";  // name of window class
	wcx.hIconSm = NULL;
	// Register the window class.

	return RegisterClassEx(&wcx);
}


static void testSessionTimeout ()
{
	std::string idleTimeout;

	// Timeout in seconds
	SeyconCommon::readProperty("soffid.esos.idleTimeout", idleTimeout);
	if (!idleTimeout.empty() )
	{
		long timeout = 99999;
		sscanf (idleTimeout.c_str(), "%ld", &timeout);
		MazingerEnv *pEnv = MazingerEnv::getDefaulEnv ();
		MAZINGER_DATA *data = pEnv->getDataRW();
		if (data != NULL)
		{
			time_t now;
			time (&now);
			if (now - data->lastUpdate >  timeout)
			{
				KojiKabuto::CloseSession();
				KojiKabuto::StartLoginProcess();
			}
		}
	}

}


static void updateKojiKabutoWindowText ()
{
	std::string text = INTERNAL_WND_PREFIX;
	bool ok = MZNIsStarted(MZNC_getUserName());
	if (! ok )
	{
		text += "Empty desktop";
	}
	else
	{
		SecretStore ss ( MZNC_getUserName() );
		wchar_t * userName = ss.getSecret(L"fullName");
		if (userName == NULL || userName[0] == L'\0')
		{
			ss.freeSecret(userName);
			userName = ss.getSecret(L"user");
		}
		text += MZNC_wstrtostr(userName);
		ss.freeSecret(userName);
	}
	SetWindowText (_hwnd, text.c_str());
}

static DWORD WINAPI systrayThread (void* lparam)
{
	HICON hIconFail = LoadIconA(_hInstance, MAKEINTRESOURCE(IDI_ICONGRAY) );
	HICON hIconOk = LoadIconA(_hInstance, MAKEINTRESOURCE(IDI_ICON2) );
	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof nid);
	nid.cbSize = sizeof nid;
	nid.hIcon = hIconFail;
	nid.hWnd = _hwnd;
	strcpy(nid.szTip, "Unknown state");
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	nid.uCallbackMessage = WM_APP_SYSTRAY;
	DWORD dwAction = NIM_ADD;

	std::string version;

	SeyconCommon::readProperty("MazingerVersion", version);
	bool laststatus;
	bool change = true;
	const int maxTip = (sizeof nid.szTip) - 1;
	int lastChangeElapsed = 0;
	while (true)
	{
		testSessionTimeout ();
		bool ok = MZNIsStarted(MZNC_getUserName());
		if (laststatus != ok)
			change = true;
		if (change || lastChangeElapsed > 120)
		{
			updateKojiKabutoWindowText ();

			laststatus = ok;
			std::string msg;
			if (ok)
			{
				nid.hIcon = hIconOk;
				msg = Utils::LoadResourcesString(19);
			}
			else
			{
				nid.hIcon = hIconFail;
				msg = Utils::LoadResourcesString(20);
			}

			char buffer[50];

			sprintf(buffer, Utils::LoadResourcesString(21).c_str(), version.c_str());
			msg += buffer;
			strncpy(nid.szTip, msg.c_str(), maxTip);
			nid.szTip[maxTip] = '\0';

			if (Shell_NotifyIconA(dwAction, &nid))
			{
				change = false;
				dwAction = NIM_MODIFY;
				lastChangeElapsed = 0;
			}
			else
			{
				dwAction = NIM_ADD;
			}
		}
		Sleep(1000);
		lastChangeElapsed++;
	}
	return 0;
}

typedef WINAPI BOOL (*CWMFType) (HWND hwnd, UINT message, DWORD action, LPVOID pointer);

static void setMessageFilterWindows7 (HWND hwnd)
{
	HMODULE hModule = LoadLibraryA("user32");
	if (hModule != NULL)
	{
		CWMFType cwmf = (CWMFType) GetProcAddress(hModule, "ChangeWindowMessageFilterEx");
		if (cwmf != NULL)
		{
			BOOL result = cwmf (hwnd, WM_COMMAND,  1 /*MSGFLT_ALLOW*/, NULL);
		}
	}

}

void createSystrayWindow (HINSTANCE hInstance)
{
	_hInstance = hInstance;
	createSystrayWndClass(hInstance);

	// Create the main window.

	_hwnd = CreateWindow(
			"KojiSystrayWClass",        // name of window class
			"",// title-bar string
			WS_POPUP,// top-level window
			0,// default horizontal position
			0,// default vertical position
			10,// default width
			10,// default height
			(HWND) NULL,// no owner window
			(HMENU) NULL,// use class menu
			hInstance,// handle to application instance
			(LPVOID) NULL);// no window-creation data

	if (!_hwnd)
		return;

	setMessageFilterWindows7 (_hwnd);

	// Show the window and send a WM_PAINT message to the window
	// procedure.

	//
	ShowWindow(_hwnd, SW_HIDE);
	//
	UpdateWindow(_hwnd);

	_hMenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_POPUPMENU) );
	HMENU hm = GetSubMenu(_hMenu, 0);
	defaultMenuEntries = GetMenuItemCount (hm);

	RegisterHotKey (_hwnd, 1, MOD_WIN, 'Z');

	CreateThread(NULL, 0, systrayThread, NULL, 0, NULL);
}
