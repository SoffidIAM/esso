/*
 * ProcessDetector.cpp
 *
 *  Created on: Dec 17, 2020
 *      Author: gbuades
 */

# include "KojiKabuto.h"

#include "ProcessDetector.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <string>
#include <MazingerEnv.h>

ProcessDetector::ProcessDetector() {
}

ProcessDetector::~ProcessDetector() {
}
// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

static std::string getProcessName( DWORD processID )
{
    char szProcessName[MAX_PATH] = "";

    // Get a handle to the process.

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod),
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName,
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
    }

    // Print the process name and identifier.

    std::string r  = szProcessName;

    // Release the handle to the process.

    CloseHandle( hProcess );
    return r;
}

std::string ProcessDetector::getProcessList()
{
    // Get the list of process identifiers.

    DWORD aProcesses[10240], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return std::string("??");
    }


    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.
    std::string r = "";
    for ( i = 0; i < cProcesses; i++ )
    {
        if( aProcesses[i] != 0 )
        {
        	std::string n = getProcessName(aProcesses[i]);
        	if (n.length() > 0) {
        		bool ignore = false;
        		if (stricmp(n.c_str(), "kojikabuto.exe") == 0) ignore = true;
        		else if (stricmp(n.c_str(), "rundll32.exe") == 0 ) ignore = true;
        		else {
        			HKEY hKey = NULL;
        			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\SysProcs", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        				DWORD r;
        				DWORD size = sizeof r;
        				DWORD dwType;
        				if (RegQueryValueExA(hKey, n.c_str(), (LPDWORD) NULL, &dwType, (LPBYTE) &r, &size) == ERROR_SUCCESS)
        					ignore = true;
        				RegCloseKey(hKey);
        			}

        		}
        		if (!ignore) {
					r += n;
					r += "\n";
        		}
        	}
        }
    }

    return r;
}

DWORD ProcessDetector::mainLoop(LPVOID param) {
	ProcessDetector pd;
	bool last = true;
	do {
		bool now = pd.getProcessList().length() > 0;
		if ( !now && ! last) {
			MZNStop(MZNC_getUserName());
			Sleep(1000);
			ExitProcess(12);
		}
		last = now;
		Sleep(3000);
	} while (true);
}
