# include "KojiKabuto.h"
#include <userenv.h>

#include <MazingerEnv.h>
#include "SoffidDirHandler.h"
#include "ProcessDetector.h"

HINSTANCE hKojiInstance;
SeyconSession *KojiKabuto::session = NULL;
HWND hwndLogon = NULL;

extern bool sessionStarted;
extern bool startingSession;

bool KojiKabuto::usedKerberos = false;
bool KojiKabuto::desktopSession = false;
bool KojiKabuto::windowLess = false;

HWND createKojiWindow(HINSTANCE hinstance);
void createSystrayWindow(HINSTANCE hInstance);

void KojiKabuto::enableTaskManager(DWORD dwEnable) {
	if (!KojiKabuto::desktopSession) {
		if (dwEnable)
			KOJITaskMgrEnable();
		else
			KOJITaskMgrDisable();
	}
}

int KojiKabuto::runProgram(char *achString, char *achDir, int bWait) {
	PROCESS_INFORMATION p;
	STARTUPINFO si;
	DWORD dwExitStatus;

	printf("Executing %s from dir: %s\n", achString, achDir);

	memset(&si, 0, sizeof si);
	si.cb = sizeof si;
	si.wShowWindow = SW_NORMAL;

	if (CreateProcess(NULL, achString, NULL, // Process Atributes
			NULL, // Thread Attributes
			FALSE, // bInheritHandles
			NORMAL_PRIORITY_CLASS, NULL, // Environment,
			achDir, // Current directory
			&si, // StartupInfo,
			&p)) {
		if (bWait) {
			MSG msg;
			do {
				Sleep(1000);
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) //Or use an if statement
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				GetExitCodeProcess(p.hProcess, &dwExitStatus);
			} while (dwExitStatus == STILL_ACTIVE);

			CloseHandle(p.hProcess);
			CloseHandle(p.hThread);
		}

		return 0;
	}

	else {
		MessageBox(NULL, achString, Utils::LoadResourcesString(9).c_str(),
		MB_OK | MB_ICONSTOP);

		return -1;
	}
}

static void waitForDesktop () {
	std::string loginType;	// Login type registry key value
	SeyconCommon::readProperty("LoginType", loginType);

	HDESK hdesk = GetThreadDesktop(GetCurrentThreadId());

	if (loginType != "manual") {
		DWORD size;
		do {
			DWORD dwIO = 0;
			if (GetUserObjectInformationA( hdesk, 6 /*UOI_IO*/, &dwIO, sizeof dwIO, &size )) {
				if (dwIO) break;
			} else {
				break;
			}
			Sleep(500);
		} while (true);
	}

	if (hdesk == NULL) {
		SeyconCommon::debug("Cannot open desktop Default");
	} else {
		if (SwitchDesktop(hdesk)) {
			SeyconCommon::debug("Switch desktop Default SUCCESS");
		}
		else {
			SeyconCommon::debug("Switch desktop Default FAILURE");
		}
	}
}

void KojiKabuto::runScript(LPCSTR lpszScript) {
	char achCmdLine[2048];
	sprintf(achCmdLine, "c:\\tcl\\bin\\wish80 \"%s\"", lpszScript);
	char achDir[2048];
	strcpy(achDir, lpszScript);
	int i = strlen(achDir);

	while (i > 0 && achDir[i] != '\\')
		i--;

	achDir[i] = '\0';

	runProgram(achCmdLine, achDir, TRUE);
}

void KojiKabuto::consumeMessages() {
//	MSG msg;

//	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
//	{
//		DispatchMessage (&msg);
//	}
}

void KojiKabuto::createProgressWindow(HINSTANCE hInstance) {
	if ( ! KojiKabuto::windowLess) {
		if (hwndLogon != NULL)
			destroyProgressWindow();
		hwndLogon = createKojiWindow(hInstance);
		HDESK hd = GetThreadDesktop(GetCurrentThreadId());
		SwitchDesktop(hd);
		return;
	}
}

void KojiKabuto::destroyProgressWindow() {
	if (! KojiKabuto::windowLess && hwndLogon != NULL) {
		HDESK hd = GetThreadDesktop(GetCurrentThreadId());
		SwitchDesktop(hd);
		consumeMessages();
		ShowWindow(hwndLogon, SW_HIDE);
		consumeMessages();
		DestroyWindow(hwndLogon);
		hwndLogon = NULL;
	}
}

void KojiKabuto::displayProgressMessage(LPCSTR achString) {
	if (! KojiKabuto::windowLess) {
		if (hwndLogon != NULL) {
			consumeMessages();
	//		SetWindowTextA(hwndLogon, achString);
			SetDlgItemText(hwndLogon, IDC_TEXT, achString);
			consumeMessages();
		}
	}
}

BOOL KojiKabuto::mountScriptDrive() {
	NETRESOURCE nr;
	char *token;
	BOOL bMontado = FALSE;

	std::string mounts;
	SeyconCommon::readProperty("LogonPath", mounts);
	std::string localDrive;
	SeyconCommon::readProperty("LogonDrive", localDrive);
	if (localDrive.empty())
		localDrive = "Z:";

	char* achMounts = strdup(mounts.c_str());

	token = strtok(achMounts, ", ");

	while (token != NULL && !bMontado) {
		if (token[0] != '\0') {
			displayProgressMessage(token);
			nr.dwType = RESOURCETYPE_ANY;
			nr.lpLocalName = (char*) localDrive.c_str();
			nr.lpRemoteName = token;
			nr.lpProvider = NULL;
			if (WNetAddConnection2(&nr, NULL, NULL, 0) == NO_ERROR) {
				bMontado = TRUE;
			}

			else {
				SeyconCommon::warn("Unable to mount drive %s: %s\n",
						localDrive.c_str(), token);
				SeyconCommon::notifyError();
			}
		}

		/* Get next token: */
		token = strtok((char*) NULL, ", ");
	}

	free(achMounts);

	return bMontado;
}

void KojiKabuto::umountScriptDrive() {
	std::string localDrive;
	SeyconCommon::readProperty("LogonDrive", localDrive);

	if (localDrive.empty())
		localDrive = "Z:";

	WNetCancelConnection2(localDrive.c_str(), CONNECT_UPDATE_PROFILE, TRUE);
}

void KojiKabuto::writeIntegerProperty(HKEY hKey, const char *name, int value) {
	char achValue[30];
	sprintf(achValue, "%d", value);
	RegSetValueExA(hKey, name, NULL, REG_SZ, (LPBYTE) achValue,
			strlen(achValue) + 1);
}

int KojiKabuto::readIntegerProperty(HKEY hKey, const char *name) {
	char *achValue;
	DWORD size = 0;
	DWORD dwType;
	RegQueryValueEx(hKey, name, NULL, &dwType, (LPBYTE) NULL, &size);

	if (size > 0) {
		achValue = (char*) malloc(size + 1);
		RegQueryValueEx(hKey, name, NULL, &dwType, (LPBYTE) achValue, &size);
		achValue[size] = '\0';
		int v = 0;
		sscanf(achValue, " %d", &v);
		free(achValue);
		return v;
	}

	else {
		return 0;
	}
}

bool KojiKabuto::startDisabled() {
	std::string disabled;
	SeyconCommon::readProperty("startDisabled", disabled);
	return disabled.size() > 0;
}

void KojiKabuto::checkScreenSaver() {
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
		DWORD dwActive = readIntegerProperty(hKey, "ScreenSaveActive");
		DWORD dwTimeout = readIntegerProperty(hKey, "ScreenSaveTimeOut");
		DWORD dwSecure = readIntegerProperty(hKey, "ScreenSaverIsSecure");
		bool change = false;

		if (!dwActive || !dwSecure) {
			int r = MessageBoxA(NULL, Utils::LoadResourcesString(10).c_str(),
					Utils::LoadResourcesString(11).c_str(), MB_YESNO);

			if (r == IDYES) {
				dwActive = 1;
				dwSecure = 1;
				change = true;
				if (dwTimeout < 600)
					dwTimeout = 600;
				const char ach[] = "scrnsave.scr";
				RegSetValueExA(hKey, "SCRNSAVE.EXE", NULL, REG_SZ, (LPBYTE) ach,
						strlen(ach) + 1);
			}
		}

		else if (dwTimeout > 900) {
			int r = MessageBoxA(NULL, Utils::LoadResourcesString(12).c_str(),
					Utils::LoadResourcesString(11).c_str(), MB_YESNO);

			if (r == IDYES) {
				change = true;
				dwTimeout = 600;
			}
		}

		if (change) {
			writeIntegerProperty(hKey, "ScreenSaveActive", dwActive);
			writeIntegerProperty(hKey, "ScreenSaverIsSecure", dwSecure);
			writeIntegerProperty(hKey, "ScreenSaveTimeOut", dwTimeout);
		}
	}
}

// Force login process
void KojiKabuto::ForceLogin() {
	std::string forceLogin;	// Force login on startup registry key value

	SeyconCommon::readProperty("ForceStartupLogin", forceLogin);

	destroyProgressWindow();

	// Check force login
	if (desktopSession) {
//		SwitchDesktop(OpenDesktop("Default", 0, 0, GENERIC_ALL));
	} else if (forceLogin != "false") {
		ExitWindowsEx(EWX_LOGOFF, 0);
	}
}

// Login error process
void KojiKabuto::LoginErrorProcess() {
	std::string clientName;			// Client name
	DWORD offlineAllowed = 0;	// Offline login allowed

	destroyProgressWindow();

	SeyconCommon::getCitrixClientName(clientName);

	// Check client name
	if (clientName.empty()) {
		offlineAllowed = SeyconCommon::readIntProperty("LocalOfflineAllowed");
	}

	else {
		offlineAllowed = SeyconCommon::readIntProperty("RemoteOfflineAllowed");
	}

	// Check offline allowed
	if (offlineAllowed && !desktopSession) {
		MessageBox(NULL, (session->getErrorMessage()),
				Utils::LoadResourcesString(1001).c_str(),
				MB_OK | MB_ICONWARNING);

		session->executeOfflineScript();
	}

	else {
		LoginDeniedProcess();
	}
}

// Login denied process
void KojiKabuto::LoginDeniedProcess() {
	MessageBox(NULL, (session->getErrorMessage()),
			Utils::LoadResourcesString(1001).c_str(), MB_OK | MB_ICONSTOP);

	ForceLogin();
}

// Login success process
void KojiKabuto::LoginSucessProcess() {
	SeyconCommon::debug("Enable task manager\n");
	enableTaskManager(TRUE);

	if (startDisabled())
		MZNStop(MZNC_getUserName());
//	{
//		checkScreenSaver ();
//	}

	destroyProgressWindow();

	sessionStarted = true;
}

// Login status process
/**
 * True if must repeat login process
 *
 */
bool KojiKabuto::LoginStatusProcess(int pLoginResult) {
	SeyconCommon::info("Login result = %d\n", pLoginResult);

	switch (pLoginResult) {
	case LOGIN_DENIED: {
		LoginDeniedProcess();
		return !usedKerberos;
	}

	case LOGIN_ERROR: {
		LoginErrorProcess();
		return false;
	}

	case LOGIN_SUCCESS: {
		LoginSucessProcess();
		return false;
	}

	case LOGIN_UNKNOWNUSER: {
		LoginDeniedProcess();
		return !usedKerberos;
	}

	default:
		return false;
	}
}

// Start manual login
int KojiKabuto::StartManualLogin() {
	LoginDialog loginDialog;			// Login dialog handler
	int loginResult;						// Login dialog process result
	int manualLoginResult = -1;	// Manual login process result

	loginResult = loginDialog.show();

	// Check login dialog result
	if (loginResult == 0) {
		manualLoginResult = session->passwordSessionStartup(
				MZNC_wstrtostr(loginDialog.getUserId().c_str()).c_str(),
				loginDialog.getUserPassword().c_str());
	}

	else {
		ForceLogin();
	}

	return manualLoginResult;
}

// Start manual login
int KojiKabuto::StartSoffidLogin() {
	SoffidDirHandler handler;

	SeyconCommon::warn("Trying soffid dir login");

	std::string user = MZNC_getUserName();
	SeyconCommon::warn("Trying soffid dir login. User %s", user.c_str());

	std::wstring wuser = MZNC_strtowstr(user.c_str());
	std::wstring pass = handler.getPassword(wuser);
	SeyconCommon::warn("Trying soffid dir login. Pass %ls", pass.c_str());

	if (pass.length() == 0)
		return LOGIN_UNKNOWNUSER;

	int loginResult = session->passwordSessionStartup(user.c_str(), pass.c_str());

	return loginResult;
}


// Start kerberos login
int KojiKabuto::StartKerberosLogin() {
	return session->kerberosSessionStartup();
}

static KojiDialog dlg;				// KojiKabuto dialog handler

// Start login process
int KojiKabuto::StartLoginProcess() {
	std::string loginType;	// Login type registry key value
	int result = -1;			// Login result

	session = new SeyconSession();
	session->setDialog(&dlg);

	usedKerberos = false;

	SeyconCommon::readProperty("LoginType", loginType);

	// Check unrecognized login type
	if ((loginType != "kerberos") && (loginType != "manual")
			&& (loginType != "both") &&
			loginType != "soffid")  {
		loginType = "both";
	}

	SeyconCommon::debug("Login type: %s", loginType.c_str());
	// Check login type
	if (! desktopSession && loginType == "soffid") {
		result = StartSoffidLogin();
		usedKerberos = false;
	}

	if (!desktopSession
			&& ((loginType == "kerberos") || (loginType == "both"))) {
		result = StartKerberosLogin();
		usedKerberos = true;
	}

	// Check login status
	if (result != LOGIN_SUCCESS) {
		if (desktopSession || loginType == "manual" || loginType == "both" || loginType == "soffid") {
			result = StartManualLogin();
			usedKerberos = false;
		}
	}

	return result;
}

static BOOL CALLBACK closeAllWindowsStep1(HWND hwnd,	LPARAM lParam) {
	DWORD dwProcessId = GetCurrentProcessId();
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	if (dwProcessId != GetCurrentProcessId())
	{
		if (IsWindowVisible(hwnd) || IsIconic(hwnd))
		{
			bool *allOk = (bool*) lParam;
			DWORD result = SendMessage (hwnd, WM_QUERYENDSESSION, 0 , ENDSESSION_LOGOFF);
			if (allOk != NULL && result == FALSE)
			{
				char ach[1000];
				GetWindowText(hwnd, ach, sizeof ach);
				*allOk = false;
			}
		}
	}
	return true;

}

static BOOL CALLBACK closeAllWindowsStep2(HWND hwnd,	LPARAM lParam) {
	DWORD dwProcessId = GetCurrentProcessId();
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	if (dwProcessId != GetCurrentProcessId())
	{
		if (IsWindowVisible(hwnd) || IsIconic(hwnd))
		{
			SendMessage (hwnd, WM_ENDSESSION, 1, ENDSESSION_LOGOFF);
		}
	}
	return true;

}

static BOOL CALLBACK closeAllWindowsStep3(HWND hwnd,	LPARAM lParam) {
	DWORD dwProcessId = GetCurrentProcessId();
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	if (dwProcessId != GetCurrentProcessId())
	{
		HANDLE h = OpenProcess(GENERIC_ALL, false, dwProcessId);
		TerminateProcess(h, 0);
		CloseHandle(h);
	}
	return true;

}


// Close session
void KojiKabuto::CloseSession() {
	if (desktopSession) {
		bool allOk = true;
		EnumWindows(closeAllWindowsStep1, (LPARAM)&allOk);
		if (! allOk)
			return;
	}

	if (session != NULL)
		session->close();
	session = NULL;

	MZNStop(MZNC_getUserName());

	sessionStarted = false;
	if (desktopSession) {

		EnumWindows(closeAllWindowsStep2, 0);

		HDESK hdesk = GetThreadDesktop(GetCurrentThreadId());
		SwitchDesktop(OpenDesktop("Default", 0, 0, GENERIC_ALL));
		EnumDesktopWindows(hdesk, closeAllWindowsStep3, 0);
		SwitchDesktop(OpenDesktop("Default", 0, 0, GENERIC_ALL));
		ExitProcess(0);
	}
}

// Start user login
void KojiKabuto::StartUserInit() {
	HDESK hd = GetThreadDesktop(GetCurrentThreadId());
	SwitchDesktop(hd);

	runProgram((char*) "userinit.exe", NULL, FALSE);
	enableTaskManager(TRUE);
}

DWORD WINAPI KojiKabuto::mainLoop(LPVOID param) {
	DWORD debugLevel;	// Debug level
	int loginResult = -1;	// Login process result
	std::string forceLogin;	// Force user login registry key value

	debugLevel = SeyconCommon::readIntProperty("debugLevel");

	startingSession = true;

	KojiKabuto::displayProgressMessage(Utils::LoadResourcesString(13).c_str());

	// Check debug level
	if (debugLevel > 0) {
		SeyconCommon::setDebugLevel(debugLevel);
	}

	else {
		SeyconCommon::setDebugLevel(0);
	}

	SeyconCommon::readProperty("ForceStartupLogin", forceLogin);

	// If Remote desktop support is initial program, do not perform ESSO login and go to userinit
	std::string initialProgram;
	SeyconCommon::getCitrixInitialProgram(initialProgram);
	const char * pattern = "%WINDIR%\\SYSTEM32\\RDSADDIN.EXE";

	if (_strnicmp(initialProgram.c_str(), pattern, strlen(pattern)) == 0) {
		KojiKabuto::StartUserInit();
	} else {
		// Check force login before KojiKabuto login
		if (!desktopSession && forceLogin != "true") {
			KojiKabuto::StartUserInit();
		}

		waitForDesktop();
		do {
			// Start login process
			loginResult = KojiKabuto::StartLoginProcess();

			// Check login status
		} while (KojiKabuto::LoginStatusProcess(loginResult));

		if (desktopSession && loginResult != LOGIN_SUCCESS) {
			SwitchDesktop(OpenDesktopA("Default", 0, 0, GENERIC_ALL));
			ExitProcess(0);
		}
		if (desktopSession || forceLogin == "true") {
			KojiKabuto::StartUserInit();
		}


		ProcessDetector pd;
		std::string m = pd.getProcessList();
		if ( initialProgram.length() > 0) {
			CreateThread(NULL, 0, ProcessDetector::mainLoop, NULL, 0, NULL);
		}
	}


	startingSession = false;

	return 0;
}

DWORD WINAPI KojiKabuto::loginThread(LPVOID param) {
	DWORD debugLevel;	// Debug level
	int loginResult = -1;	// Login process result
	std::string forceLogin;	// Force user login registry key value

	debugLevel = SeyconCommon::readIntProperty("debugLevel");

	startingSession = true;

	KojiKabuto::displayProgressMessage(Utils::LoadResourcesString(13).c_str());

	// Check debug level
	if (debugLevel > 0) {
		SeyconCommon::setDebugLevel(debugLevel);
	}

	else {
		SeyconCommon::setDebugLevel(0);
	}

	do {
		// Start login process
		loginResult = KojiKabuto::StartLoginProcess();

		// Check login status

	} while (KojiKabuto::LoginStatusProcess(loginResult));

	KojiKabuto::destroyProgressWindow();

	startingSession = false;

	return 0;
}

static void displayError(const char *header, DWORD error) {
	char* buffer;
	char ach[1000];
	if (FormatMessageA(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0,
			(LPSTR) &buffer, 0, NULL) > 0) {
		sprintf(ach, "%s. Error %d: %s", header, error, buffer);
	} else {
		sprintf(ach, "%s. Error %d", header, error);
	}
	MessageBox(NULL, ach, "Soffid ESSO", MB_OK);
}

static void displayError(const char *header) {
	displayError(header, GetLastError());
}

static bool getUserAndPassword(std::wstring &user, std::wstring &password) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);

	std::string port;
	int portNumber = 0;
	SeyconCommon::readProperty("createUserSocket", port);
	sscanf(port.c_str(), " %d", &portNumber);
	if (portNumber <= 0)
		return false;

	SOCKADDR_IN addrin;
	SOCKET socket_in = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	SeyconCommon::debug("Created socket: %d\n", (int) socket_in);
	if (socket_in < 0) {
		fprintf(stderr, "Error creating socket\n");
		return false;
	}

	addrin.sin_family = AF_INET;
	addrin.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrin.sin_port = htons(portNumber);
	if (connect(socket_in, (LPSOCKADDR) &addrin, sizeof(addrin)) != 0) {
		printf("Error connecting socket\n");
		return false;
	}

	const wchar_t* msg = L"LOCAL_USER\n";

	send(socket_in, (char*) msg, wcslen(msg) * sizeof (wchar_t), 0);

	wchar_t buffer[4096];
	int read = recv(socket_in, (char*) buffer, sizeof buffer, 0);
	if (read <= 0) {
		return false;
	}

	user.clear();
	password.clear();
	int i = 0;
	int max = read / (sizeof(wchar_t));
	while (buffer[i] != '\n' && i < max)
		user += buffer[i++];
	i++;
	if (i >= max)
		return false;

	while (i < max)
		password += buffer[i++];

	return true;
}

static void switchNewDesktop() {
	std::wstring user;
	std::wstring password;
	HANDLE hToken;
	LPVOID lpvEnv;
	DWORD dwSize;
	char ach[1000];
	wchar_t szUserProfile[500];

	printf("Creating desktop\n");
	bool done = false;
	for (int i = 0; i < 3; i++) {
		if (!getUserAndPassword(user, password)) {
			MessageBoxA(NULL,
					"Unable to get local account. Verify Soffid local service is up and running",
					"Soffid ESSO", MB_ICONEXCLAMATION);
			SwitchDesktop(OpenDesktopA("Default", 0, 0, GENERIC_ALL));
			return;
		}

		if (LogonUserW(user.c_str(), NULL, password.c_str(),
				LOGON32_LOGON_INTERACTIVE,
				LOGON32_PROVIDER_DEFAULT, &hToken)) {
			done = true;
			break;
		}
	}

	if (!done) {
		displayError("Error creating users session");
		SwitchDesktop(OpenDesktopA("Default", 0, 0, GENERIC_ALL));
		return;
	}

	lpvEnv = NULL;

	dwSize = sizeof(szUserProfile) / sizeof(WCHAR);

	if (!GetSystemDirectoryW(szUserProfile, dwSize)) {
		wcscpy(szUserProfile, L"c:\\windows\\system32");
	}

	printf("Created desktop %s\n", ach);
	wchar_t achFilename[1000];
	PROCESS_INFORMATION p;
	STARTUPINFOW siw;
	memset(&p, 0, sizeof p);
	memset(&siw, 0, sizeof siw);
	siw.cb = sizeof siw;
	siw.wShowWindow = SW_NORMAL;

	fflush(stdout);

	if (GetModuleFileNameW(NULL, achFilename, sizeof achFilename)) {
		printf("Creating process %ls\n", achFilename);
		fflush(stdout);
		wchar_t achParams[1000];
		wcscpy(achParams, L"newDesktop2");

		HDESK hOld = GetThreadDesktop(GetCurrentThreadId());

		if (CreateProcessWithLogonW(user.c_str(), //LPCWSTR lpUsername
				NULL, //LPCWSTR lpDomain
				password.c_str(), //  LPCWSTR lpPassword
				LOGON_WITH_PROFILE,  // DWORD dwLogonFlags
				achFilename, achParams, // LPCWSTR lpApplicationName, LPWSTR lpCommandLine
				CREATE_UNICODE_ENVIRONMENT, // DWORD dwCreationFlags
				lpvEnv, // Environment,
				szUserProfile, // Current directory
				&siw, // StartupInfo,
				&p) == 0) {
			SetThreadDesktop(hOld);
			DWORD error = GetLastError();
			displayError("Error creating process", error);
			SwitchDesktop(OpenDesktopA("Default", 0, 0, GENERIC_ALL));
		}
	}

	CloseHandle(hToken);
}

static void enableFocusSteal()
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,
			"Control Panel\\Desktop", 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		DWORD dwValue = 0;
		RegSetValueEx(hKey, "ForegroundLockTimeout", 0, REG_DWORD, (LPBYTE) &dwValue,
				sizeof dwValue);
		RegCloseKey(hKey);
	}

}
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInst2,
		LPSTR cmdLine, int nShow) {
	time_t t;
	time(&t);
	srand((int) t);
	hKojiInstance = hInstance;

	bool nowindow = false;
	int numArgs = 0;
	LPCWSTR wCmdLine = GetCommandLineW();
	LPWSTR* args = CommandLineToArgvW(wCmdLine, &numArgs);

	if (wcsicmp(wCmdLine, L"no-window") == 0
			|| numArgs == 2 && wcsicmp(args[1], L"no-window") == 0) {
		nowindow = true;
	} else if (wcsicmp(wCmdLine, L"newDesktop") == 0
			|| numArgs == 2 && wcsicmp(args[1], L"newDesktop") == 0) {
		SwitchDesktop(GetThreadDesktop(GetCurrentThreadId()));
		switchNewDesktop();
		ExitProcess(0);
	} else if (wcsicmp(wCmdLine, L"newDesktop2") == 0
			|| numArgs == 2 && wcsicmp(args[1], L"newDesktop2") == 0) {
		KojiKabuto::desktopSession = true;
	} else if (numArgs > 1) {
		if (wcsicmp(args[1], L"update") == 0) {
			SeyconSession session;
			session.setUser(MZNC_getUserName());
			session.updateMazingerConfig();

			MessageBox(NULL, Utils::LoadResourcesString(14).c_str(),
					Utils::LoadResourcesString(4).c_str(), MB_OK);

			return 0;
		}

		else if (wcsicmp(args[1], L"test-hook") == 0) {
			KojiKabuto::enableTaskManager(false);

			MessageBoxW(NULL,
					MZNC_strtowstr(Utils::LoadResourcesString(15).c_str()).c_str(),
					MZNC_strtowstr(Utils::LoadResourcesString(16).c_str()).c_str(),
					MB_OK);

			KojiKabuto::enableTaskManager(true);

			return 1;
		} else if (wcsicmp(args[1], L"newDesktop2") == 0) {
			KojiKabuto::desktopSession = true;
		} else {
			wchar_t wchMessage[4096];
			swprintf(wchMessage,
					MZNC_strtowstr(Utils::LoadResourcesString(17).c_str()).c_str(),
					args[1]);

			MessageBoxW(NULL, wchMessage,
					MZNC_strtowstr(Utils::LoadResourcesString(16).c_str()).c_str(),
					MB_OK);

			return 1;
		}
	}

	KojiKabuto::enableTaskManager(FALSE);
	enableFocusSteal();

	// Eliminar configuración de DNIe
	RegDeleteKey(HKEY_CURRENT_USER,
			"Software\\Microsoft\\SystemCertificates\\MY\\PhysicalStores\\DNI electrónico");
	std::string logfile;
	SeyconCommon::readProperty("logFile", logfile);

	if (!logfile.empty()) {
		printf("LOGFILE ACTIVE %s\n", logfile.c_str());
		SeyconCommon::setDebugLevel(2);

		if (stdout != NULL) {
			setbuf(stdout, NULL);
			fclose(stdout);
		}

		FILE * f = fopen(logfile.c_str(), "a");
		if (f != NULL)
			setbuf(f, NULL);
	}

	else {
		SeyconCommon::setDebugLevel(0);
	}

	std::string app;
	SeyconCommon::getCitrixInitialProgram(app);
	if (! nowindow && app.length() == 0) {
		KojiKabuto::windowLess = false;
		KojiKabuto::createProgressWindow(hInstance);
		createSystrayWindow(hInstance);
	} else {
		KojiKabuto::windowLess = true;
	}


	if ( KojiKabuto::windowLess) {
		KojiKabuto::mainLoop(NULL);
		while (true) Sleep (60000);
	} else {
		CreateThread(NULL, 0, KojiKabuto::mainLoop, NULL, 0, NULL);

		MSG msg;
		while (GetMessageA(&msg, (HWND) NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		printf("End main loop\n");
	}
	return 0;
}
