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
#include <sddl.h>
#include <stdio.h>

void disableProgressWindow();
void setProgressMessage(const char *lpszMessage, ...);
int RunProgram(const char *achString, const char *achDir);

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
bool pam = true;
bool updateConfigFlag = false;
bool reboot = false;
bool isUpdate = false;
bool noGina = false;
bool msi = false;

char *enableCloseSession = "false";
char *forceStartupLogin = "true";
char *loginType = "both";

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

void notifyError(DWORD lastError)
{
	LPSTR pstr;
	char errorMsg[255];

	int fm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, 	NULL, lastError, 0, (LPSTR) &pstr, 0, NULL);

	// Format message failure
	if (fm == 0)
	{
		sprintf(errorMsg, "Unknown error: %d\n", lastError);
		log(">> %s", errorMsg);
	}

	else
	{
		log(">> %s", pstr);
	}

	if (!quiet)
	{
		disableProgressWindow();
		MessageBox(NULL, pstr, "Installation error", MB_OK | MB_ICONEXCLAMATION);
	}

	LocalFree(pstr);
}

void notifyError() {
	notifyError(GetLastError());
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
		log("AllocateAndInitializeSid Error %d\n", (int) GetLastError());
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

	// Extension for FF >= 52

	// Register ff extension
	wsprintf(szBuff, "%s\\afroditaFf2.xpi", getMazingerDir());

	log("Registering Firefox extension");
	HelperWriteKey(32, HKEY_LOCAL_MACHINE,
			"Software\\Mozilla\\Firefox\\Extensions",
			"esso@soffid.com", REG_SZ, (void*) szBuff,
			lstrlen(szBuff));

	log("Registering Firefox extension 64 bits");
	HelperWriteKey(64, HKEY_LOCAL_MACHINE,
			"Software\\Mozilla\\Firefox\\Extensions",
			"esso@soffid.com", REG_SZ, (void*) szBuff,
			lstrlen(szBuff));

	// Register ff manifest
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Mozilla\\NativeMessagingHosts",
			0, (LPSTR) "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,
			NULL) == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);

	}

	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Mozilla\\NativeMessagingHosts\\com.soffid.esso_chrome1",
			0, (LPSTR) "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,
			NULL) == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
	}

	std::string dir = getMazingerDir();
	dir += "\\afrodita-firefox.manifest";

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Mozilla\\NativeMessagingHosts\\com.soffid.esso_chrome1",
		NULL,
		REG_SZ, dir.c_str(), dir.length());


	// Create ff application manifest
	LPCSTR mznDir = getMazingerDir();
	std::string dir2 ;
	for (int i = 0; mznDir[i]; i++)
	{
		if (mznDir[i] == '\\')
			dir2 += '\\';
		dir2 += mznDir[i];
	}

	FILE * f = fopen (dir.c_str(), "w");
	fprintf (f, "{"
				"\"name\": \"com.soffid.esso_chrome1\","
				"\"description\": \"Soffid ESSO native host\","
				"\"type\": \"stdio\","
				"\"path\": \"%s\\\\afrodita-chrome.exe\","
				"\"allowed_extensions\": [\"esso@soffid.com\"]"
				"}",
				dir2.c_str());
	fclose (f);


}

void registerChromePlugin()
{
	char szBuff[4096];
	//
	//write the default value
	//

	log("Registering Chrome extension");

	std::string dir = getMazingerDir();
	dir += "\\afrodita-chrome.manifest";

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\Google\\Chrome\\NativeMessagingHosts\\com.soffid.esso_chrome1",
			NULL, REG_SZ, (void*)dir.c_str(), -1);

	LPCSTR mznDir = getMazingerDir();
	std::string dir2 ;
	for (int i = 0; mznDir[i]; i++)
	{
		if (mznDir[i] == '\\')
			dir2 += '\\';
		dir2 += mznDir[i];
	}
	FILE * f = fopen (dir.c_str(), "w");
	fprintf (f, "{\n"
				"  \"name\": \"com.soffid.esso_chrome1\", \n"
				"  \"description\": \"Soffid Chrome ESSO Handler\", \n"
				"  \"path\": \"%s\\\\afrodita-chrome.exe\",\n"
				"  \"type\": \"stdio\",\n"
				"  \"allowed_origins\": [\n"
				"    \"chrome-extension://gacgphonbajokjblndebfhakgcpbemdl/\",\n"
				"    \"chrome-extension://ggafmbddcpnehaegkbfleodcbjnllmbc/\"\n"
				"  ]\n"
				"}\n",
				dir2.c_str());
	fclose (f);


	// Register extension
	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\Google\\Chrome\\Extensions\\gacgphonbajokjblndebfhakgcpbemdl",
			"update_url", REG_SZ, (void*) "https://clients2.google.com/service/update2/crx", -1);

	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\Policies\\Google\\Chrome\\ExtensionInstallForcelist",
			"1", REG_SZ, (void*) "gacgphonbajokjblndebfhakgcpbemdl;https://clients2.google.com/service/update2/crx", -1);

	// Register EDGE extension
	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\Microsoft\\Edge\\Extensions\\ggafmbddcpnehaegkbfleodcbjnllmbc",
			"update_url", REG_SZ, (void*) "https://edge.microsoft.com/extensionwebstorebase/v1/crx", -1);


	HelperWriteKey(0, HKEY_LOCAL_MACHINE,
			"Software\\Policies\\Microsoft\\Edge\\ExtensionInstallForcelist",
			"1", REG_SZ, (void*) "ggafmbddcpnehaegkbfleodcbjnllmbc;https://edge.microsoft.com/extensionwebstorebase/v1/crx", -1);

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"Software\\Google\\Chrome\\Extensions", 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteKey(hKey, "ecdihhgdjciiabklgobilokhejhecjbm");
		RegCloseKey(hKey);
	}

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
	wsprintf(szDescriptionVal, "%s", ".mzn");
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
		log("\nUnable get previous GINA, stored default (msginal.dll)\n");

		strcpy(ginaPath, DEF_WINLOGON_DLL.c_str());
	}

	else
	{
		winlogon[size] = 0;
		strcpy(ginaPath, winlogon);
	}

	return ginaPath;
}

DWORD getCurrentUserSid(PSID* pSid) {
	HANDLE hToken;
	OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) ;
	//
	// Get the size of the memory buffer needed for the SID
	//
	DWORD dwBufferSize = 0;
	PTOKEN_USER token = NULL;
	while (true) {
		if (GetTokenInformation( hToken, TokenUser, (void*)token, dwBufferSize, &dwBufferSize ))
			break;
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
			if (token != NULL)
				free (token);
			token =(PTOKEN_USER) malloc(dwBufferSize);
		} else {
			return GetLastError();
		}
	}
	*pSid = token->User.Sid;
	return ERROR_SUCCESS;
}

BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue(
            NULL,            // lookup privilege on local system
            lpszPrivilege,   // privilege to lookup
            &luid ) )        // receives LUID of privilege
    {
        log("LookupPrivilegeValue error: %u\n", GetLastError() );
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
           hToken,
           FALSE,
           &tp,
           sizeof(TOKEN_PRIVILEGES),
           (PTOKEN_PRIVILEGES) NULL,
           (PDWORD) NULL) )
    {
          log("AdjustTokenPrivileges error: %u\n", GetLastError() );
          return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
          log("The token does not have the specified privilege. \n");
          return FALSE;
    }

    return TRUE;
}

DWORD getRegistryAcl (LPCSTR registryKey, PACL *pAcl, bool wow64) {
	HKEY hKey;
	DWORD r;
	log("Getting registry ACL");
	// {25CBB996-92ED-457e-B28C-4774084BD562}
	r = RegOpenKeyEx(HKEY_CLASSES_ROOT,
			registryKey, 0,
			wow64 ? Wow64Key(KEY_READ|READ_CONTROL) : KEY_READ|READ_CONTROL, &hKey);
	if (r != ERROR_SUCCESS) {
		notifyError();
		log("Error opening key");
		return r;
	}

	SECURITY_INFORMATION si;
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
	DWORD lpcbSecurityDescriptor = 0;
	while (ERROR_INSUFFICIENT_BUFFER == (r = RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION| DACL_SECURITY_INFORMATION,
		(LPVOID) pSecurityDescriptor, &lpcbSecurityDescriptor)))
	{
		if (pSecurityDescriptor != NULL)
			free(pSecurityDescriptor);
		pSecurityDescriptor = malloc (lpcbSecurityDescriptor);

	}

	if (r != ERROR_SUCCESS) {
		log("Error: Cannot get registry ACL");
		return r;
	}
	if (!IsValidSecurityDescriptor(pSecurityDescriptor)) {
		log("Error: Security descriptor is not valid");
		return r;
	}

	PACL dacl = NULL;
	BOOL daclPresent;
	BOOL daclDefault;
	if (!GetSecurityDescriptorDacl(pSecurityDescriptor, &daclPresent, &dacl, &daclDefault))
		return GetLastError();

	RegCloseKey(hKey);
	*pAcl = dacl;
	return ERROR_SUCCESS;
}

DWORD changeOwnership (LPCSTR lpszRegistryKey, bool wow64) {
	BOOL bRetval = FALSE;

	HANDLE hToken = NULL;
	PSID pSIDAdmin = NULL;
	PSID pSIDEveryone = NULL;
	PACL pACL = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
			SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	ULONG cEntries;
	EXPLICIT_ACCESS *prevEa;
	EXPLICIT_ACCESS *ea;
	DWORD dwRes;

	std::string reg = "CLASSES_ROOT\\";
	reg += lpszRegistryKey;

	log("Setting permissions for %s", reg.c_str());

	PACL prevAcl = NULL;
	dwRes = getRegistryAcl(lpszRegistryKey, &prevAcl, wow64);
	if (dwRes != ERROR_SUCCESS) {
		log("Cannot get registry ACL");
		return dwRes;
	}

	if ( GetExplicitEntriesFromAcl(prevAcl, &cEntries, &prevEa) != ERROR_SUCCESS) {
		notifyError();
		goto Cleanup;
	}

	ea = (EXPLICIT_ACCESS*)malloc((1+cEntries) * sizeof (EXPLICIT_ACCESS) );
	ZeroMemory(ea, (1+cEntries) * sizeof(EXPLICIT_ACCESS));
	for (int i = 0; i < cEntries; i++) {
		ea[i] = prevEa[i];
		if (ea[i].Trustee.TrusteeForm == TRUSTEE_IS_SID) {
			LPSTR str = NULL;
			ConvertSidToStringSid(ea[i].Trustee.ptstrName, &str);
			log ("ACL %d = %s", i, str);
		}
		else
			log ("ACL %d = %s", i, ea[i].Trustee.ptstrName);
	}
	// Set full control for myself.
	PSID myself;
	if (getCurrentUserSid(&myself) != ERROR_SUCCESS) {
		notifyError();
		goto Cleanup;
	}
	ea[cEntries].grfAccessPermissions = GENERIC_ALL;
	ea[cEntries].grfAccessMode = SET_ACCESS;
	ea[cEntries].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[cEntries].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[cEntries].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[cEntries].Trustee.ptstrName = (LPTSTR) myself;

	if (ERROR_SUCCESS != SetEntriesInAcl(cEntries+1,
										 ea,
										 NULL,
										 &pACL))
	{
		log("Failed SetEntriesInAcl\n");
		goto Cleanup;
	}

	// Try to modify the object's DACL.
	log("Registry %s. acl %p", reg.c_str(), pACL);
	dwRes = SetNamedSecurityInfo(
		(LPSTR) reg.c_str(),                 // name of the object
		wow64 ? (SE_OBJECT_TYPE) (SE_REGISTRY_WOW64_32KEY  ) : SE_REGISTRY_KEY,              // type of object
		DACL_SECURITY_INFORMATION,   // change only the object's DACL
		NULL, NULL,                  // do not change owner or group
		pACL,                        // DACL specified
		NULL);                       // do not change SACL

	if (ERROR_SUCCESS == dwRes)
	{
		log("Successfully changed DACL\n");
		// No more processing needed.
		goto Cleanup;
	}
	if (dwRes != ERROR_ACCESS_DENIED)
	{
		notifyError(dwRes);
		log("First SetNamedSecurityInfo call failed: %u\n",
				dwRes);
		goto Cleanup;
	}

	// If the preceding call failed because access was denied,
	// enable the SE_TAKE_OWNERSHIP_NAME privilege, create a SID for
	// the Administrators group, take ownership of the object, and
	// disable the privilege. Then try again to set the object's DACL.

	// Open a handle to the access token for the calling process.
	if (!OpenProcessToken(GetCurrentProcess(),
						  TOKEN_ADJUST_PRIVILEGES,
						  &hToken))
	{
		dwRes = GetLastError();
		log("OpenProcessToken failed: %u\n", GetLastError());
		goto Cleanup;
	}

	// Enable the SE_TAKE_OWNERSHIP_NAME privilege.
	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
	{
		dwRes = GetLastError();
		log("You must be logged on as Administrator.\n");
		goto Cleanup;
	}

	// Set the owner in the object's security descriptor.
	log("Setting security info");
	dwRes = SetNamedSecurityInfo(
		(LPSTR) reg.c_str(),                 // name of the object
		wow64 ? (SE_OBJECT_TYPE) (SE_REGISTRY_WOW64_32KEY ) : SE_REGISTRY_KEY,              // type of object
		OWNER_SECURITY_INFORMATION,  // change only the object's owner
		myself,                   // SID of Administrator group
		NULL,
		NULL,
		NULL);

	if (dwRes != ERROR_SUCCESS)
	{
		dwRes = GetLastError();
		log("Could not set owner. Error: %u\n", dwRes);
		goto Cleanup;
	}

	// Disable the SE_TAKE_OWNERSHIP_NAME privilege.
	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE))
	{
		dwRes = GetLastError();
		log("Failed SetPrivilege call unexpectedly.\n");
		goto Cleanup;
	}

	// Try again to modify the object's DACL,
	// now that we are the owner.
	dwRes = SetNamedSecurityInfo(
		(LPSTR) reg.c_str(),                 // name of the object
		wow64 ? (SE_OBJECT_TYPE) (SE_REGISTRY_WOW64_32KEY  ) : SE_REGISTRY_KEY,              // type of object
		DACL_SECURITY_INFORMATION,   // change only the object's DACL
		NULL, NULL,                  // do not change owner or group
		pACL,                        // DACL specified
		NULL);                       // do not change SACL

	if (dwRes == ERROR_SUCCESS)
	{
		log("Successfully changed DACL\n");
	}
	else
	{
		log("Second SetNamedSecurityInfo call failed: %u\n",
				dwRes);
	}

Cleanup:

	if (pSIDAdmin)
		FreeSid(pSIDAdmin);

	if (pSIDEveryone)
		FreeSid(pSIDEveryone);

	if (pACL)
	   LocalFree(pACL);

	if (hToken)
	   CloseHandle(hToken);

	return dwRes;

}

void updateBasicCredentialProvider (bool wow64) {
	HKEY hKey;
	DWORD r;
	if (wow64)
		log("Registering basic credential provider (64bits)");
	else
		log("Registering basic credential provider (32bits)");

	r = RegOpenKeyEx(HKEY_CLASSES_ROOT,
				"Clsid\\{25CBB996-92ED-457e-B28C-4774084BD562}", 0,
				wow64 ?  Wow64Key(KEY_ALL_ACCESS):KEY_ALL_ACCESS, &hKey);
	if (r != ERROR_SUCCESS) {
		r = changeOwnership("Clsid\\{25CBB996-92ED-457e-B28C-4774084BD562}", wow64);
		if ( r != ERROR_SUCCESS)
		{
			log("Error setting ownership key");
			return;
		}
		r = RegOpenKeyEx(HKEY_CLASSES_ROOT,
					"Clsid\\{25CBB996-92ED-457e-B28C-4774084BD562}", 0,
					wow64 ?  Wow64Key(KEY_ALL_ACCESS):KEY_ALL_ACCESS, &hKey);
	}

	if (r == ERROR_SUCCESS)
	{
		HKEY hKey2;
		r = RegCreateKeyExA(hKey, "TreatAs", 0, (LPSTR) "",
				REG_OPTION_NON_VOLATILE, Wow64Key(KEY_ALL_ACCESS), NULL,
				&hKey2, NULL);
		if (r != ERROR_SUCCESS) {
			log("Error create registry key");
			notifyError(r);
			return;
		}
		std::string target = "{44ee45c8-529b-4542-98ed-cfeceab1d4cc}";
		RegSetValueA(hKey, "TreatAs", REG_SZ, target.c_str(), target.length());
		RegCloseKey ( hKey2);
		RegCloseKey ( hKey);
	}
}

void updateBasicCredentialProvider2 ( bool wow64 ) {
	std::string s = "rundll32 \"";

	s += getMazingerDir();


	if (wow64) {
		s += "\\sayakaCP32.dll";
	} else {
		s += "\\sayakaCP.dll";
	}
	s += "\",Register";

	PROCESS_INFORMATION pInfo;	// Process information
	STARTUPINFO sInfo;					// Startup information
	DWORD dwExitStatus;				// Process execution result
	char command[4096];				// Command to run program

	strcpy(command, s.c_str());

	memset(&sInfo, 0, sizeof sInfo);
	sInfo.cb = sizeof sInfo;
	sInfo.wShowWindow = SW_NORMAL;

// Check execution process correctly
	log("Executing %s", s.c_str());
	if (!CreateProcess(NULL, command, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
			NULL, NULL, &sInfo, &pInfo))
	{
		log("Error executing %s", s.c_str());
		notifyError();

	}
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
			log("Error setting registry permissions\n");
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

		if (RegQueryValueEx(hKey, "enableCloseSession", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			RegSetValueEx(hKey, "enableCloseSession", 0, REG_SZ, (LPBYTE) enableCloseSession,
					strlen(enableCloseSession));
		}

		if (RegQueryValueEx(hKey, "ForceStartupLogin", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			RegSetValueEx(hKey, "ForceStartupLogin", 0, REG_SZ, (LPBYTE) forceStartupLogin,
					strlen(forceStartupLogin));
		}

		if (RegQueryValueEx(hKey, "LoginType", NULL, NULL, NULL,
				NULL) == ERROR_FILE_NOT_FOUND)
		{
			RegSetValueEx(hKey, "LoginType", 0, REG_SZ, (LPBYTE) loginType,
					strlen(loginType));
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

		log("REGISTRY HKLM\\Software\\Soffid\\esso configured");
	}

	else
	{
		anyError = true;
		notifyError();
		log("Cannot configure registry HKLM\\Software\\Soffid\\esso");
	}

	if (!msi) {

		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
				"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SoffidESSO", 0,
				NULL, REG_OPTION_NON_VOLATILE, Wow64Key(KEY_ALL_ACCESS), NULL,
				&hKey, &dwResult) == ERROR_SUCCESS)
		{
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
			log("Registered uninstall");
		}
		else
		{
			log("> Cannot register uninstall");
			anyError = true;
		}
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

static bool needsUpdate ()
{
	HKEY hKey;
	char ach[4096];
	DWORD dw;
	DWORD dwType;

	DWORD dwResult;
	bool needsUpdate = false;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				DEF_REGISTRY_FOLDER.c_str(), 0, Wow64Key(KEY_READ),
				&hKey) == ERROR_SUCCESS)
	{

		dw = sizeof ach;
		if (RegQueryValueEx(hKey, "MazingerVersion", NULL, &dwType, (LPBYTE) ach, &dw) == ERROR_SUCCESS)
		{
			ach[dw] = '\0';
			if (strcmp(ach, MAZINGER_VERSION_STR) != 0)
				needsUpdate = true;
		}
		else
			needsUpdate = true;
		RegCloseKey(hKey);
	}
	else
		needsUpdate = true;

	return needsUpdate;

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

		DWORD result = RegSetValueEx(hKey, "Userinit", 0, REG_SZ,
				(LPBYTE) achNewPath, strlen(achNewPath));

		if (result != ERROR_SUCCESS)
		{
			anyError = true;
			log("> ERROR: Cannot set registry entry userinit");
		}

		RegCloseKey(hKey);
		log("Configured winlogon");
	}

	else
	{
		anyError = true;
		log("> Cannot configure winlogon");
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
		log("Unable to register SoffidESSO\n");
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
		DWORD result = RegSetValueEx(hKey, "GinaDll", 0, REG_SZ, (LPBYTE) file,
				strlen(file));

		if (result != ERROR_SUCCESS)
		{
			anyError = true;
			log("Cannot configure ginadll");
		}

		RegCloseKey(hKey);
	}

	else
	{
		anyError = true;
		log("Cannot open HKLM\\software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
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


	if (pam)
	{
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
	}

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

	// IE Basic / NTLM authenticator
	const char *ssoClsid = "{9fe842aa-b8c4-4c93-a6b3-a957c0dd3037}";

	// SSO CREDENTIAL PROVIDER
	sprintf(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\%s",
			ssoClsid);
	strcpy(szValue, "Soffid basices Credential Provider");
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SHIRO CLSID
	sprintf(szKey, "CLSID\\%s", ssoClsid);
	strcpy(szValue, "Soffid basic Credential Provider");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	// SHIRO CLSID / Inprocserver32
	sprintf(szKey, "CLSID\\%s\\InprocServer32", ssoClsid);
	strcpy(szValue, file);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));
	strcpy(szValue, "Apartment");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, "ThreadingModel", REG_SZ,
			(void*) szValue, strlen(szValue));

	const char * soffidClsid = "{31543d94-5f56-4a1e-ab09-576e680203ee}";

	// SOFFID DIRECTORY CREDENTIAL PROVIDER
	sprintf(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\%s",
			soffidClsid);
	strcpy(szValue, "Soffid directory Credential Provider");
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	sprintf(szKey, "CLSID\\%s", soffidClsid);
	strcpy(szValue, "Soffid directory Credential Provider");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	sprintf(szKey, "CLSID\\%s\\InprocServer32", soffidClsid);
	strcpy(szValue, file);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));
	strcpy(szValue, "Apartment");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, "ThreadingModel", REG_SZ,
			(void*) szValue, strlen(szValue));

	// SOFFID SSO BASIC 44ee45c8-529b-4542-98ed-cfeceab1d4cc
	const char *basicClsid = "{44ee45c8-529b-4542-98ed-cfeceab1d4cc}";
	sprintf(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\%s",
			basicClsid);
	strcpy(szValue, "Soffid Basic SSO");
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	sprintf(szKey, "CLSID\\%s", basicClsid);
	strcpy(szValue, "Soffid Basic SSO");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));

	sprintf(szKey, "CLSID\\%s\\InprocServer32", basicClsid);
	strcpy(szValue, file);
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, NULL, REG_SZ, (void*) szValue,
			strlen(szValue));
	strcpy(szValue, "Apartment");
	HelperWriteKey(0, HKEY_CLASSES_ROOT, szKey, "ThreadingModel", REG_SZ,
			(void*) szValue, strlen(szValue));

	updateBasicCredentialProvider2(false);
	if (IsWow64())
		updateBasicCredentialProvider2(true);
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
		log("Cannot register shiro kabuto service");
		anyError = true;
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

	if (pam)
	{
		if (!anyError)
		{
			strcpy(achNewPath, getMazingerDir());
			strcat(achNewPath, "\\ShiroKabuto.exe");
			installShiroKabuto(achNewPath);
		}
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

		DWORD result = MoveFileEx(achFilePath, NULL, MOVEFILE_WRITE_THROUGH);

		if (result != 0)
		{
			log("> Removed");
		}

		else
		{
			result = MoveFileEx(achFilePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

			if (result)
			{
				log("> Remove scheduled");
			}

			else
			{
				log("> Remove failed");
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
		if (!quiet)
		{
			disableProgressWindow();
			MessageBox (NULL, "Installer file is corrupt", "Soffid ESSO installer", MB_OK|MB_ICONWARNING);
		}
		exit(2);
	}

	DWORD dwSize = SizeofResource(NULL, hRsrc);

	if (dwSize <= 0)
	{
		log(">> Wrong resource size %d", dwSize);
		notifyError();
		exit(2);
	}

	HGLOBAL hGlobal = LoadResource(NULL, hRsrc);

	if (hGlobal == NULL)
	{
		log(">> Cannot load resource %s", resource);
		notifyError();
		exit(2);
	}

	LPVOID lpVoid = LockResource(hGlobal);

	if (lpVoid == NULL)
	{
		log(">> Cannot lock resource %s", resource);
		notifyError();
		exit(2);
	}

	FILE *f = fopen(lpszFileName, "wb");
	if (f == NULL)
	{
		log(">> Cannot create file %s", lpszFileName);

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
					return false;

				case BZ_DATA_ERROR:
					log(">> BZIP ERROR: Corrupt resource");
					return false;

				case BZ_DATA_ERROR_MAGIC:
					log(">> BZIP ERROR: Compression error");
					return false;

				case BZ_MEM_ERROR:
					log(">> BZIP ERROR: Out of memory");
					return false;
			}
		}

		else
		{
			if (fwrite(lpVoid, dwSize, 1, f) != dwSize)
			{
				log(">> BZIP ERROR: Cannot write file");
				return false;
			}
		}

		fflush(f);
		fclose(f);
		return true;
	}
}

/**
 *
 * @param lpszTargetDir
 * @param lpszResourceName
 * @param lpszFileName
 * @param replaceAction => SOFT_REPLACE tries to delete
 *                         HARD_REPLACE tries to delete or rename
 *                         NO_REPLACE never replaces
 * @return true if the installation succeeds
 */

int SOFT_REPLACE = 0;
int HARD_REPLACE = 1;
int NO_REPLACE = 2;
bool installResource(const char *lpszTargetDir, const char *lpszResourceName,
		const char *lpszFileName, int replaceAction)
{
	bool cp = strcmp(lpszFileName, "SayakaCP.dll") == 0;
	bool cp32 = strcmp(lpszFileName, "SayakaCP32.dll") == 0;

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
		if (f == NULL || (replaceAction != NO_REPLACE && unlink(filePath.c_str()) == 0))
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
		else if (noGina && IsWindowsXP())
		{
			reboot = true;
			if (MoveFileEx(filePath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
			{
				log("> Old file %s will be deleted on reboot",
						oldFile.c_str());
			}

			// Despu\E9s de renombrar, puedo sustituir el fichero nuevo
			if (MoveFileEx(tempFilePath.c_str(), filePath.c_str(),
					MOVEFILE_DELAY_UNTIL_REBOOT))
			{
				log("> New file %s will be replaced on reboot",
						tempFilePath.c_str());
				success = true;
			}
			else
				log("> FAILED to replace file %s with %s", filePath.c_str(),
						tempFilePath.c_str());
		}
		// Si no puedo borrarlo, intento renombrarlo ahora
		else if (replaceAction == HARD_REPLACE
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
		log("> Success");
	}

	else
	{
		anyError = true;
	}

	fflush(stdout);

	return success;
}

bool installResource(const char *lpszTargetDir, const char *lpszFileName,
		const char *lpszTargetName)
{
	return installResource(lpszTargetDir, lpszFileName, lpszTargetName, HARD_REPLACE);
}

bool installResource(const char *lpszTargetDir, const char *lpszFileName, int replaceAction)
{
	return installResource(lpszTargetDir, lpszFileName, lpszFileName, replaceAction);
}

/**
 *
 * @param lpszTargetDir
 * @param lpszFileName
 * @return true if the installation succeeds
 */
bool installResource(const char *lpszTargetDir, const char *lpszFileName)
{
	return installResource(lpszTargetDir, lpszFileName, HARD_REPLACE);
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

	log("Installing MAZINGER %s into %s\n", MAZINGER_VERSION_STR, lpszDir);

	if (IsWow64())
	{
		installResource(NULL, "Mazinger64.exe", "Mazinger.exe");
		installResource(NULL, "MazingerHook64.dll", "MazingerHook.dll", SOFT_REPLACE);
		installResource(NULL, "MazingerHook.dll", "MazingerHook32.dll", SOFT_REPLACE);
		installResource(NULL, "KojiKabuto64.exe", "KojiKabuto.exe");
		installResource(NULL, "KojiHook64.dll", "KojiHook.dll");

		/* Browser extensions should not be replaced if a reboot is needed
		 *
		 */
		int replaceAction = reboot ? NO_REPLACE : HARD_REPLACE;
		installResource(NULL, "AfroditaC64.exe", "Afrodita-chrome.exe", replaceAction);

		installResource(NULL, "AfroditaE64.dll", "AfroditaE.dll", replaceAction);
		installResource(NULL, "AfroditaE.dll", "AfroditaE32.dll", replaceAction);
		installResource(NULL, "JetScrander.exe");
		installResource(NULL, "SayakaCP64.dll", "SayakaCP.dll");
		installResource(NULL, "SayakaCP.dll", "SayakaCP32.dll");

		installResource(NULL, "SoffidConfig64.exe", "SoffidConfig.exe");
	}
	else
	{
		installResource(NULL, "Mazinger.exe");
		installResource(NULL, "MazingerHook.dll", "MazingerHook.dll", false);
		installResource(NULL, "KojiKabuto.exe");
		installResource(NULL, "KojiHook.dll");

		int replaceAction = reboot ? NO_REPLACE : HARD_REPLACE;
		installResource(NULL, "AfroditaE.dll", replaceAction);
//		installResource(NULL, "AfroditaFC32.dll", "AfroditaFC.dll", replaceAction);
		installResource(NULL, "AfroditaC.exe", "Afrodita-chrome.exe", replaceAction);
		installResource(NULL, "JetScrander.exe");
		installResource(NULL, "SayakaCP.dll");

		installResource(NULL, "SoffidConfig.exe");
	}

	std::string system = getenv("SystemRoot");
	if (IsWow64())
	{
		std::string sys1 = system + "\\Sysnative";
		installResource (sys1.c_str(), "libwinpthread-1-64.dll", "libwinpthread-1.dll");
		std::string sys2 = system + "\\SysWOW64";
		installResource (sys2.c_str(), "libwinpthread-1-32.dll", "libwinpthread-1.dll");
	}
	else
	{
		installResource (NULL, "libwinpthread-1-32.dll", "libwinpthread-1.dll");
	}

	if (IsWindowsXP())
	{
		installResource(NULL, "Boss.exe");
	}

	installResource(NULL, "SayakaGina.dll");
	if (IsWow64())
		installResource(NULL, "ShiroKabuto64.exe", "ShiroKabuto.exe");
	else
		installResource(NULL, "ShiroKabuto.exe");
	installResource(NULL, "Nossori.exe");

//	installResource(NULL, "seycon.cer");
	installResource(NULL, "logon.tcl");
	installResource(NULL, "uninstall.exe");
	installResource(NULL, "sewashi.exe");
	installResource(NULL, "sewbr.dll");
	installResource(NULL, "profyumi.jar");
//	installResource(NULL, "afroditaFf.xpi");
	installResource(NULL, "afroditaFf2.xpi");


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
int RunProgram(const char *achString, const char *achDir)
{
	PROCESS_INFORMATION pInfo;	// Process information
	STARTUPINFO sInfo;					// Startup information
	DWORD dwExitStatus;				// Process execution result
	char command[4096];				// Command to run program

	sprintf(command, "%s\\%s", achDir, achString);

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
void RunConfigurationTool()
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
	bool uninstall = false;
	bool smartUpdate = false;

	log("Starting installer");
	for (int i = 0; i < argc; i++)
		log("Param %d: %s", i, argv[i]);
	// Read call arguments
	for (int i = 0; i < argc; i++)
	{
		if (stricmp(argv[i], "/msi") == 0 )
		{
			msi = true;
		}
		if (stricmp(argv[i], "/smartupdate") == 0 || stricmp(argv[i], "-smartupdate") == 0)
		{
			smartUpdate = true;
		}
		if (strcmp(argv[i], "/u") == 0 || strcmp(argv[i], "-u") == 0)
		{
			uninstall = true;
		}
		if (strcmp(argv[i], "/nopam") == 0 || strcmp(argv[i], "-nopam") == 0)
		{
			pam = false;
		}

		// Check quiet install method
		if (strcmp(argv[i], "/q") == 0 || strcmp(argv[i], "-q") == 0)
		{
			quiet = true;
		}

		// Check quiet install method
		if (strcmp(argv[i], "/updateconfig") == 0 || strcmp(argv[i], "-updateconfig") == 0)
		{
			updateConfigFlag = true;
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

		if (strcmp(argv[i], "/loginType") == 0 || strcmp(argv[i], "-loginType") == 0)
		{
			i++;
			if (i < argc)
			{
				loginType = argv[i];
			}
		}

		if (strcmp(argv[i], "/enableCloseSession") == 0 || strcmp(argv[i], "-enableCloseSession") == 0)
		{
			i++;
			if (i < argc)
			{
				enableCloseSession = argv[i];
			}
		}

		if (strcmp(argv[i], "/forceStartupLogin") == 0 || strcmp(argv[i], "-forceStartupLogin") == 0)
		{
			i++;
			if (i < argc)
			{
				forceStartupLogin = argv[i];
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


	int result;

	if (uninstall)
	{
		RunProgram((char *)"uninstall.exe", (char *)getMazingerDir());
		result = 0;
	}
	else if ( !smartUpdate || needsUpdate() )
	{
		log("Preparing install");
		log("Configured server %s", serverName);

		// Check pending operations
		if (checkPending)
		{
			bool pendingOperations = notifyPendingRenames();

			if (pendingOperations)
			{
				log("Installation aborted due to pending changes to apply");
				if (!quiet)
				{
					MessageBoxA(NULL,
							"A prior installation needed to reboot the system.\nReboot prior to install",
							"Soffid ESSO", MB_OK | MB_ICONEXCLAMATION);
				}

				exit(-1);
			}
		}

		result = install(true);

		if (noGina)
		{
			SetOriginalWinlogon();
		}

		if (result == 0 && serverName != NULL)
		{
			if (isUpdate && ! updateConfigFlag)
			{
				// Skip configuration
				setProgressMessage("Skipping server configuration");
			} else {
				setProgressMessage("Connecting to %s", serverName);

				if (!configure(getMazingerDir(), serverName))
					!quiet ? result = 1 : result = 3;
			}
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
	}
	ExitProcess ( result );
	return result;
}
