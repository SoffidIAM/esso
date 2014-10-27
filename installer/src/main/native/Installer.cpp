#include <stdio.h>
#include <tchar.h>
#include <io.h>
#include <ctype.h>
#include <time.h>
#include <accctrl.h>
#include <aclapi.h>
#include <MazingerGuid.h>
#include <string>
#include <shellapi.h>
#include <shlobj.h>
#include <shlguid.h>
#include <stdlib.h>
#include <winhttp.h>

#include "Installer.h"
#include "resource.h"
#include "bz/bzlib.h"

void disableProgressWindow();
void setProgressMessage(const char *lpszMessage, ...);

#define QUOTEME(x) #x
#define STR(macro) QUOTEME(macro)
#define MAZINGER_VERSION_STR STR(MAZINGER_VERSION)

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
static const std::string DEF_WINLOGON_KEY_PATH =
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static const std::string DEF_REGISTRY_FOLDER = "SOFTWARE\\Soffid\\esso";
static const std::string DEF_CONFIG_TOOL_NAME = "SoffidConfig.exe";
static const std::string DEF_ROOT_START_MENU = "\\Soffid ESSO";
static const std::string DEF_CONFIG_SHORTCUT = DEF_ROOT_START_MENU
		+ " Configuration.lnk";
static const std::string DEF_WINLOGON_DLL = "msgina.dll";
static const char* DEF_DEFAULT_SERVERS = "";

bool anyError = false;
bool quiet = false;
bool reboot = false;
bool isUpdate = false;
bool noGina = false;

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.
	if (fnIsWow64Process == NULL)
		fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
				GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			//handle error
		}
	}

	return bIsWow64;
}

BOOL IsWindowsXP()
{
	DWORD dwVersion = 0;
	WORD dwMajorVersion = 0;
	WORD dwMinorVersion = 0;

	dwVersion = GetVersion();

	// Get the Windows version.
	dwMajorVersion = (LOBYTE(LOWORD(dwVersion)) );
	dwMinorVersion = (HIBYTE(LOWORD(dwVersion)) );

	return (dwMajorVersion <= 5);
}

static DWORD Wow64Key(DWORD flag)
{
	return flag | (IsWow64() ? KEY_WOW64_64KEY : 0);
}

LPCSTR getMazingerDir()
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, Wow64Key(KEY_READ),
			&hKey) == ERROR_SUCCESS)
	{
		static char achPath[4096] = "XXXX";
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		size = sizeof achPath;
		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType, (LPBYTE) achPath,
				&size);
		RegCloseKey(hKey);
		strcat(achPath, "\\SoffidESSO");

		return achPath;
	}

	else
	{
		return FALSE;
	}
}

void log(const char *szFormat, ...)
{
	static FILE *logFile = NULL;
	char achFile[1024];

	if (logFile == NULL)
	{
		GetSystemDirectoryA(achFile, 1024);
		strcat(achFile, "\\mazinger-install.log");
		logFile = fopen(achFile, "a");
	}

	if (logFile != NULL)
	{
		setbuf(logFile, NULL);
		time_t t;
		time(&t);
		struct tm *tm = localtime(&t);
		fprintf(logFile, "%d-%02d-%04d %02d:%02d:%02d INSTALL ", tm->tm_mday,
				tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min,
				tm->tm_sec);
		va_list v;
		va_start(v, szFormat);
		vfprintf(logFile, szFormat, v);
		va_end(v);

		if (szFormat[strlen(szFormat) - 1] != '\n')
			fprintf(logFile, "\n");

#if 0
		printf("%d-%02d-%04d %02d:%02d:%02d INSTALL ", tm->tm_mday,
				tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min,
				tm->tm_sec);
		va_start(v, szFormat);
		vprintf(szFormat, v);
		va_end(v);

		if (szFormat[strlen(szFormat) - 1] != '\n')
			printf("\n");
#endif
	}
}

BOOL HelperWriteKey(int bits, HKEY roothk, const char *lpSubKey, LPCTSTR val_name,
		DWORD dwType, const void *lpvData, DWORD dwDataSize)
{
	if (dwDataSize == (DWORD) -1)
		dwDataSize = lstrlen ((const char*) lpvData);
	//
	//Helper function for doing the registry write operations
	//
	//roothk:either of HKCR, HKLM, etc

	//lpSubKey: the key relative to 'roothk'

	//val_name:the key value name where the data will be written

	//dwType:the type of data that will be written ,REG_SZ,REG_BINARY, etc.

	//lpvData:a pointer to the data buffer

	//dwDataSize:the size of the data pointed to by lpvData
	//
	//

	// Registrar en 64 bits
	if (IsWow64() && (bits == 0 || bits == 64))
	{
		HKEY hk;
		if (ERROR_SUCCESS
				!= RegCreateKeyEx(roothk, lpSubKey, 0, (LPSTR) "",
						REG_OPTION_NON_VOLATILE, Wow64Key(KEY_ALL_ACCESS), NULL,
						&hk, NULL))
		{
			log("> ERROR Cannot create 64 bits key %s", lpSubKey);
			anyError = true;

			return FALSE;
		}

		if (dwDataSize >0 &&
			ERROR_SUCCESS != RegSetValueEx(hk, val_name, 0, dwType, (CONST BYTE *) lpvData,
						dwDataSize))
		{
			log("> ERROR Cannot set 64 bits value %s\\%s", lpSubKey, val_name);
			anyError = true;

			return FALSE;
		}

		if (ERROR_SUCCESS != RegCloseKey(hk))
			return FALSE;
	}

	// Registrar en 32 bits
	if (bits == 0 || bits == 32)
	{
		HKEY hk;
		if (ERROR_SUCCESS
				!= RegCreateKeyEx(roothk, lpSubKey, 0, (LPSTR) "",
						REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, NULL))
		{
			anyError = true;
			log("> ERROR Cannot create key %s", lpSubKey);

			return FALSE;
		}

		if (dwDataSize > 0 &&
			 ERROR_SUCCESS != RegSetValueEx(hk, val_name, 0, dwType, (CONST BYTE *) lpvData,
						dwDataSize))
		{
			anyError = true;
			log("> ERROR Cannot set value %s\\%s", lpSubKey, val_name);

			return FALSE;
		}

		if (ERROR_SUCCESS != RegCloseKey(hk))
			return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void notifyError()
{
	LPSTR pstr;
	char errorMsg[255];
	DWORD lastError = GetLastError();

	int fm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, 	NULL, lastError, 0, (LPSTR) &pstr, 0, NULL);

	printf("***************\n");
	printf("    ERROR      \n");
	printf("\n");

	// Format message failure
	if (fm == 0)
	{
		sprintf(errorMsg, "Unknown error: %d\n", lastError);
		printf(errorMsg);
		log(">> %s", errorMsg);
	}

	else
	{
		printf("%s\n", pstr);
		log(">> %s", pstr);
	}

	printf("***************\n");

	if (!quiet)
	{
		disableProgressWindow();
		MessageBox(NULL, pstr, "Installation error", MB_OK | MB_ICONEXCLAMATION);
	}

	LocalFree(pstr);
}

bool replaceOnReboot(const char *szTarget, const char *szSource)
{
	HKEY hKey;
	const char *entryName = "SayakaPendingFileRenameOperations";

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0,
			Wow64Key(KEY_ALL_ACCESS), &hKey) == ERROR_SUCCESS)
	{
		static char achPath[2];
		DWORD dwType;
		DWORD size = 0;
		char *buffer;
		DWORD len = 0;

		if (ERROR_FILE_NOT_FOUND
				!= RegQueryValueEx(hKey, entryName, NULL, &dwType,
						(LPBYTE) achPath, &size))
		{
			len = size + strlen(szTarget) + strlen(szSource) + 2;
			buffer = (char*) malloc(len);

			if (ERROR_SUCCESS
					!= RegQueryValueEx(hKey, entryName, NULL, &dwType,
							(LPBYTE) buffer, &size))
			{
				printf("..ERROR\n", __LINE__);
				log("Error accessing to %s", entryName);
				notifyError();
				free(buffer);
				return false;
			}
		}

		else
		{
			len = strlen(szTarget) + strlen(szSource) + 3;
			buffer = (char*) malloc(len);
			size = 1;
		}

		strcpy(&buffer[size - 1], szTarget);
		strcpy(&buffer[size + strlen(szTarget)], szSource);
		buffer[len - 1] = '\0';
		dwType = REG_BINARY;

		if (ERROR_SUCCESS
				!= RegSetValueEx(hKey, entryName, 0, dwType,
						(CONST BYTE *) buffer, len))
		{
			log("> ERROR Cannot set value %s", entryName);
			notifyError();
			return FALSE;
		}

		free(buffer);
		RegCloseKey(hKey);
		return true;
	}

	else
	{
		log(
				"> ERROR Cannot open key HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager");

		return false;
	}
}

/////////////////////////////////////////////////
PSECURITY_DESCRIPTOR createSecurityDescriptor(DWORD everyOnePermission)
{
	PSECURITY_DESCRIPTOR pSD;
	PSID pEveryoneSID = NULL;
	PSID pAdminSID = NULL;
	PACL pAcl = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = { SECURITY_WORLD_SID_AUTHORITY };
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
	EXPLICIT_ACCESS ea[2];
	DWORD dwRes;

	/* Initialize a security descriptor. */
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); /* defined in WINNT.H */

	if (pSD == NULL)
	{
		goto Cleanup;
	}

	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
	{ /* defined in WINNT.H */
		//              ErrorHandler("InitializeSecurityDescriptor");
		goto Cleanup;
	}

	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0,
			0, 0, 0, 0, &pEveryoneSID))
	{
		printf("AllocateAndInitializeSid Error %d\n", (int) GetLastError());
		goto Cleanup;
	}

	ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
	ea[0].grfAccessPermissions = everyOnePermission;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName = (LPTSTR) pEveryoneSID;

	if (!AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSID))
	{
		goto Cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to
	// the key.
	ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName = (LPTSTR) pAdminSID;

	dwRes = SetEntriesInAcl(2, ea, NULL, &pAcl);
	if (dwRes != ERROR_SUCCESS)
		goto Cleanup;

	/* Add a NULL disc. ACL to the security descriptor. */
	if (!SetSecurityDescriptorDacl(pSD, TRUE, /* specifying a disc. ACL  */
	pAcl, FALSE))
	{ /* not a default disc. ACL */
		//              ErrorHandler("SetSecurityDescriptorDacl");
		goto Cleanup;
	}

	return pSD;
	/* Add the security descriptor to the file. */
	Cleanup: notifyError();
	return NULL;
}

void createCacheDir()
{
	std::string cacheDir = getMazingerDir();
	cacheDir += "\\Cache";

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor =
			createSecurityDescriptor(
					FILE_GENERIC_EXECUTE | FILE_GENERIC_READ | FILE_GENERIC_WRITE);
	sa.bInheritHandle = FALSE;

	CreateDirectoryA(cacheDir.c_str(), &sa);
}

void doRegisterIEHook(int bits, const char *achDllPath)
{
	WCHAR *lpwszClsid;
	char szBuff[MAX_PATH] = "";
	char szClsid[MAX_PATH] = "", szInproc[MAX_PATH] = "", szProgId[MAX_PATH];
	char szDescriptionVal[256] = "";

	log("Registered Internet Explorer (%d bits)", bits);

	StringFromCLSID(CLSID_Mazinger, &lpwszClsid);

	wsprintf(szClsid, "%S", lpwszClsid);
	wsprintf(szInproc, "%s\\%s\\%s", "clsid", szClsid, "InprocServer32");
	wsprintf(szProgId, "%s\\%s\\%s", "clsid", szClsid, "ProgId");

	//
	//write the default value
	//
	wsprintf(szBuff, "%s", "Soffid ESSO for Internet Explorer");
	wsprintf(szDescriptionVal, "%s\\%s", "clsid", szClsid);

	HelperWriteKey(bits, HKEY_CLASSES_ROOT, szDescriptionVal, NULL, //write to the "default" value
			REG_SZ, (void*) szBuff, lstrlen(szBuff));

	//
	//write the "InprocServer32" key data
	//
	HelperWriteKey(bits, HKEY_CLASSES_ROOT, szInproc, NULL, //write to the "default" value
			REG_SZ, (void*) achDllPath, lstrlen(achDllPath));

	//
	//write the "ProgId" key data under HKCR\clsid\{---}\ProgId
	//
	lstrcpy(szBuff, MznIEProgId);
	HelperWriteKey(bits, HKEY_CLASSES_ROOT, szProgId, NULL, REG_SZ,
			(void*) szBuff, lstrlen(szBuff));

	//
	//write the "ProgId" data under HKCR\CodeGuru.FastAddition
	//
	wsprintf(szBuff, "%s", "Soffid ESSO Hook for Internet Explorer");
	HelperWriteKey(bits, HKEY_CLASSES_ROOT, MznIEProgId, NULL, REG_SZ,
			(void*) szBuff, lstrlen(szBuff));

	wsprintf(szProgId, "%s\\%s", MznIEProgId, "CLSID");
	HelperWriteKey(bits, HKEY_CLASSES_ROOT, szProgId, NULL, REG_SZ,
			(void*) szClsid, lstrlen(szClsid));

	wsprintf(szBuff,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\%s",
			szClsid);
	DWORD dw = 1;
	strcpy(szDescriptionVal, "SoffidESSOPlugin");
	HelperWriteKey(bits, HKEY_LOCAL_MACHINE, szBuff, "Name", REG_SZ,
			(void*) szDescriptionVal, strlen(szDescriptionVal));
	HelperWriteKey(bits, HKEY_LOCAL_MACHINE, szBuff, "NoExplorer", REG_DWORD,
			(void*) &dw, sizeof dw);
}

void registerIEHook()
{
	char achDllPath[4096];
	strcpy(achDllPath, getMazingerDir());
	strcat(achDllPath, "\\AfroditaE.dll");

	if (IsWow64())
	{
		doRegisterIEHook(64, achDllPath);
		strcpy(achDllPath, getMazingerDir());
		strcat(achDllPath, "\\AfroditaE32.dll");
		doRegisterIEHook(32, achDllPath);
	}

	else
	{
		doRegisterIEHook(32, achDllPath);
	}
}

void registerFFHook()
{
	char szBuff[4096];
	//
	//write the default value
	//
	wsprintf(szBuff, "%s\\FFExtension", getMazingerDir());

	log("Registering Firefox extension");
	HelperWriteKey(32, HKEY_LOCAL_MACHINE,
			"Software\\Mozilla\\Firefox\\Extensions",
			"{df382936-f24b-11df-96e1-9bf54f13e327}", REG_SZ, (void*) szBuff,
			lstrlen(szBuff));
}

void registerChromePlugin()
{
	char szBuff[4096];
	//
	//write the default value
	//

	log("Registering Firefox extension");

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins",
			NULL, REG_SZ, NULL, 0);

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins\\@soffid.com/SoffidPlugin",
			"Description", REG_SZ, (void*) "Soffid ESSO Module", -1);

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins\\@soffid.com/SoffidPlugin",
			"ProductName", REG_SZ, (void*) "Soffid ESSO", -1);

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins\\@soffid.com/SoffidPlugin",
			"Vendor", REG_SZ, (void*) "Soffid", -1);

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins\\@soffid.com/SoffidPlugin",
			"Version", REG_SZ, (void*) MAZINGER_VERSION_STR, -1);

	// Register DLL

	wsprintf(szBuff, "%s\\AfroditaC.dll", getMazingerDir());
	HelperWriteKey(32, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins\\@soffid.com/SoffidPlugin",
			"Path", REG_SZ, (void*) szBuff, -1);

	wsprintf(szBuff, "%s\\AfroditaC64.dll", getMazingerDir());

	HelperWriteKey(64, HKEY_LOCAL_MACHINE,
			"Software\\MozillaPlugins\\@soffid.com/SoffidPlugin",
			"Path", REG_SZ, (void*) szBuff, -1);
	// Register extension
		HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\Google\\Chrome\\Extensions\\gmipnihabpcggdnlbbncgleblgjapgkm",
			"update_url", REG_SZ, (void*) "https://clients2.google.com/service/update2/crx", -1);

}

void registerJetScrander()
{
	char achExePath[4096];
	strcpy(achExePath, getMazingerDir());
	strcat(achExePath, "\\JetScrander.exe");

	WCHAR *lpwszClsid;
	char szBuff[MAX_PATH] = "";
	char szProgId[MAX_PATH];
	char szDescriptionVal[256] = "";

	log("Registering Jet Scrander");

	StringFromCLSID(CLSID_Mazinger, &lpwszClsid);

	wsprintf(szProgId, "%s", "SoffidESSO.JetScrander.1");

	//
	//write the default value
	//
	wsprintf(szBuff, "%s", "Soffid ESSO application launcher");
	wsprintf(szDescriptionVal, "%s", szProgId);

	HelperWriteKey(0, HKEY_CLASSES_ROOT, szDescriptionVal, NULL, //write to the "default" value
			REG_SZ, (void*) szBuff, lstrlen(szBuff));

	DWORD dwFlags = 128;
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szDescriptionVal, "Flags", //write to the flgas value
			REG_DWORD, (void*) &dwFlags, sizeof dwFlags);

	//
	//write the shell\open\command
	//
	wsprintf(szDescriptionVal, "%s\\shell\\open\\command", szProgId);
	wsprintf(szBuff, "\"%s\" \"%%1\"", achExePath);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szDescriptionVal, NULL, //write to the "default" value
			REG_SZ, (void*) szBuff, lstrlen(szBuff));

	//
	//write the .jsm entry
	//
	wsprintf(szDescriptionVal, "%s", ".jsm");
	wsprintf(szBuff, "%s", szProgId);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szDescriptionVal, NULL, //write to the "default" value
			REG_SZ, (void*) szBuff, lstrlen(szBuff));
}

void registerMazingerMsg()
{
	HKEY hKey;

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Mazinger",
			0, (LPSTR) "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,
			NULL) == ERROR_SUCCESS)
	{
		DWORD dwValue = 1;
		RegSetValueEx(hKey, "CategoryCount", 0, REG_DWORD, (LPBYTE) &dwValue,
				sizeof dwValue);
		std::string msg = getMazingerDir();
		msg.append("\\mazinger.exe");
		RegSetValueEx(hKey, "CategoryMessageFile", 0, REG_SZ,
				(LPBYTE) msg.c_str(), msg.size());
		RegSetValueEx(hKey, "EventMessageFile", 0, REG_SZ, (LPBYTE) msg.c_str(),
				msg.size());
		dwValue = EVENTLOG_AUDIT_SUCCESS | EVENTLOG_ERROR_TYPE
				| EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE;
		RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (LPBYTE) &dwValue,
				sizeof dwValue);
		RegCloseKey(hKey);
		log("Registered Event log");
	}
}

void registerBoss()
{
	log("Registering BOSS...\n");
	std::string cmd = "\"";
	cmd += getMazingerDir();
	cmd += "\\boss.exe\" \"%1\"";
	HelperWriteKey(0, HKEY_CLASSES_ROOT,
			"exefile\\shell\\Run with administrator permission\\command", NULL,
			REG_SZ, cmd.c_str(), cmd.length());

	log("Register menu handler on %s", cmd.c_str());

	TCHAR szPath[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPath(NULL,
					CSIDL_COMMON_PROGRAMS | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
	{
		std::string dir = szPath;
		dir += "\\Soffid ESSO";
		mkdir(dir.c_str());

		dir += "\\Administrator shell.lnk";

		IShellLinkA* pShellLink;

		CoInitialize(NULL);
		HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL,
				IID_IShellLinkA, (void**) &pShellLink);

		if (FAILED(hr))
		{
			log("> Error creating shell instance");
			return;
		}

		cmd = getMazingerDir();
		cmd += "\\boss.exe";

		std::string icon = "\"";
		icon += cmd;
		icon += "\"";
		pShellLink->SetPath(cmd.c_str()); // Path to the object we are referring to
		pShellLink->SetArguments("cmd.exe"); // Path to the object we are referring to
		pShellLink->SetDescription("Start command line as local administrator");
		pShellLink->SetIconLocation(cmd.c_str(), 0);

		IPersistFile *pPersistFile;

		pShellLink->QueryInterface(IID_IPersistFile, (void**) &pPersistFile);

		std::wstring wfullName;
		wchar_t wsFileName[4096];
		mbstowcs(wsFileName, dir.c_str(), 4095);
		pPersistFile->Save(wsFileName, TRUE);
		pPersistFile->Release();
		pShellLink->Release();

		log("Creted menu entry %ls", wsFileName);
	}

	else
	{
		log("> Error looking up for programs folder");
	}
}

/** @brief Soffid ESSO start menu folder
 *
 * Implements the functionality to add the Soffid ESSO programs folder
 * on windows start menu.
 */
void CreateStartMenuFolder()
{
	std::string cmd = "\"";			// Command executable
	std::string folderPath;			// Start menu folder
	std::string icon;					// Icon of link
	std::wstring wfullName;		// Full name
	TCHAR szPath[MAX_PATH];	// Common programs folder
	IShellLinkA* pShellLink;		// Shell link
	HRESULT hr;							// Create instance result

	log("Creating Soffid ESSO start menu folder...\n");

	// Get start menu programs folder
	if (SUCCEEDED(SHGetFolderPath(NULL,
					CSIDL_COMMON_PROGRAMS | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
	{
		folderPath = szPath;
		folderPath += DEF_ROOT_START_MENU;
		mkdir(folderPath.c_str());

		folderPath += DEF_CONFIG_SHORTCUT;

		CoInitialize(NULL);

		hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLinkA,
				(void**) &pShellLink);

		if (FAILED(hr))
		{
			log("> Error creating shell instance");

			return;
		}

		cmd = getMazingerDir();
		cmd += "\\";
		cmd += DEF_CONFIG_TOOL_NAME;

		icon = "\"";
		icon += cmd;
		icon += "\"";

		pShellLink->SetPath(cmd.c_str());// Path to the object we are referring to
		pShellLink->SetArguments("cmd.exe");	// Command to execute
		pShellLink->SetDescription("Start Soffid ESSO configuration tool");
		pShellLink->SetIconLocation(cmd.c_str(), 0);

		IPersistFile *pPersistFile;

		pShellLink->QueryInterface(IID_IPersistFile, (void**) &pPersistFile);

		wchar_t wsFileName[4096];
		mbstowcs(wsFileName, folderPath.c_str(), 4095);

		pPersistFile->Save(wsFileName, TRUE);
		pPersistFile->Release();
		pShellLink->Release();

		log("Creted menu entry %ls", wsFileName);
	}

	else
	{
		log("> Error looking up for programs folder");
	}
}


IDispatch *automationCreateObject(OLECHAR *name)
{
	HRESULT hr;
	CLSID clsidHNetCfg;
	hr = CLSIDFromProgID(name, &clsidHNetCfg);
	if (FAILED(hr))
	{
		log("> Error locating firewall service");
		return NULL;
	}

	IDispatch *result;
	hr = CoCreateInstance(clsidHNetCfg, NULL, CLSCTX_ALL, IID_IDispatch,
			(void**) &result);
	if (FAILED(hr))
	{
		log("> Error creating %s instance", name);
		return NULL;
	}

	return result;
}

HRESULT automationPutAttribute (IDispatch *pDispatch, const OLECHAR *name, VARIANT *value)
{
	DISPPARAMS dispParams;
	HRESULT hr;
	DISPID dispid;
	unsigned int argError;
	VARIANT result;
	DISPID mydispid = DISPID_PROPERTYPUT;

	log ("Setting value for attribute %ls" ,name);


	hr = pDispatch ->GetIDsOfNames(IID_NULL, (OLECHAR**) &name, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
	if (FAILED(hr))
	{
		log ("Failed to find attribute %ls" ,name);
		return hr;
	}

	dispParams.cArgs = 1;
	dispParams.cNamedArgs = 1;
	dispParams.rgvarg = value;
	dispParams.rgdispidNamedArgs = &mydispid;

	hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams, &result, NULL, &argError);
	if (FAILED(hr))
		log ("Failed to put value for attribute %ls (%lx)" ,name, hr);
	else
		log ("Set value for attribute %ls" ,name);
	return hr;

}

HRESULT automationInvoke (IDispatch *pDispatch, const OLECHAR *name, IDispatch *pDispatchParam)
{
	DISPPARAMS dispParams;
	HRESULT hr;
	DISPID dispid;
	unsigned int argError;
	VARIANT result;

	hr = pDispatch ->GetIDsOfNames(IID_NULL,(OLECHAR**) &name, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
	if (FAILED(hr))
	{
		log ("Failed to find attribute %ls" ,name);
		return hr;
	}

	VARIANT param;
	param.vt = VT_DISPATCH;
	param.pdispVal = pDispatchParam;
	dispParams.cArgs = 1;
	dispParams.cNamedArgs = 0;
	dispParams.rgvarg = &param;

	hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispParams, &result, NULL, &argError);
	if (FAILED(hr))
		log ("Failed to call %ls" ,name);
	return hr;

}


IDispatch* automationGetProfileByType (IDispatch *pDispatch, int type)
{
	DISPPARAMS dispParams;
	HRESULT hr;
	DISPID dispid;
	unsigned int argError;
	VARIANT result;
	VariantInit(&result);
	const OLECHAR* name= L"GetProfileByType";

	hr = pDispatch ->GetIDsOfNames(IID_NULL,(OLECHAR**) &name, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
	if (FAILED(hr))
	{
		log ("Failed to find method %ls" ,name);
		return NULL;
	}

	IDispatch *pResult = pDispatch;
	VARIANTARG param[3];
	param[0].vt = VT_INT;
	param[0].iVal = type;
	dispParams.cArgs = 1;
	dispParams.cNamedArgs = 0;
	dispParams.rgvarg = param;
	dispParams.rgdispidNamedArgs = NULL;

	hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispParams, &result, NULL, &argError);
	if (FAILED(hr))
	{
		log ("Failed to call %ls (%lx)" ,name , (long) hr);
		return NULL;
	}

	if (result.vt != VT_DISPATCH)
	{
		log("> Result from %ls is not a dispatch", name);
		return NULL;
	}
	return result.pdispVal;

}


HRESULT automationPutStringAttribute (IDispatch *pDispatch, const OLECHAR *name, wchar_t *value)
{
	VARIANT varValue;
	varValue.vt = VT_BSTR;
	varValue.bstrVal = SysAllocString(value);

	HRESULT hr = automationPutAttribute(pDispatch, name, &varValue);
	SysFreeString(varValue.bstrVal);

	return hr;
}

HRESULT automationPutIntAttribute (IDispatch *pDispatch, const OLECHAR *name, int value)
{
	VARIANT varValue;
	varValue.vt = VT_INT;
	varValue.intVal = value;

	return automationPutAttribute(pDispatch, name, &varValue);
}

HRESULT automationPutBooleanAttribute (IDispatch *pDispatch, const OLECHAR *name, bool value)
{
	VARIANT varValue;
	varValue.vt = VT_INT;
	varValue.intVal = value ? 1: 0;

	return automationPutAttribute(pDispatch, name, &varValue);
}

HRESULT automationGetAttribute (IDispatch *pDispatch, const OLECHAR *name, VARIANT *result)
{
	DISPPARAMS dispParams;
	HRESULT hr;
	DISPID dispid;
	unsigned int argError;

	hr = pDispatch ->GetIDsOfNames(IID_NULL, (OLECHAR**)&name, 1, LOCALE_SYSTEM_DEFAULT, &dispid );
	if (FAILED(hr))
	{
		log ("> Unknown attribute %ls", name);
		return hr;
	}

	dispParams.cArgs = 0;
	dispParams.cNamedArgs = 0;

	hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispParams, result, NULL, &argError);
	return hr;

}

IDispatch* automationGetDispatchAttribute (IDispatch *pDispatch, const OLECHAR *name)
{
	VARIANT v;
	HRESULT hr;
	hr = automationGetAttribute(pDispatch, name, &v);
	if (FAILED(hr))
	{
		log("> Error getting attribute %ls (%lx)", name);
		return NULL;
	}
	if (v.vt != VT_DISPATCH)
	{
		log("> Result from %ls is not a dispatch", name);
		return NULL;
	}
	else
		return v.pdispVal;

}

typedef enum NET_FW_PROFILE_TYPE_ {
  NET_FW_PROFILE_DOMAIN,
  NET_FW_PROFILE_STANDARD,
  NET_FW_PROFILE_CURRENT,
  NET_FW_PROFILE_TYPE_MAX
} NET_FW_PROFILE_TYPE;

/** @brief Register firewall rules
 *
 * Enables KojiKabuto to to connect and to listen to sockets
 */
void ConfigureFirewall()
{
	HRESULT hr;							// Create instance result
	VARIANT varResult;
	unsigned int argError;


	CoInitialize(NULL);


	IDispatch* pHNetCfg = automationCreateObject(L"HNetCfg.FwMgr");
	if (pHNetCfg != NULL)
	{
		IDispatch *localPolicy = automationGetDispatchAttribute (pHNetCfg, L"LocalPolicy");
		if (localPolicy != NULL)
		{
			for ( int i = -1; i <= 4; i++)
			{
				log ("Testing profile %d", i);
				IDispatch *currentProfile ;
				if (i < 0)
					currentProfile = automationGetDispatchAttribute(localPolicy, L"CurrentProfile");
				else
					currentProfile = automationGetProfileByType(localPolicy, i);
				if (currentProfile != NULL)
				{
					log ("Found profile %d", i);
					char achNewPath[4096];
					wchar_t wchNewPath[4096];
					strcpy(achNewPath, getMazingerDir());
					strcat(achNewPath, "\\KojiKabuto.exe");
					mbstowcs(wchNewPath, achNewPath, 4095);

					IDispatch *authorizedapplication = automationCreateObject (L"HNetCfg.FwAuthorizedApplication");
					if (authorizedapplication != NULL)
					{
						hr = automationPutStringAttribute(authorizedapplication, L"ProcessImageFilename", wchNewPath);
						if (FAILED(hr))
							return;
						hr = automationPutStringAttribute(authorizedapplication, L"Name", L"Soffid ESSO Session Handler");
						if (FAILED(hr))
							return;
						hr = automationPutIntAttribute(authorizedapplication, L"Scope", 0); // Scope ALL
						if (FAILED(hr))
							return;
						hr = automationPutIntAttribute(authorizedapplication, L"IpVersion", 2); // ip version any
						if (FAILED(hr))
							return;
						hr = automationPutBooleanAttribute(authorizedapplication, L"Enabled", true); // ip version any
						if (FAILED(hr))
							return;
						IDispatch * authorizedApplications = automationGetDispatchAttribute(currentProfile, L"AuthorizedApplications");
						if (authorizedApplications != NULL)
						{
							automationInvoke (authorizedApplications, L"Add", authorizedapplication);
							log ("Firewal is configured");
							authorizedApplications -> Release();
						}
						authorizedapplication->Release();
					}
					currentProfile -> Release();
				}
			}
			localPolicy->Release();
		}
		pHNetCfg -> Release();
	}
}

IDispatch* createRule (int direction)
{
	HRESULT hr;

	IDispatch* pFwRule = automationCreateObject(L"HNetCfg.FWRule");
	if (pFwRule != NULL)
	{
		char achNewPath[4096];
		wchar_t wchNewPath[4096];
		strcpy(achNewPath, getMazingerDir());
		strcat(achNewPath, "\\KojiKabuto.exe");
		mbstowcs(wchNewPath, achNewPath, 4095);

		hr = automationPutStringAttribute(pFwRule, L"Name", L"Allow Soffid ESSO traffic");
		if (FAILED(hr)) return NULL;
		hr = automationPutStringAttribute(pFwRule, L"ApplicationName", wchNewPath);
		if (FAILED(hr)) return NULL;
		hr = automationPutIntAttribute(pFwRule, L"Action", 1); // Allow
		if (FAILED(hr)) return NULL;
		hr = automationPutIntAttribute(pFwRule, L"Direction", direction); // Input
		if (FAILED(hr)) return NULL;
		hr = automationPutBooleanAttribute(pFwRule, L"Enabled", true); // ip version any
		if (FAILED(hr)) return NULL;
		hr = automationPutIntAttribute(pFwRule, L"Profiles", 0x7FFFFFFF); // All profiles
		if (FAILED(hr)) return NULL;
	}
	return pFwRule;
}
/** @brief Register vista firewall rules
 *
 * Enables KojiKabuto to to connect and to listen to sockets
 */
void ConfigureFirewallVista()
{
	HRESULT hr;							// Create instance result
	VARIANT varResult;
	unsigned int argError;


	CoInitialize(NULL);


	IDispatch* pFwPolicy2 = automationCreateObject(L"HNetCfg.FwPolicy2");
	if (pFwPolicy2 != NULL)
	{
		IDispatch *pFwRules = automationGetDispatchAttribute(pFwPolicy2, L"Rules");
		if (pFwRules != NULL)
		{
			IDispatch* pFwRule = createRule(1); // Input
			if (pFwRule != NULL)
			{
				automationInvoke(pFwRules, L"Add", pFwRule);
				pFwRule->Release();
			}
			IDispatch* pFwRule2 = createRule(2); // Output
			if (pFwRule2 != NULL)
			{
				automationInvoke(pFwRules, L"Add", pFwRule2);
				pFwRule2->Release();
			}
			pFwRules -> Release();
		}
		pFwPolicy2 -> Release();
	}
}

/** @brief Get previous GINA path
 *
 * Implements the functionality to obtain the previous GINA dll path.
 * @return Previous GINA dll path.
 */
char * GetPreviousGinaPath()
{
	HKEY hKey;										// Handle to open registry key
	DWORD dwType;								// Type of data stored
	DWORD getDLLresult;						// Result of obtain previous GINA dll
	char *ginaPath = new char();		// GINA path
	char winlogon[4096];						// Winlogon path
	DWORD size = sizeof winlogon;	// Size GINA path value

	getDLLresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DEF_WINLOGON_KEY_PATH.c_str(),
			0, Wow64Key(KEY_ALL_ACCESS), &hKey);

	// Check opened key
	if (getDLLresult == ERROR_SUCCESS)
	{
		getDLLresult = RegQueryValueEx(hKey, "GinaDll", NULL, &dwType,
				(LPBYTE) &winlogon, &size);
	}

	// Check previous GINA dll not obtained
	if (getDLLresult != ERROR_SUCCESS)
	{
		printf("\nUnable get previous GINA, stored default (msginal.dll)\n");

		strcpy(ginaPath, DEF_WINLOGON_DLL.c_str());
	}

	else
	{
		winlogon[size] = 0;
		strcpy(ginaPath, winlogon);
	}

	return ginaPath;
}

void updateConfig()
{
	HKEY hKey;
	char ach[4096];
	DWORD dw;
	DWORD dwType;

	DWORD dwResult;
	SECURITY_ATTRIBUTES sa;

	PSECURITY_DESCRIPTOR psd = createSecurityDescriptor(KEY_READ | KEY_WRITE);
	// Initialize a security attributes structure.
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = psd;
	sa.bInheritHandle = FALSE;

	printf("Configuring registry ....");
	fflush(stdout);

	if (!quiet)
		setProgressMessage("Configuring...");

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, DEF_REGISTRY_FOLDER.c_str(), 0, NULL,
			REG_OPTION_NON_VOLATILE, Wow64Key(KEY_ALL_ACCESS), &sa, &hKey,
			&dwResult) == ERROR_SUCCESS)
	{
		dw = RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, psd);

		if (dw != ERROR_SUCCESS)
		{
			anyError = true;
			printf("Error setting registry permissions\n");
		}

		if (RegQueryValueEx(hKey, "CertificateFile", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "%s\\root.cer", getMazingerDir());
			RegSetValueEx(hKey, "CertificateFile", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		if (RegQueryValueEx(hKey, "LogonDrive", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "p:");
			RegSetValueEx(hKey, "LogonDrive", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		if (RegQueryValueEx(hKey, "LogonPath", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "\\\\lofiapp1\\pcapp,\\\\sprewts1\\pcapp");
			RegSetValueEx(hKey, "LogonPath", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		if (RegQueryValueEx(hKey, "PreferedServers", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "lofiapp1");
			RegSetValueEx(hKey, "PreferedServers", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		if (RegQueryValueEx(hKey, "QueryServer", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, DEF_DEFAULT_SERVERS);
			RegSetValueEx(hKey, "QueryServer", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		if (RegQueryValueEx(hKey, "SSOServer", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, DEF_DEFAULT_SERVERS);
			RegSetValueEx(hKey, "SSOServer", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		if (RegQueryValueEx(hKey, "EnablePB", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			dw = 1;
			RegSetValueEx(hKey, "EnablePB", 0, REG_DWORD, (LPBYTE) &dw,
					sizeof dw);
		}

		if (RegQueryValueEx(hKey, "LocalCardSupport", NULL, &dwType, (LPBYTE) ach,
				&dw) == ERROR_FILE_NOT_FOUND)
		{
			dw = 3;
			RegSetValueEx(hKey, "LocalCardSupport", 0, REG_DWORD, (LPBYTE) &dw,
					sizeof dw);
		}

		if (RegQueryValueEx(hKey, "LocalOfflineAllowed", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			dw = 1;
			RegSetValueEx(hKey, "LocalOfflineAllowed", 0, REG_DWORD, (LPBYTE) &dw,
					sizeof dw);
		}

		if (RegQueryValueEx(hKey, "RemoteCardSupport", NULL, &dwType,
				(LPBYTE) ach, &dw) == ERROR_FILE_NOT_FOUND)
		{
			dw = 3;
			RegSetValueEx(hKey, "RemoteCardSupport", 0, REG_DWORD, (LPBYTE) &dw,
					sizeof dw);
		}

		if (RegQueryValueEx(hKey, "RemoteOfflineAllowed", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			dw = 1;
			RegSetValueEx(hKey, "RemoteOfflineAllowed", 0, REG_DWORD, (LPBYTE) &dw,
					sizeof dw);
		}

		if (RegQueryValueEx(hKey, "seycon.https.port", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "760");
			RegSetValueEx(hKey, "seycon.https.port", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		// Check previous version installed
		dw = sizeof ach;
		if (RegQueryValueEx(hKey, "MazingerVersion", NULL, &dwType, (LPBYTE) ach, &dw) == ERROR_SUCCESS)
		{
			isUpdate = true;
		}

		strcpy(ach, MAZINGER_VERSION_STR);
		RegSetValueEx(hKey, "MazingerVersion", 0, REG_SZ, (LPBYTE) ach,
				strlen(ach));

		dw = sizeof(ach);
		dwResult = RegQueryValueEx(hKey, "PreviousGinaDll", NULL, &dwType,
				(LPBYTE) ach, &dw);

		// Add previous GINA registry key
		if ((dwResult == ERROR_FILE_NOT_FOUND) || (dwResult == ERROR_SUCCESS))
		{
//			strncpy(ach, GetPreviousGinaPath(), 4096);
			strcpy(ach, DEF_WINLOGON_DLL.c_str());

			RegSetValueEx(hKey, "PreviousGinaDll", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		// Add force startup on login registry key
		if (RegQueryValueEx(hKey, "ForceStartupLogin", NULL, &dwType,
				(LPBYTE) ach, &dw) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "true");

			RegSetValueEx(hKey, "ForceStartupLogin", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		// Add enable user close session registry key
		if (RegQueryValueEx(hKey, "EnableCloseSession", NULL, &dwType,
				(LPBYTE) ach, &dw) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "false");

			RegSetValueEx(hKey, "EnableCloseSession", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		// Add login type registry key
		if (RegQueryValueEx(hKey, "LoginType", NULL, &dwType, (LPBYTE) ach,
				&dw) == ERROR_FILE_NOT_FOUND)
		{
			sprintf(ach, "kerberos");

			RegSetValueEx(hKey, "LoginType", 0, REG_SZ, (LPBYTE) ach,
					strlen(ach));
		}

		RegCloseKey(hKey);

		printf("Configured\n");
		log("REGISTRY HKLM\\Software\\Soffid\\esso configured");
	}

	else
	{
		anyError = true;
		printf("Unable to create registry KEY\n");
		notifyError();
		log("Cannot configure registry HKLM\\Software\\Soffid\\esso");
	}

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SoffidESSO", 0,
			NULL, REG_OPTION_NON_VOLATILE, Wow64Key(KEY_ALL_ACCESS), NULL,
			&hKey, &dwResult) == ERROR_SUCCESS)
	{
		printf("Registering uninstaller ....");
		fflush(stdout);

		if (!quiet)
			setProgressMessage("Registering uninstalller...");

		char ach[4096];
		DWORD dw;

		strcpy(ach, "Soffid Enterprise Single Sign-On");
		RegSetValueEx(hKey, "DisplayName", 0, REG_SZ, (LPBYTE) ach, strlen(ach));

		strcpy(ach, MAZINGER_VERSION_STR);
		RegSetValueEx(hKey, "DisplayVersion", 0, REG_SZ, (LPBYTE) ach,
				strlen(ach));

		strcpy(ach, "Soffid");
		RegSetValueEx(hKey, "Publisher", 0, REG_SZ, (LPBYTE) ach, strlen(ach));

		sprintf(ach, "\"%s\\uninstall.exe\"", getMazingerDir());
		RegSetValueEx(hKey, "UninstallString", 0, REG_SZ, (LPBYTE) ach,
				strlen(ach));

		sprintf(ach, "\"%s\\mazinger.exe\"", getMazingerDir());
		RegSetValueEx(hKey, "DisplayIcon", 0, REG_SZ, (LPBYTE) ach, strlen(ach));

		time_t t;
		t = time(NULL);
		struct tm* tmp;
		tmp = localtime(&t);
		strftime(ach, sizeof ach, "%Y%m%d", tmp);
		RegSetValueEx(hKey, "InstallDate", 0, REG_SZ, (LPBYTE) ach, strlen(ach));

		dw = 1;
		RegSetValueEx(hKey, "NoModify", 0, REG_DWORD, (LPBYTE) &dw, sizeof dw);

		dw = 1;
		RegSetValueEx(hKey, "NoRepair", 0, REG_DWORD, (LPBYTE) &dw, sizeof dw);

		RegCloseKey(hKey);

		printf("Registered\n");
		log("Registered uninstall");
	}

	else
	{
		anyError = true;
		printf("Unable to create registry KEY\n");
	}

	registerIEHook();
	registerFFHook();
	registerChromePlugin ();
	registerJetScrander();
	registerMazingerMsg();
//	registerFirewallException();

	if (IsWindowsXP())
		ConfigureFirewall();
	else
		ConfigureFirewallVista();



	if (IsWindowsXP())
		registerBoss();
}

void updateUserInit(const char *quitar, const char*poner)
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0,
			Wow64Key(KEY_ALL_ACCESS), &hKey) == ERROR_SUCCESS)
	{
		char achPath[4096] = "";
		char achNewPath[4096];
		DWORD dwType;
		DWORD size = sizeof achPath;
		RegQueryValueEx(hKey, "UserInit", NULL, &dwType, (LPBYTE) achPath, &size);

		strcpy(achNewPath, poner);
		char *token = strtok(achPath, ",");

		while (token != NULL)
		{
			for (int i = 0; token[i] != '\0'; i++)
				token[i] = tolower(token[i]);

			// Si no cont\E9 el que s'ha de llevar
			if (strstr(token, quitar) == NULL && strstr(token, poner) == NULL)
			{
				strcat(achNewPath, ",");
				strcat(achNewPath, token);
			}

			token = strtok(NULL, ",");
		}

		DWORD result = RegSetValueEx(hKey, "Userinit", NULL, REG_SZ,
				(LPBYTE) achNewPath, strlen(achNewPath));

		if (result != ERROR_SUCCESS)
		{
			anyError = true;
			log("> ERROR: Cannot set registry entry userinit");
			printf("Error setting registry value \n");
		}

		RegCloseKey(hKey);
		log("Configured winlogon");
	}

	else
	{
		anyError = true;
		printf("Unable to register SoffidESSO\n");
		log("Cannot configure winlogon");
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValueA(hKey, "PasswordBank SingleSignOn Client");
		RegCloseKey(hKey);
	}

	else
	{
		printf("Unable to register SoffidESSO\n");
	}
}

void updateGina(const char *file)
{
	HKEY hKey;

	if (!quiet)
		setProgressMessage("Configuring GINA...");

	log("Configurig gina");

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		DWORD result = RegSetValueEx(hKey, "GinaDll", NULL, REG_SZ, (LPBYTE) file,
				strlen(file));

		if (result != ERROR_SUCCESS)
		{
			anyError = true;
			printf("Error setting registry value GinaDll \n");
			log("Cannot configure ginadll");
		}

		RegCloseKey(hKey);
	}

	else
	{
		anyError = true;
		printf("Unable to deregister SoffidESSO\n");
		log(
				"Cannot open HKLM\\software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
	}
}

void installCP(const char *file)
{
	char szKey[MAX_PATH] = "";
	char szEntry[MAX_PATH] = "";
	char szValue[MAX_PATH] = "";
	DWORD dwValue;

	if (!quiet)
		setProgressMessage("Configuring Credential Provider...");

	// SMART CARD PROVIDER
	strcpy(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\"
			"{8bf9a910-a8ff-457f-999f-a5ca10b4a885}");
	strcpy(szEntry, "Disabled");
	dwValue = 1;
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, szEntry, REG_DWORD,
			(void*) &dwValue, sizeof dwValue);

	// SMARTCARD PIN PROVIDER DISABLED
	strcpy(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\"
			"{94596c7e-3744-41ce-893e-bbf09122f76a}");
	strcpy(szEntry, "Disabled");
	dwValue = 0;
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, szEntry, REG_DWORD,
			(void*) &dwValue, sizeof dwValue);

	const char *sayakaClsid = "{bdbb527b-40b1-44af-8c19-816f65c14016}";

	// SAYAKA CREDENTIAL PROVIDER
	sprintf(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\%s",
			sayakaClsid);
	strcpy(szValue, "Sayaka Credential Provider");
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SAYAKA CLSID
	sprintf(szKey, "CLSID\\%s", sayakaClsid);
	strcpy(szValue, "Sayaka Credential Provider");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SAYAKA CLSID / Inprocserver32
	sprintf(szKey, "CLSID\\%s\\InprocServer32", sayakaClsid);
	strcpy(szValue, file);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));
	strcpy(szValue, "Apartment");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, "ThreadingModel", REG_SZ,
			(void*) szValue, strlen(szValue));

	const char *shiroClsid = "{e30dee24-e1aa-4880-a0ca-4a02e74f78f2}";

	// SHIRO CREDENTIAL PROVIDER
	sprintf(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\%s",
			shiroClsid);
	strcpy(szValue, "ShiroKabuto Credential Provider");
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SHIRO CLSID
	sprintf(szKey, "CLSID\\%s", shiroClsid);
	strcpy(szValue, "Shiro Kabuto Credential Provider");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SHIRO CLSID / Inprocserver32
	sprintf(szKey, "CLSID\\%s\\InprocServer32", shiroClsid);
	strcpy(szValue, file);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));
	strcpy(szValue, "Apartment");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, "ThreadingModel", REG_SZ,
			(void*) szValue, strlen(szValue));

	const char *recoverClsid = "{e046f8f0-7ca2-4c83-8e6b-a273f4911a48}";
	// Recover CREDENTIAL PROVIDER
	sprintf(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\%s",
			recoverClsid);
	strcpy(szValue, "Sayaka Recover Credential Provider");
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// Recover CLSID
	sprintf(szKey, "CLSID\\%s", recoverClsid);
	strcpy(szValue, "Sayaka Recover Credential Provider");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SHIRO CLSID / Inprocserver32
	sprintf(szKey, "CLSID\\%s\\InprocServer32", recoverClsid);
	strcpy(szValue, file);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));
	strcpy(szValue, "Apartment");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, "ThreadingModel", REG_SZ,
			(void*) szValue, strlen(szValue));
}

bool installShiroKabuto(char *achShiroPath)
{
	bool allowed = true;

	log("Configuring Shirokabuto");

	if (!quiet)
		setProgressMessage("Configuring Shiro Kabuto....");

	SC_HANDLE sch = OpenSCManager(NULL, // Current Machine
			NULL, // Active Services
			SC_MANAGER_CREATE_SERVICE);
	char achpath[MAX_PATH];
	sprintf(achpath, "\"%s\"", achShiroPath);

	if (sch != NULL)
	{
		SC_HANDLE h = CreateServiceA(sch, "ShiroKabuto",
				"Soffid Single Sing-On User Manager", SC_MANAGER_CREATE_SERVICE,
				SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL, achpath, NULL, NULL, NULL, NULL, NULL);

		if (h == NULL)
		{
			if (GetLastError() != ERROR_SERVICE_EXISTS)
			{
				printf("WARNING !! Cannot create ShiroKabuto service\n");
				notifyError();
				anyError = true;
				log("Cannot register shirokabuto");
			}
		}

		else
			CloseServiceHandle(h);
	}

	else
	{
		anyError = true;
		printf("WARNING !! Cannot register ShiroKabuto service\n");
	}

	CloseServiceHandle(sch);

	return allowed;
}

void registerKojiKabuto()
{
	char achNewPath[4096];

	if (!anyError)
	{
		strcpy(achNewPath, getMazingerDir());
		strcat(achNewPath, "\\KojiKabuto.exe");

		for (int i = 0; achNewPath[i]; i++)
			achNewPath[i] = tolower(achNewPath[i]);

		updateUserInit("userinit", achNewPath);
	}

	if (!anyError)
	{
		strcpy(achNewPath, getMazingerDir());
		strcat(achNewPath, "\\SayakaGina.dll");

		updateGina(achNewPath);
	}

	if (!anyError)
	{
		strcpy(achNewPath, getMazingerDir());
		strcat(achNewPath, "\\SayakaCP.dll");
		installCP(achNewPath);
	}

	if (!anyError)
	{
		strcpy(achNewPath, getMazingerDir());
		strcat(achNewPath, "\\ShiroKabuto.exe");
		installShiroKabuto(achNewPath);
	}
}

void uninstallFile(LPCSTR achFilePath)
{
	if (!quiet)
		setProgressMessage("Uninstalling %s ....", achFilePath);

	log("Removing %s", achFilePath);

	FILE *f = fopen(achFilePath, "r");

	if (f != NULL)
	{
		fclose(f);

		printf("Removing %s ...", achFilePath);

		DWORD result = MoveFileEx(achFilePath, NULL, MOVEFILE_WRITE_THROUGH);

		if (result != 0)
		{
			log("> Removed");
			printf("Removed\n");
		}

		else
		{
			result = MoveFileEx(achFilePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

			if (result)
			{
				log("> Remove scheduled");
				printf("Delayed\n");
			}

			else
			{
				log("> Remove failed");
				printf("Failed\n");
				anyError = true;
			}
		}
	}

	fflush(stdout);
}

void uninstallResource(const char *lpszFileName)
{
	if (anyError)
		return;

	char achFilePath[5096];
	LPCSTR lpszDir = getMazingerDir();
	strcpy(achFilePath, lpszDir);
	strcat(achFilePath, "\\");
	strcat(achFilePath, lpszFileName);

	uninstallFile(achFilePath);
}

bool extractResource(LPCSTR resource, const char *lpszFileName)
{
	HRSRC hRsrc = FindResource(NULL, resource, MAKEINTRESOURCE(IDT_FILE) );

	if (hRsrc == NULL)
	{
		log(">> Missing resource %s", resource);
		printf("ERROR. Missing resource %s for %s\n", resource, lpszFileName);
		exit(2);
	}

	DWORD dwSize = SizeofResource(NULL, hRsrc);

	if (dwSize <= 0)
	{
		log(">> Wrong resource size %d", dwSize);
		printf("ERROR. Missing resource %s for %s\n", resource, lpszFileName);
		notifyError();
		exit(2);
	}

	HGLOBAL hGlobal = LoadResource(NULL, hRsrc);

	if (hGlobal == NULL)
	{
		log(">> Cannot load resource %s", resource);
		printf("ERROR. Missing resource %s for %s\n", resource, lpszFileName);
		notifyError();
		exit(2);
	}

	LPVOID lpVoid = LockResource(hGlobal);

	if (lpVoid == NULL)
	{
		printf("ERROR. Missing resource %s for %s\n", resource, lpszFileName);
		log(">> Cannot lock resource %s", resource);
		notifyError();
		exit(2);
	}

	FILE *f = fopen(lpszFileName, "wb");
	if (f == NULL)
	{
		log(">> Cannot create file %s", lpszFileName);
		printf("ERROR: Cannot create file %s\n", lpszFileName);

		return false;
	}

	else
	{
		if (true)
		{
			char *data = (char*) lpVoid;
			const int bufferSize = 2000;
			char buffer[bufferSize];
			bz_stream stream;
			memset(&stream, 0, sizeof stream);
			stream.avail_in = dwSize;
			stream.next_in = data;
			BZ2_bzDecompressInit(&stream, 0, 0);
			int status;

			do
			{
				stream.next_out = buffer;
				stream.avail_out = bufferSize;
				status = BZ2_bzDecompress(&stream);

				if (status == BZ_OK || status == BZ_STREAM_END)
				{
					int d = bufferSize - stream.avail_out;
					int d2 = (long) stream.next_out - (long) buffer;
					fwrite(buffer, d, 1, f);
				}
			}
			while (status == BZ_OK);

			switch (status)
			{
				case BZ_PARAM_ERROR:
					log(">> BZIP ERROR: Error de parametro");
					printf("BZIP: Error de parametro\n");
					return false;

				case BZ_DATA_ERROR:
					log(">> BZIP ERROR: Corrupt resource");
					printf("Archivo corrupto\n");
					return false;

				case BZ_DATA_ERROR_MAGIC:
					log(">> BZIP ERROR: Compression error");
					printf("Error de compresi\F3n\n");
					return false;

				case BZ_MEM_ERROR:
					log(">> BZIP ERROR: Out of memory");
					printf("Error de memoria\n");
					return false;
			}
		}

		else
		{
			if (fwrite(lpVoid, dwSize, 1, f) != dwSize)
			{
				log(">> BZIP ERROR: Cannot write file");
				printf("Error de escritura en disco");
				return false;
			}
		}

		fflush(f);
		fclose(f);
		return true;
	}
}

bool installResource(const char *lpszTargetDir, const char *lpszResourceName,
		const char *lpszFileName, bool hardReplace)
{
	std::string dirPath;	// Installation dir path
	std::string filePath;	// Installation file path

	if (anyError)
		return false;

	if (!quiet)
		setProgressMessage("Installing %s ...", lpszFileName);

	fflush(stdout);

	if ((lpszTargetDir != NULL) && (lpszTargetDir[1] == ':'))
		dirPath = lpszTargetDir;

	else
	{
		dirPath = getMazingerDir();

		if (lpszTargetDir != NULL)
		{
			dirPath += "\\";
			dirPath += lpszTargetDir;
		}
	}

	mkdir(dirPath.c_str());

	filePath = dirPath;
	filePath += "\\";
	filePath += lpszFileName;

	log("Installing %s to %s", lpszResourceName, filePath.c_str());

	printf("%s ...", filePath.c_str());

	// En cualquier caso, planificar repetir la acci\F3n al reiniciar
	std::string tempFilePath = filePath;
	tempFilePath += "_";
	FILE *f;

	do
	{
		f = fopen(tempFilePath.c_str(), "r");
		if (f != NULL)
		{
			tempFilePath += "_";
			fclose(f);
		}
	}
	while (f != NULL);

	std::string oldFile = filePath;
	oldFile += ".old";

	do
	{
		f = fopen(oldFile.c_str(), "r");

		if (f != NULL)
		{
			oldFile += "_";
			fclose(f);
		}
	}
	while (f != NULL);

	bool success = false;

	if (extractResource(lpszResourceName, tempFilePath.c_str()))
	{
		// Intentar mover el archivo actual
		// Ahora intentarlo copiar en caliente
		f = fopen(filePath.c_str(), "r");
		if (f != NULL)
			fclose(f);

		// Si no existe o puedo borrarlo => Generar el fichero
		if (f == NULL || unlink(filePath.c_str()) == 0)
		{
			if (MoveFileEx(tempFilePath.c_str(), filePath.c_str(),
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			{
				success = true;
			}

			else
				log("> FAILED to create file %s ", filePath.c_str());
		}

		// Si no puedo borrarlo, intento renombrarlo ahora
		else if (hardReplace
				&& MoveFileEx(filePath.c_str(), oldFile.c_str(),
						MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			if (MoveFileEx(oldFile.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
			{
				log("> Old file renamed to %s. Will be deleted on reboot",
						oldFile.c_str());
			}

			// Despu\E9s de renombrar, ya puedo sustituir el fichero en caliente
			if (MoveFileEx(tempFilePath.c_str(), filePath.c_str(),
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			{
				success = true;
			}

			else
				log("> FAILED to replace file %s with %s", filePath.c_str(),
						tempFilePath.c_str());
			// Si no puedo renombrarlo ahora, lo sustituir\E9 mas tarde
		}

		else if (replaceOnReboot(filePath.c_str(), tempFilePath.c_str()))
		{
			log("> Scheduled substitution on reboot");
			success = true;
			reboot = true;
		}

		else
		{
			log("> FAILED to schedule replace of file %s with %s",
					filePath.c_str(), tempFilePath.c_str());
		}
	}

	else
	{
		log("> FAILED extraction of %s into %s", lpszResourceName,
				tempFilePath.c_str());

		if (GetLastError() != ERROR_SUCCESS)
		{
			notifyError();
		}
	}

	if (success)
	{
		printf("OK\n");
		log("> Success");
	}

	else
	{
		printf("ERROR\n");
		anyError = true;
	}

	fflush(stdout);

	return success;
}

void installResource(const char *lpszTargetDir, const char *lpszFileName,
		const char *lpszTargetName)
{
	installResource(lpszTargetDir, lpszFileName, lpszTargetName, true);
}

void installResource(const char *lpszTargetDir, const char *lpszFileName)
{
	installResource(lpszTargetDir, lpszFileName, lpszFileName, true);
}

struct Files
{
		const char *dir;
		const char *resourceName;
		const char *fileName;
};

void mksubdir(const char* subdir)
{
	char achDir[4096];
	LPCSTR lpszDir = getMazingerDir();

	if (subdir == NULL)
		strcpy(achDir, lpszDir);

	else
		sprintf(achDir, "%s\\%s", lpszDir, subdir);

#ifdef __GNUC__
	mkdir(achDir);

#else
	CreateDirectoryA(achDir, NULL);
#endif
}

bool notifyPendingRenames()
{
	bool anyFound = false;
	log("****************************");
	log("** Scheduled file operations");
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0,
			Wow64Key(KEY_READ), &hKey) == ERROR_SUCCESS)
	{
		static char achPath[2];
		DWORD dwType;
		DWORD size;
		size = 0;
		if (ERROR_FILE_NOT_FOUND
				!= RegQueryValueEx(hKey, "PendingFileRenameOperations", NULL,
						&dwType, (LPBYTE) achPath, &size))
		{
			char *buffer = (char*) malloc(size + 1);

			if (ERROR_SUCCESS
					== RegQueryValueEx(hKey, "PendingFileRenameOperations", NULL,
							&dwType, (LPBYTE) buffer, &size))
			{
				const char *mznDir = getMazingerDir();
				int mznDirLen = strlen(mznDir);

				for (int i = 0; i < size && buffer[i]; i++)
				{
					char *f1 = &buffer[i];
					int len1 = strlen(f1);
					i += len1 + 1;
					char *f2 = &buffer[i];
					int len2 = strlen(f2);
					i += len2 + 1;

					if (strstr(f1, mznDir) != NULL)
						anyFound = true;

					log("** [%s]->[%s] **", f1, f2);
				}
			}
		}

		RegCloseKey(hKey);
	}

	log("****************************");

	return anyFound;
}

int install(int full)
{
	LPCSTR lpszDir = getMazingerDir();

	if (lpszDir == NULL)
	{
		printf("ERROR. Unable to generate installationDir\n");
		anyError = true;
		return (-1);
	}

	if (!quiet)
		setProgressMessage("Installing into %s", lpszDir);

	mksubdir(NULL);
	mksubdir("FFExtension");
	mksubdir("FFExtension\\chrome");
	mksubdir("FFExtension\\chrome\\modules");
	mksubdir("FFExtension\\chrome\\content");
	mksubdir("FFExtension\\chrome\\locale");
	mksubdir("FFExtension\\chrome\\locale\\en-US");
	mksubdir("FFExtension\\default");
	mksubdir("FFExtension\\default\\preferences");
	mksubdir("FFExtension\\components");

	createCacheDir();

	printf("Installing MAZINGER %s into %s\n", MAZINGER_VERSION_STR, lpszDir);
	log("Installing MAZINGER %s into %s\n", MAZINGER_VERSION_STR, lpszDir);

	if (IsWow64())
	{
		installResource(NULL, "Mazinger64.exe", "Mazinger.exe");
		installResource(NULL, "MazingerHook64.dll", "MazingerHook.dll", false);
		installResource(NULL, "MazingerHook.dll", "MazingerHook32.dll", false);
		installResource(NULL, "KojiKabuto64.exe", "KojiKabuto.exe");
		installResource(NULL, "KojiHook64.dll", "KojiHook.dll");

//		installResource(NULL, "AfroditaFC64.dll", "AfroditaFC.dll");
		installResource(NULL, "AfroditaFC.dll", "AfroditaFC.dll");
		installResource(NULL, "AfroditaFC.dll", "AfroditaFC32.dll");
		installResource(NULL, "AfroditaC64.dll", "AfroditaC64.dll");
		installResource(NULL, "AfroditaC.dll", "AfroditaC.dll");

		installResource(NULL, "AfroditaE64.dll", "AfroditaE.dll");
		installResource(NULL, "AfroditaE.dll", "AfroditaE32.dll");
		installResource(NULL, "JetScrander.exe");
		installResource(NULL, "SayakaCP64.dll", "SayakaCP.dll");

		installResource(NULL, "SoffidConfig64.exe", "SoffidConfig.exe");
	}
	else
	{
		installResource(NULL, "Mazinger.exe");
		installResource(NULL, "MazingerHook.dll", "MazingerHook.dll", false);
		installResource(NULL, "KojiKabuto.exe");
		installResource(NULL, "KojiHook.dll");

		installResource(NULL, "AfroditaE.dll");
		installResource(NULL, "AfroditaFC.dll");
		installResource(NULL, "AfroditaC.dll");
		installResource(NULL, "JetScrander.exe");
		installResource(NULL, "SayakaCP.dll");

		installResource(NULL, "SoffidConfig.exe");
	}

	std::string system = getenv("SystemRoot");
	if (IsWow64())
	{
		std::string sys1 = system + "\\System32";
		installResource (sys1.c_str(), "libwinpthread-1-64.dll", "libwinpthread-1.dll");
		std::string sys2 = system + "\\SysWOW64";
		installResource (sys2.c_str(), "libwinpthread-1-32.dll", "libwinpthread-1.dll");
//		installResource (NULL, "libwinpthread-1-64.dll", "libwinpthread-1.dll");
	}
	else
	{
//		std::string sys1 = system + "\\System32";
//		installResource (sys1.c_str(), "Winpthread-1-32.dll", "WINPTHREAD-1.DLL");
		installResource (NULL, "libwinpthread-1-32.dll", "libwinpthread-1.dll");
	}

	if (IsWindowsXP())
	{
		installResource(NULL, "Boss.exe");
	}

	installResource(NULL, "SayakaGina.dll");
	installResource(NULL, "ShiroKabuto.exe");
	installResource(NULL, "Nossori.exe");

	installResource(NULL, "seycon.cer");
	installResource(NULL, "logon.tcl");
	installResource(NULL, "uninstall.exe");
	installResource(NULL, "sewashi.exe");
	installResource(NULL, "profyumi.jar");
	installResource("FFExtension\\modules", "afrodita.jsm");
	installResource("FFExtension\\chrome\\content", "about.xul");
	installResource("FFExtension\\chrome\\content", "ff-overlay.xul");
	installResource("FFExtension\\chrome\\content", "overlay.js");
	installResource("FFExtension\\chrome\\locale\\en-US", "about.dtd");
	installResource("FFExtension\\chrome\\locale\\en-US", "overlay.properties");
	installResource("FFExtension\\default\\preferences", "prefs.js");
	installResource("FFExtension", "chrome.manifest");
	installResource("FFExtension", "install.rdf");
	uninstallResource("FFExtension\\components\\AfroditaF.dll");
	uninstallResource("FFExtension\\components\\AfroditaF5.dll");
	uninstallResource("FFExtension\\components\\AfroditaF.xpt");


//	installTCL();

	if (!anyError)
		updateConfig();

	if (full && !anyError)
	{
		registerKojiKabuto();
	}

	notifyPendingRenames();

	// Create Soffid ESSO programs folder
	CreateStartMenuFolder();

	return anyError;
}

/** @brief Set original Windows logon
 *
 * Implements the functionality to set original windows logon as active method.
 */
void SetOriginalWinlogon()
{
	HKEY hKey;
	char previuosGINA[4096];	// Previous GINA registered
	DWORD dw = sizeof previuosGINA;
	DWORD dwType;

	// Check Windows XP OS version
	if (IsWindowsXP())
	{
		// Get previous GINA
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, DEF_REGISTRY_FOLDER.c_str(), 0,
				Wow64Key(KEY_ALL_ACCESS), &hKey) == ERROR_SUCCESS)
		{
			// Add previous GINA registry key
			if (RegQueryValueEx(hKey, "PreviousGinaDll", NULL, &dwType,
					(LPBYTE) previuosGINA, &dw) == ERROR_SUCCESS)
			{
				strcpy(previuosGINA, getMazingerDir());
				strcat(previuosGINA, "\\SayakaGina.dll");

				RegSetValueEx(hKey, "PreviousGinaDll", 0, REG_SZ,
						(LPBYTE) previuosGINA, strlen(previuosGINA));

				updateGina(DEF_WINLOGON_DLL.c_str());
			}
		}

		RegCloseKey(hKey);
	}
}

/** @brief Run executable program
 *
 * Implementss the functionality to launch a executable program specified.
 * @param achString	Complete name of program to launch.
 * @param achDir		Folder path of program.
 * @return					Execution result.
 */
int RunProgram(char *achString, char *achDir)
{
	PROCESS_INFORMATION pInfo;	// Process information
	STARTUPINFO sInfo;					// Startup information
	DWORD dwExitStatus;				// Process execution result
	char command[4096];				// Command to run program

	sprintf(command, "%s\\%s", achDir, achString);
	printf("Executing %s from dir: %s\n", achString, achDir);

	memset(&sInfo, 0, sizeof sInfo);
	sInfo.cb = sizeof sInfo;
	sInfo.wShowWindow = SW_NORMAL;

// Check execution process correctly
	if (!CreateProcess(NULL, command, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
			NULL, NULL, &sInfo, &pInfo))
	{
		notifyError();

		return -1;
	}

	return 0;
}

/** @brief Run Soffid ESSO configuration tool
 *
 * Implements the functionality to run the Soffid ESSO configuration tool.
 * @return Execution result.
 */
int RunConfigurationTool()
{
	char mznDir[4096];	// Store the 'Mazinger' dir path
	int runResult = 0;		// Run program result

	if (MessageBoxA(NULL, "Do you want proceed with configuration?",
			"Soffid ESSO", MB_ICONQUESTION | MB_YESNO) == IDYES)
	{

		strcpy(mznDir, getMazingerDir());

		runResult = RunProgram((char*) DEF_CONFIG_TOOL_NAME.c_str(), mznDir);
	}
}

extern "C" int main(int argc, char **argv)
{
	const char* serverName = NULL;
	bool checkPending = true;
	int ibsalut = 0;

	// Read call arguments
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "/ibsalut") == 0 || strcmp(argv[i], "-ibsalut") == 0)
		{
			ibsalut = 1;
		}

		// Check quiet install method
		if (strcmp(argv[i], "/q") == 0 || strcmp(argv[i], "-q") == 0)
		{
			quiet = true;
		}

		// Check server install method
		if (strcmp(argv[i], "/server") == 0 || strcmp(argv[i], "-server") == 0)
		{
			i++;

			// Read server ID
			if (i < argc)
			{
				serverName = argv[i];
			}
		}

		if (strcmp(argv[i], "/force") == 0 || strcmp(argv[i], "-force") == 0)
			checkPending = false;

		// Check not modify previous GINA
		if ((strcmp(argv[i], "/nogina") == 0)
				|| (strcmp(argv[i], "-nogina") == 0))
		{
			noGina = true;
		}
	}

	log("Preparing install %s", ibsalut ? "/ibsalut" : "");
	log("Configured server %s", serverName);

	// Check pending operations
	if (checkPending)
	{
		bool pendingOperations = notifyPendingRenames();

		if (pendingOperations)
		{
			log("Installation aborted due to pending changes to apply");
			printf(
					"\n\nERROR. A prior installation needed to reboot the system.\nReboot prior to install\n\n");

			if (!quiet)
			{
				MessageBoxA(NULL,
						"A prior installation needed to reboot the system.\nReboot prior to install",
						"Soffid ESSO", MB_OK | MB_ICONEXCLAMATION);
			}

			exit(-1);
		}
	}

	int result = install(!ibsalut);

	if (noGina)
	{
		SetOriginalWinlogon();
	}

	if (result == 0 && serverName != NULL)
	{
		setProgressMessage("Connecting to %s", serverName);

		if (!configure(getMazingerDir(), serverName))
			!quiet ? result = 1 : result = 3;
	}

	disableProgressWindow();

	if (result)
	{
		if (!quiet)
		{
			MessageBoxA(NULL, "Installation has failed. Please, look at log file", "Soffid ESSO",
					MB_OK | MB_ICONEXCLAMATION);
		}
	}

	else if (reboot)
	{
		printf(
				"\n\n\nWARNING: Reboot is needed in order to complete setup\n\n\n");

		if (!quiet)
		{
			MessageBoxA(NULL, "Reboot is needed in order to complete setup",
					"Soffid ESSO", MB_OK | MB_ICONEXCLAMATION);

			if (!isUpdate)
			{
				RunConfigurationTool();
			}
		}
	}

	else
	{
		if (!quiet)
		{
			MessageBoxA(NULL, "Installation complete", "Soffid ESSO",
					MB_OK | MB_ICONEXCLAMATION);

			// Check update installation process
			if (!isUpdate)
			{
				RunConfigurationTool();
			}
		}
	}

	return result;
}
