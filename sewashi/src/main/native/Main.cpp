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


static DWORD CALLBACK threadFunction (void * param)
{

	ThreadInfo *ti = (ThreadInfo*) param;
	HANDLE handle = NULL;

#ifdef DEBUG
	printf ("Starting thread for session %c\n", ti->session);
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
#ifdef DEBUG
		printf ("Session %c Connected\n", ti->session);
#endif
		while (connected) {
			DWORD dwResult = WaitForSingleObject(handle, 3000);
			if ( dwResult == WAIT_OBJECT_0 ) {
				// Detected change
				ActualHllApplication app (ti->api, ti->session);
				MZNHllMatch(&app);
			}
			std::string id, name, type;
			int rows, cols, codepage;
			int result = ti->api->querySesssionStatus (ti->session, id, name, type, rows, cols, codepage);
			if (result != 0)
				connected = false;
		}
#ifdef DEBUG
		printf ("Seesion %c Disconnected\n", ti->session);
#endif
	} while (true);
	return 0;
}

extern "C" int main(int argc, char**argv) {

	std::string sessions = "AB";
	std::string dll;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp (argv[i], "--dll") == 0 || strcmp (argv[i], "-d") == 0)
		{
			dll = argv[++i];
		}
		else if (strcmp (argv[i], "--sessions") == 0 || strcmp (argv[i], "-s") == 0)
		{
			sessions = argv[++i];
		}
		else
		{
			std::string s = std::string("Invalid switch ")+argv[i]+"\nSyntax:\nsewashi --dll [DLL] --sessions [SESSIONS]";
			MessageBoxA(NULL, s.c_str(), "Soffid HLL SSO monitor", MB_OK);
			return 1;
		}

	}

	HllApi *api = new HllApi (dll.c_str());

	int size = sessions.length();
	ThreadInfo *threads = new ThreadInfo[size];

	for (unsigned int i = 0; i < sessions.length(); i++)
	{
		threads[i].api = api;
		threads[i].session = sessions[i];
		if (i == sessions.length() - 1)
			threadFunction (&threads[i]);
		else
			CreateThread (NULL, 0, threadFunction, &threads[i], 0, NULL);
	}
	return 0;
}


