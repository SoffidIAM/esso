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

#include <MazingerInternal.h>

# include "KojiKabuto.h"
# include "Utils.h"
# include <stdio.h>

#define WM_APP_SYSTRAY (WM_APP + 1)

static HMENU _hMenu;
static HINSTANCE _hInstance;
static HWND _hwnd;
static int defaultMenuEntries = 0;

bool sessionStarted;	// Session started status
bool startingSession;	// Session is starting

void creditsDialog1 ();

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
	bool updateMenu;
	bool execute;
	HMENU hm;
	int nextId;
	int idToExeucte;
};

static BOOL CALLBACK enumDesktopProc(
		  LPTSTR lpszDesktop,
		  LPARAM lParam
		)
{
	HANDLE hd = GetThreadDesktop (GetCurrentThreadId());
	EnumDesktopAction *action = (EnumDesktopAction*) lParam;
	char desktopName[1024] = "";

	GetUserObjectInformation (hd,
		UOI_NAME,
		desktopName, sizeof desktopName,
		NULL);
	printf ("Desktop %s (Current = %s)\n", lpszDesktop, desktopName);
	if (strcmp (lpszDesktop, desktopName) == 0)
		return true;


	std::string user = MZNC_getUserName();
	MazingerEnv *penv = MazingerEnv::getEnv (user.c_str(), lpszDesktop);
	if (penv != NULL && penv->getDataRW() != NULL)
	{
		printf ("Processing desktop %s (Current = %s)\n", lpszDesktop, desktopName);
		if ( action->updateMenu )
		{

			SecretStore ss ( MZNC_getUserName(), lpszDesktop);
			wchar_t * userName = ss.getSecret(L"fullName");
			if (userName == NULL || userName[0] == L'\0')
			{
				ss.freeSecret(userName);
				userName = ss.getSecret(L"user");
			}
			printf ("User = %ls\n", userName);
			if (userName != NULL && userName [0] != L'\0')
			{
				// Get desktop user
				if (action->nextId == IDM_USER_DESKTOP)
					AppendMenuW (action->hm, MF_SEPARATOR, -1, L"");
				AppendMenuW (action->hm, MF_STRING, action->nextId, userName);
			}
			ss.freeSecret(userName);

		}
		printf ("Action to find = %d Current action = %d\n",
				action->idToExeucte, action->nextId);
		if (action-> execute && action->nextId == action->idToExeucte)
		{
		 	HDESK h = OpenDesktop (lpszDesktop  ,
				NULL,
				0,
				GENERIC_ALL);
			if (h != NULL)
			{
				printf ("Switching to desktop %s", lpszDesktop);
				SwitchDesktop(h);
				CloseHandle(h);
			}
		}
		action -> nextId ++;
		delete penv;
	}
	fflush (stdout);
	return true;
}

static void createDesktop ()
{
	printf ("Crating desktop\n");
	for (int i = 0; i < 9999; i++)
	{
		char ach[20];
		sprintf (ach, "Desktop_%d", i);
		printf ("Probing desktop %s\n", ach);
	 	HDESK h = OpenDesktop (ach  ,
			NULL,
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
				printf ("Created desktop %s\n", ach);
				char achFilename[1000];
				PROCESS_INFORMATION p;
				STARTUPINFO si;
				memset(&si, 0, sizeof si);
				si.cb = sizeof si;
				si.wShowWindow = SW_NORMAL;
				si.lpDesktop = ach;


				fflush (stdout);

				if ( GetModuleFileNameA(NULL, achFilename, sizeof achFilename))
				{
					printf ("Creating process %s\n", achFilename);
					fflush (stdout);
					char achParams[100];
					strcpy (achParams, "newDesktop");

					CreateProcess(achFilename, achParams,
							NULL, // Process Atributes
							NULL, // Thread Attributes
							FALSE, // bInheritHandles
							NORMAL_PRIORITY_CLASS, NULL, // Environment,
							NULL, // Current directory
							&si, // StartupInfo,
							&p);


					if (SwitchDesktop (h))
					{
						printf ("Switched desktop\n");
						fflush (stdout);
					}
//					CloseHandle(h);
					return;
				}

			}
		}
		CloseHandle(h);
	}

}


static void ContextMenu (HWND hwnd)
{
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
		case WM_APP_SYSTRAY:
		{
			switch (lParam)
			{
				case WM_CONTEXTMENU:
				case WM_RBUTTONDOWN:
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

	// Show the window and send a WM_PAINT message to the window
	// procedure.

	//
	ShowWindow(_hwnd, SW_HIDE);
	//
	printf("Created window\n");
	UpdateWindow(_hwnd);

	_hMenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_POPUPMENU) );
	HMENU hm = GetSubMenu(_hMenu, 0);
	defaultMenuEntries = GetMenuItemCount (hm);

	CreateThread(NULL, 0, systrayThread, NULL, 0, NULL);
}
