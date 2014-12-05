#include "../MazingerHookImpl.h"
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <vector>
#include <time.h>

#include <DomainPasswordCheck.h>
#include <ConfigReader.h>
#include <Action.h>
#include <SecretStore.h>
#include "../java/JavaVirtualMachine.h"
#include <MazingerEnv.h>

HHOOK hhk;
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);

LPCWSTR MAZINGER_EVENT_NAME = L"Local\\Mazinger-Stop-Event";

static std::wstring getStopSemaphoreName(const char *user) {
	// Base name
	std::wstring semName = MAZINGER_EVENT_NAME;

	// Append user name
	if (user == NULL)
		user = MZNC_getUserName();

	semName.append(MZNC_strtowstr(user));

	// Append desktop name

	HANDLE hd = GetThreadDesktop (GetCurrentThreadId());
	wchar_t desktopName[1024];

	wcscpy (desktopName, L"Default");

	GetUserObjectInformationW (hd,
		UOI_NAME,
		desktopName, sizeof desktopName,
		NULL);

	semName.append(desktopName);
	return semName;

}

void cleanMazingerData(const char*user) {
	MZNC_waitMutex();
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
	if (pEnv != NULL)
	{
		MAZINGER_DATA *pData = pEnv->getDataRW();
		if (pData != NULL)
			pData->started = 0;
		delete pEnv;
		if (stopCallback != NULL)
			stopCallback(user);
	}
	MZNC_endMutex();
	printf("Mazinger stopped\n");
}

DWORD WINAPI waitforstop(LPVOID data) {
	char *user = (char*) data;
	std::wstring semName = getStopSemaphoreName(user);
	HANDLE ghSvcStopEvent = CreateEventW(NULL, // default security attributes
			FALSE, // manual reset event
			FALSE, // not signaled
			semName.c_str());
	HHOOK hhook = SetWindowsHookEx(WH_CBT, (HOOKPROC) CBTProc, hMazingerInstance, 0);
	hhk = hhook;
	WaitForSingleObject(ghSvcStopEvent, INFINITE);
	printf("Received stop request");
	CloseHandle(ghSvcStopEvent);
	cleanMazingerData(user);
	free(user);
	if (hhk != NULL)
	{
		UnhookWindowsHookEx(hhook);
		hhk = NULL;
	}
	return 0;
}



extern "C" __declspec(dllexport) void MZNStart(const char*user) {
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
	pEnv->getDataRW()->started = 1;

	MZNSendDebugMessage("Starting mzn for user %s", user);
	char *tuser = strdup(user);
	CreateThread(NULL, 0, waitforstop, tuser, 0, NULL);

	// Creating .java.policy
	JavaVirtualMachine::adjustPolicy();
	ConfigReader *config = pEnv->getConfigReader();
	for (std::vector<Action*>::iterator it = config->getGlobalActions().begin();
			it != config->getGlobalActions().end(); it++) {
		Action*action = (*it);
		action->executeAction();
	}
#ifdef _WIN64
	char achFileName[4096];
	if (GetModuleFileName (hMazingerInstance, achFileName, sizeof achFileName) != 0) {
		std::string fileName32 = achFileName;
		std::string::size_type len = fileName32.size();
		std::string::size_type dot = fileName32.find_last_of('.');
		if (dot > len - 6 && dot != std::string::npos)
			fileName32 = fileName32.substr(0, dot);
		fileName32 += "32.dll";

		std::string cmdLine = "RUNDLL32 \"";
		cmdLine += fileName32;
		cmdLine += "\",StartWow64";
		STARTUPINFO si;
		memset(&si, 0, sizeof si);
		si.cb = sizeof si;
		PROCESS_INFORMATION pi;
		CreateProcessA(NULL, (char*) cmdLine.c_str(), // CommandLine
				NULL, // Process attr
				NULL, // Thread attr
				FALSE, // Inherit handles
				0, // Flags
				NULL, // Environment
				NULL, // Current dir
				&si, // Startup info
				&pi);
		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

	}
#endif
}

extern "C" __declspec(dllexport) void MZNStop(const char*user) {
	MZNSendDebugMessage("Stoping mzn for user %s", user);
	std::wstring semName = getStopSemaphoreName(user);
	HANDLE ghSvcStopEvent = OpenEventW(EVENT_MODIFY_STATE, // default security attributes
			FALSE, // not inherited
			semName.c_str()); // no name
	if (ghSvcStopEvent != NULL) {
		SetEvent(ghSvcStopEvent);
		CloseHandle(ghSvcStopEvent);
	}
#ifdef _WIN64
	semName += L"_WOW64";
	ghSvcStopEvent = OpenEventW(EVENT_MODIFY_STATE, // default security attributes
			FALSE, // not inherited
			semName.c_str()); // no name
	if (ghSvcStopEvent != NULL) {
		SetEvent(ghSvcStopEvent);
		CloseHandle(ghSvcStopEvent);
	}
#endif
	cleanMazingerData(user);
}

extern "C" BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_DETACH) {
		// Solo debe descargarse de la JVM si se descarga la DLL por un STOP de mazinger
		// Si se descarga por un fin de proceso, no se debe descargar la JVM
		// para evitar un BUG de Java6
		//
		MAZINGER_DATA *pData = MazingerEnv::getDefaulEnv()->getData();
		if (pData == NULL || !pData->started) {
			uninstallJavaPlugin();
		}
	}
	if (dwReason == DLL_PROCESS_ATTACH) {
		hMazingerInstance = hinstDLL;
	}
	return TRUE;
}

HINSTANCE getDllHandler() {
	return hMazingerInstance;
}

#if 0
static void win32Error() {
	LPWSTR pstr;
	DWORD dw = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dw, 0, (LPWSTR) & pstr, 0, NULL);
	if (pstr == NULL)
	wprintf(L"Unknown error: %d\n", dw);
	else
	wprintf(L"Error: %s\n", pstr);
	LocalFree(pstr);
}
#endif

static void createMapFile(PMAZINGER_DATA pData, DWORD dwSize, HANDLE hFile) {
	wchar_t wchBuffer[150];
	time_t t;
	time(&t);
	swprintf(wchBuffer, L"MAZINGER_RULES_%ld_%ld", (long) GetCurrentProcessId(),
			(long) t);
	HANDLE hMapFile = CreateFileMappingW((HANDLE) - 1, // Current file handle.
	NULL, // Default security.
			PAGE_READWRITE, // Read/write permission.
			0, dwSize, // Size of hFile.
			wchBuffer); // Name of mapping object.

	LPBYTE gRules = (byte*) MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0,
			dwSize);

	if (gRules != NULL) {
		DWORD dwRead = 0;
		ReadFile(hFile, gRules, dwSize, &dwRead, NULL);
		if (dwRead == dwSize) {
			wcscpy(pData->achRulesFile, wchBuffer);
			pData->dwRulesSize = dwSize;
		} else {
			pData->achRulesFile[0] = '\0';
			printf(
					"ERROR reading shared memory: Expected to read %d bytes. Read %d\n",
					(int) dwSize, (int) dwRead);
		}
		UnmapViewOfFile(gRules);
	} else {
		printf("Unable to open map file %ls\n", wchBuffer);
	}
}

extern "C" __declspec(dllexport) bool MZNLoadConfiguration(const char *user,
		LPCWSTR szFile) {

	PMAZINGER_DATA pData = MazingerEnv::getEnv(user)->getDataRW();

	if (szFile == NULL) {
		TRACE;
		pData->dwRulesSize = 0;
		pData->achRulesFile[0] = L'\0';
		return TRUE;
	} else {
		TRACE;

		HANDLE hFile = CreateFileW(szFile, GENERIC_READ, 0, 0, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == NULL) {
			wprintf(L"Unable to open file [%s]\n", szFile);
			return FALSE;
		}

		DWORD size = GetFileSize(hFile, NULL);
		createMapFile(pData, size, hFile);
		CloseHandle(hFile);
		MZNSendDebugMessageA("TESTING configuration (v3)\n");
		ConfigReader *currentConfig = new ConfigReader(pData);
		currentConfig->testConfiguration();
		MZNSendDebugMessageA("CONFIGURATION TESTED  !!!\n");
		time(& pData->lastUpdate);
		return TRUE;
	}
}


#ifndef _WIN64
extern "C" __declspec(dllexport) void StartWow64() {

	HHOOK hhk = SetWindowsHookEx(WH_CBT, (HOOKPROC) CBTProc, hMazingerInstance, NULL);

	const char *user = MZNC_getUserName();
	std::wstring semName = getStopSemaphoreName(user);
	semName+=L"_WOW64";
	HANDLE ghSvcStopEvent = CreateEventW(NULL, // default security attributes
			FALSE, // manual reset event
			FALSE, // not signaled
			semName.c_str());
	WaitForSingleObject(ghSvcStopEvent, INFINITE);
	if (hhk != NULL)
		UnhookWindowsHookEx(hhk);
}

#endif


