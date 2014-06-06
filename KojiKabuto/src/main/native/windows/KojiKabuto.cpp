# include "KojiKabuto.h"

HINSTANCE hKojiInstance;
SeyconSession *KojiKabuto::session = NULL;
HWND hwndLogon = NULL;

extern bool sessionStarted;
extern bool startingSession;

bool KojiKabuto::usedKerberos = false;

HWND createKojiWindow (HINSTANCE hinstance);
void createSystrayWindow (HINSTANCE hInstance);

void KojiKabuto::enableTaskManager (DWORD dwEnable)
{
	if (dwEnable)
		KOJITaskMgrEnable();
	else
		KOJITaskMgrDisable();
}

int KojiKabuto::runProgram (char *achString, char *achDir, int bWait)
{
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
			&p))
	{
		if (bWait)
		{
			MSG msg;
			do
			{
				Sleep(1000);
				while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE) > 0) //Or use an if statement
				{
				          TranslateMessage (&msg);
				          DispatchMessage (&msg);
				}
				GetExitCodeProcess(p.hProcess, &dwExitStatus);
			} while (dwExitStatus == STILL_ACTIVE );

			CloseHandle (p.hProcess);
			CloseHandle (p.hThread);
		}

		return 0;
	}

	else
	{
		MessageBox (NULL, achString, Utils::LoadResourcesString(9).c_str(),
				MB_OK | MB_ICONSTOP);

		return -1;
	}
}

void KojiKabuto::runScript (LPCSTR lpszScript)
{
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

void KojiKabuto::consumeMessages ()
{
//	MSG msg;

//	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
//	{
//		DispatchMessage (&msg);
//	}
}

void KojiKabuto::createProgressWindow (HINSTANCE hInstance)
{
	hwndLogon = createKojiWindow(hInstance);
	return;
}

void KojiKabuto::destroyProgressWindow ()
{
	if (hwndLogon != NULL)
	{
		consumeMessages();
		ShowWindow(hwndLogon, SW_HIDE);
		consumeMessages();
		hwndLogon = NULL;
	}
}

void KojiKabuto::displayProgressMessage (LPCSTR achString)
{
	if (hwndLogon != NULL)
	{
		consumeMessages();
//		SetWindowTextA(hwndLogon, achString);
		SetDlgItemText(hwndLogon, IDC_TEXT, achString);
		consumeMessages();
	}
}

BOOL KojiKabuto::mountScriptDrive ()
{
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

	while (token != NULL && !bMontado)
	{
		if (token[0] != '\0')
		{
			displayProgressMessage(token);
			nr.dwType = RESOURCETYPE_ANY;
			nr.lpLocalName = (char*) localDrive.c_str();
			nr.lpRemoteName = token;
			nr.lpProvider = NULL;
			if (WNetAddConnection2(&nr, NULL, NULL, 0) == NO_ERROR)
			{
				bMontado = TRUE;
			}

			else
			{
				SeyconCommon::warn("Unable to mount drive %s: %s\n", localDrive.c_str(),
						token);
				SeyconCommon::notifyError();
			}
		}

		/* Get next token: */
		token = strtok((char*) NULL, ", ");
	}

	free(achMounts);

	return bMontado;
}

void KojiKabuto::umountScriptDrive ()
{
	std::string localDrive;
	SeyconCommon::readProperty("LogonDrive", localDrive);

	if (localDrive.empty())
		localDrive = "Z:";

	WNetCancelConnection2(localDrive.c_str(), CONNECT_UPDATE_PROFILE, TRUE);
}

void KojiKabuto::writeIntegerProperty (HKEY hKey, const char *name, int value)
{
	char achValue[30];
	sprintf(achValue, "%d", value);
	RegSetValueExA(hKey, name, NULL, REG_SZ, (LPBYTE) achValue, strlen(achValue) + 1);
}

int KojiKabuto::readIntegerProperty (HKEY hKey, const char *name)
{
	char *achValue;
	DWORD size = 0;
	DWORD dwType;
	RegQueryValueEx(hKey, name, NULL, &dwType, (LPBYTE) NULL, &size);

	if (size > 0)
	{
		achValue = (char*) malloc(size + 1);
		RegQueryValueEx(hKey, name, NULL, &dwType, (LPBYTE) achValue, &size);
		achValue[size] = '\0';
		int v = 0;
		sscanf(achValue, " %d", &v);
		free(achValue);
		return v;
	}

	else
	{
		return 0;
	}
}

bool KojiKabuto::startDisabled ()
{
	std::string disabled;
	SeyconCommon::readProperty("startDisabled", disabled);
	return disabled.size() > 0;
}

void KojiKabuto::checkScreenSaver ()
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_ALL_ACCESS,
			&hKey) == ERROR_SUCCESS)
	{
		DWORD dwActive = readIntegerProperty(hKey, "ScreenSaveActive");
		DWORD dwTimeout = readIntegerProperty(hKey, "ScreenSaveTimeOut");
		DWORD dwSecure = readIntegerProperty(hKey, "ScreenSaverIsSecure");
		bool change = false;

		if (!dwActive || !dwSecure)
		{
			int r = MessageBoxA(NULL, Utils::LoadResourcesString(10).c_str(),
					Utils::LoadResourcesString(11).c_str(), MB_YESNO);

			if (r == IDYES)
			{
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

		else if (dwTimeout > 900)
		{
			int r = MessageBoxA(NULL, Utils::LoadResourcesString(12).c_str(),
					Utils::LoadResourcesString(11).c_str(), MB_YESNO);

			if (r == IDYES)
			{
				change = true;
				dwTimeout = 600;
			}
		}

		if (change)
		{
			writeIntegerProperty(hKey, "ScreenSaveActive", dwActive);
			writeIntegerProperty(hKey, "ScreenSaverIsSecure", dwSecure);
			writeIntegerProperty(hKey, "ScreenSaveTimeOut", dwTimeout);
		}
	}
}

// Force login process
void KojiKabuto::ForceLogin ()
{
	std::string forceLogin;	// Force login on startup registry key value

	SeyconCommon::readProperty("ForceStartupLogin", forceLogin);

	destroyProgressWindow();

	// Check force login
	if (forceLogin != "false")
	{
		ExitWindowsEx(EWX_LOGOFF, 0);
	}
}

// Login error process
void KojiKabuto::LoginErrorProcess ()
{
	std::string clientName;			// Client name
	DWORD offlineAllowed = 0;	// Offline login allowed

	destroyProgressWindow();

	SeyconCommon::getCitrixClientName(clientName);

	// Check client name
	if (clientName.empty())
	{
		offlineAllowed = SeyconCommon::readIntProperty("LocalOfflineAllowed");
	}

	else
	{
		offlineAllowed = SeyconCommon::readIntProperty("RemoteOfflineAllowed");
	}

	// Check offline allowed
	if (offlineAllowed)
	{
		MessageBox(NULL, (session->getErrorMessage()),
				Utils::LoadResourcesString(1001).c_str(), MB_OK | MB_ICONWARNING);

		session->executeOfflineScript();
	}

	else
	{
		LoginDeniedProcess();
	}
}

// Login denied process
void KojiKabuto::LoginDeniedProcess ()
{
	MessageBox(NULL, (session->getErrorMessage()),
			Utils::LoadResourcesString(1001).c_str(), MB_OK | MB_ICONSTOP);

	ForceLogin();
}

// Login success process
void KojiKabuto::LoginSucessProcess ()
{
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
bool KojiKabuto::LoginStatusProcess (int pLoginResult)
{
	SeyconCommon::info("Login result = %d\n", pLoginResult);

	switch (pLoginResult)
	{
		case LOGIN_DENIED:
		{
			LoginDeniedProcess();
			return !usedKerberos;
		}

		case LOGIN_ERROR:
		{
			LoginErrorProcess();
			return false;
		}

		case LOGIN_SUCCESS:
		{
			LoginSucessProcess();
			return false;
		}

		case LOGIN_UNKNOWNUSER:
		{
			LoginDeniedProcess();
			return !usedKerberos;
		}

		default:
			return false;
	}
}

// Start manual login
int KojiKabuto::StartManualLogin ()
{
	LoginDialog loginDialog;			// Login dialog handler
	int loginResult;						// Login dialog process result
	int manualLoginResult = -1;	// Manual login process result

	loginResult = loginDialog.show();

	// Check login dialog result
	if (loginResult == 0)
	{
		manualLoginResult = session->passwordSessionStartup(
				MZNC_wstrtostr(loginDialog.getUserId().c_str()).c_str(),
				loginDialog.getUserPassword().c_str());
	}

	else
	{
		ForceLogin();
	}

	return manualLoginResult;
}

// Start kerberos login
int KojiKabuto::StartKerberosLogin ()
{
	return session->kerberosSessionStartup();
}

static KojiDialog dlg;				// KojiKabuto dialog handler

// Start login process
int KojiKabuto::StartLoginProcess ()
{
	std::string loginType;	// Login type registry key value
	int result = -1;			// Login result

	session = new SeyconSession();
	session->setDialog(&dlg);

	usedKerberos = false;

	SeyconCommon::readProperty("LoginType", loginType);

	// Check unrecognized login type
	if ((loginType != "kerberos") && (loginType != "manual") && (loginType != "both"))
	{
		loginType = "both";
	}

	// Check login type
	if ((loginType == "kerberos") || (loginType == "both"))
	{
		result = StartKerberosLogin();
		usedKerberos = true;
	}

	// Check login status
	if (result != LOGIN_SUCCESS)
	{
		if (loginType == "manual" || loginType == "both")
		{
			result = StartManualLogin();
			usedKerberos = false;
		}
	}

	return result;
}

// Close session
void KojiKabuto::CloseSession ()
{
	if (session != NULL)
		session->close();
	session = NULL;

	MZNStop(MZNC_getUserName());

	sessionStarted = false;
}

// Start user login
void KojiKabuto::StartUserInit ()
{
	SeyconCommon::info("Starting userinit\n");

	runProgram((char*) "userinit.exe", NULL, FALSE);
	enableTaskManager(TRUE);
}

DWORD WINAPI KojiKabuto::mainLoop (LPVOID param)
{
	DWORD debugLevel;	// Debug level
	int loginResult = -1;	// Login process result
	std::string forceLogin;	// Force user login registry key value

	debugLevel = SeyconCommon::readIntProperty("debugLevel");

	startingSession = true;

	KojiKabuto::displayProgressMessage(Utils::LoadResourcesString(13).c_str());

	// Check debug level
	if (debugLevel > 0)
	{
		SeyconCommon::setDebugLevel(debugLevel);
	}

	else
	{
		SeyconCommon::setDebugLevel(0);
	}

	SeyconCommon::readProperty("ForceStartupLogin", forceLogin);

	// Check force login before KojiKabuto login
	if (forceLogin != "true")
	{
		KojiKabuto::StartUserInit();
	}

	do
	{
		// Start login process
		loginResult = KojiKabuto::StartLoginProcess();

		// Check login status
	} while (KojiKabuto::LoginStatusProcess(loginResult));

	if (forceLogin == "true")
	{
		KojiKabuto::StartUserInit();
	}

	startingSession = false;

	return 0;
}

DWORD WINAPI KojiKabuto::loginThread (LPVOID param)
{
	DWORD debugLevel;	// Debug level
	int loginResult = -1;	// Login process result
	std::string forceLogin;	// Force user login registry key value

	debugLevel = SeyconCommon::readIntProperty("debugLevel");

	startingSession = true;

	KojiKabuto::displayProgressMessage(Utils::LoadResourcesString(13).c_str());

	// Check debug level
	if (debugLevel > 0)
	{
		SeyconCommon::setDebugLevel(debugLevel);
	}

	else
	{
		SeyconCommon::setDebugLevel(0);
	}

	do
	{
		// Start login process
		loginResult = KojiKabuto::StartLoginProcess();

		// Check login status

	} while (KojiKabuto::LoginStatusProcess(loginResult));

	KojiKabuto::destroyProgressWindow();

	startingSession = false;

	return 0;
}

extern "C" int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hInst2, LPSTR cmdLine,
		int nShow)
{
	time_t t;
	time(&t);
	srand((int) t);
	hKojiInstance = hInstance;

	int numArgs = 0;
	LPCWSTR wCmdLine = GetCommandLineW();
	LPWSTR* args = CommandLineToArgvW(wCmdLine, &numArgs);

	if (numArgs > 1)
	{
		if (wcsicmp(args[1], L"update") == 0)
		{
			SeyconSession session;
			session.setUser(MZNC_getUserName());
			session.updateMazingerConfig();

			MessageBox(NULL, Utils::LoadResourcesString(14).c_str(),
					Utils::LoadResourcesString(4).c_str(), MB_OK);

			return 0;
		}

		else if (wcsicmp(args[1], L"test-hook") == 0)
		{
			KojiKabuto::enableTaskManager(false);

			MessageBoxW(NULL, MZNC_strtowstr(Utils::LoadResourcesString(15).c_str()).c_str(),
					MZNC_strtowstr(Utils::LoadResourcesString(16).c_str()).c_str(), MB_OK);

			KojiKabuto::enableTaskManager(true);

			return 1;
		}

		else
		{
			wchar_t wchMessage[4096];
			swprintf(wchMessage, MZNC_strtowstr(Utils::LoadResourcesString(17).c_str()).c_str(),
					args[1]);

			MessageBoxW(NULL, wchMessage,
					MZNC_strtowstr(Utils::LoadResourcesString(16).c_str()).c_str(), MB_OK);

			return 1;
		}
	}

	KojiKabuto::enableTaskManager(FALSE);

	// Eliminar configuración de DNIe
	RegDeleteKey(HKEY_CURRENT_USER,
			"Software\\Microsoft\\SystemCertificates\\MY\\PhysicalStores\\DNI electrónico");
	std::string logfile;
	SeyconCommon::readProperty("logFile", logfile);

	if (!logfile.empty())
	{
		printf("LOGFILE ACTIVE %s\n", logfile.c_str());
		SeyconCommon::setDebugLevel(2);

		if (stdout!= NULL)
		{
			setbuf(stdout, NULL);
			fclose(stdout);
		}

		FILE * f = fopen(logfile.c_str(), "a");
		if (f != NULL)
			setbuf(f, NULL);
	}

	else
	{
		SeyconCommon::setDebugLevel(0);
	}

	KojiKabuto::createProgressWindow(hInstance);
	createSystrayWindow(hInstance);

	HDESK hdesk = OpenDesktopA("Default", 0, FALSE, GENERIC_ALL);

	if (hdesk == NULL)
	{
		SeyconCommon::debug("Cannot open desktop Default");
	}

	else
	{
		if (SwitchDesktop(hdesk))
		{
			SeyconCommon::debug("Switch desktop Default SUCCESS");
		}

		else
		{
			SeyconCommon::debug("Switch desktop Default FAILURE");
		}
	}

	CreateThread(NULL, 0, KojiKabuto::mainLoop, NULL, 0, NULL);

	MSG msg;
	while (GetMessageA(&msg, (HWND) NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	printf("End main loop\n");

	return 0;
}
