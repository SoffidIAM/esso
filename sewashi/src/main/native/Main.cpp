#include "hllapi.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <HllMatcher.h>
#include "ActualHllApplication.h"
#include "MazingerInternal.h"

struct ThreadInfo
{
	HllApi *api;
	char session;
};

bool polling = false;
int pollDelay = 3000;

#define DEBUG

static DWORD CALLBACK threadFunction (void * param)
{

	ThreadInfo *ti = (ThreadInfo*) param;
	HANDLE handle = NULL;

#ifdef DEBUG
	MZNSendDebugMessageA ("HLLAPI: Starting thread for session %c\n", ti->session);
#endif
	do {
		bool connected = false;
		do {
			int result = ti->api->startHostNotification (ti->session, handle);
			if (result == 0)
				connected = true;
			else
				Sleep(3);
		} while (! connected );
		MZNSendDebugMessageA ("HLLAPI: Session %c Connected\n", ti->session);

		bool process = true;

		while (connected) {
			if (process || polling)
			{
				MZNSendDebugMessageA ("HLLAPI: Testing session %c\n", ti->session);
				// Detected change
				ActualHllApplication app (ti->api, ti->session);
				MZNHllMatch(&app);
			}
			std::string id, name, type;
			int rows, cols, codepage;
			int result = ti->api->querySesssionStatus (ti->session, id, name, type, rows, cols, codepage);
			if (result != 0)
				connected = false;
			else
			{
				DWORD dwResult = WaitForSingleObject(handle, pollDelay);
				process = ( dwResult == WAIT_OBJECT_0 ) ;
				MZNSendDebugMessageA ("HLLAPI: Session %c detected change %d\n", ti->session, (int) process);
			}
		}

		int result = ti->api->stopHostNotification (ti->session);

#ifdef DEBUG
		MZNSendDebugMessageA ("Seesion %c Disconnected\n", ti->session);
#endif
	} while (true);
	return 0;
}

const char * SIGNAL_NAME = "Local\\SewashiStopSignal";

extern "C" int __stdcall WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
	    PSTR lpCmdLine, INT nCmdShow) {

	std::wstring sessions = L"AB";
	std::wstring dll;

	wchar_t *cmdLine = GetCommandLineW( );
	int argc;
	wchar_t** argv = CommandLineToArgvW (cmdLine, &argc);

	for (int i = 1; i < argc; i++)
	{
		if (wcscmp (argv[i], L"--dll") == 0 || wcscmp (argv[i], L"-d") == 0)
		{
			dll = argv[++i];
		}
		else if (wcscmp (argv[i], L"--stop") == 0)
		{
			MZNSendDebugMessageA ("Opening handle %s\n", SIGNAL_NAME);
			HANDLE ghSvcStopEvent = OpenEvent(EVENT_MODIFY_STATE,
					FALSE, // not inherited
					SIGNAL_NAME); // no name
			if (ghSvcStopEvent != NULL) {
				MZNSendDebugMessageA ("Sending stop event\n");
				SetEvent(ghSvcStopEvent);
				CloseHandle(ghSvcStopEvent);
			}
			return 0;
		}
		else if (wcscmp (argv[i], L"--sessions") == 0 || wcscmp (argv[i], L"-s") == 0)
		{
			sessions = argv[++i];
		}
		else if (wcscmp (argv[i], L"--poll") == 0 )
		{
			int number = 3;
			polling = true;
			swscanf (argv[++i], L"%d", &number);
			pollDelay = number * 1000;
		}
		else
		{
			std::wstring s = std::wstring(L"Invalid switch ")+argv[i]+L"\nSyntax:\nsewashi --dll [DLL] --sessions [SESSIONS] --poll [seconds]";
			MessageBoxW (NULL, s.c_str(), L"Soffid ESSO", MB_OK|MB_ICONEXCLAMATION);

			return 1;
		}

	}



	HllApi *api = new HllApi (dll.c_str());

	if (!api->isConnected())
	{
		MessageBox (NULL, "Unable to load HLL API library", "Soffid ESSO", MB_OK|MB_ICONEXCLAMATION);
		return 1;
	}
	int size = sessions.length();
	ThreadInfo *threads = new ThreadInfo[size];

	for (unsigned int i = 0; i < sessions.length(); i++)
	{
		threads[i].api = api;
		threads[i].session = (char) sessions[i];
		CreateThread (NULL, 0, threadFunction, &threads[i], 0, NULL);
	}

	HWND hwnd; /* This is the handle for our window */
	MSG messages; /* Here messages to the application are saved */

	SECURITY_DESCRIPTOR sd = { 0 };
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;


	HANDLE ghSvcStopEvent = CreateEvent( NULL, // default security attributes
			FALSE, // manual reset event
			FALSE, // not signaled
			SIGNAL_NAME);
	if (ghSvcStopEvent != NULL) {
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		ExitProcess (0);
	} else {
		MessageBox (NULL, "Unable to create stop event", "Soffid HLL Wrapper", MB_OK);
	}

//	while (GetMessage (&messages, NULL, 0, 0))
//	{
//		/* Translate virtual-key messages into character messages */
//		TranslateMessage(&messages);
//		/* Send message to WindowProcedure */
//		DispatchMessage(&messages);
//	}
//
//	/* The program return-value is 0 - The value that PostQuitMessage() gave */
//	return messages.wParam;
	return 0;
}


