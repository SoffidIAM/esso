#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <io.h>
#include <ctype.h>
#include <MazingerGuid.h>
#include <dirent.h>
#include <time.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlguid.h>
#include <stdlib.h>
#include <string>

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;

static void RecursiveRemoveDirectory (LPCSTR achName)
{
	DIR *d = opendir(achName);

	if (d != NULL)
	{
		struct dirent *entry = readdir(d);

		while (entry != NULL)
		{
			if (strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name) != 0)
			{
				char achFullName[4096];
				strcpy(achFullName, achName);
				strcat(achFullName, "\\");
				strcat(achFullName, entry->d_name);
				RecursiveRemoveDirectory(achFullName);
			}
			entry = readdir(d);
		}

		if (rmdir(achName))
			MoveFileExA(achName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}

	else
	{
		if (unlink(achName))
			MoveFileExA(achName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}
}

BOOL IsWindowsXP ()
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

BOOL IsWow64 ()
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

static DWORD Wow64Key (DWORD flag)
{
	return flag | (IsWow64() ? KEY_WOW64_64KEY : 0);
}

LPCSTR getMazingerDir ()
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

void log (const char *szFormat, ...)
{
	static FILE *logFile = NULL;

	if (logFile == NULL)
	{
		char achFile[1024];
		GetSystemDirectoryA(achFile, 1024);
		strcat(achFile, "\\mazinger-install.log");
		logFile = fopen(achFile, "a");
	}

	if (logFile != NULL)
	{
		time_t t;
		time(&t);
		struct tm *tm = localtime(&t);
		fprintf(logFile, "%d-%02d-%04d %02d:%02d:%02d UNINSTALL ", tm->tm_mday,
				tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min,
				tm->tm_sec);
		va_list v;
		va_start(v, szFormat);
		vfprintf(logFile, szFormat, v);
		va_end(v);
		if (szFormat[strlen(szFormat) - 1] != '\n')
			fprintf(logFile, "\n");
	}
}

typedef LONG (WINAPI *LPFN_REGDELETEKEYEX) (HKEY, LPCTSTR, REGSAM, DWORD);
static LPFN_REGDELETEKEYEX fnRegDeleteKeyEx = NULL;

void RegDelete64bitsKey (HKEY hKey, LPCSTR subkey)
{
	if (fnRegDeleteKeyEx == NULL)
	{
		fnRegDeleteKeyEx = (LPFN_REGDELETEKEYEX) GetProcAddress(
				GetModuleHandle("ADVAPI32.DLL"), "RegDeleteKeyEx");
	}

	log("> Deleting registry key %s", subkey);

	if (fnRegDeleteKeyEx == NULL)
		RegDeleteKeyA(hKey, subkey);

	else
		fnRegDeleteKeyEx(hKey, subkey, KEY_WOW64_64KEY, 0);
}

BOOL HelperWriteKey (int bits, HKEY roothk, const char *lpSubKey, LPCTSTR val_name,
		DWORD dwType, void *lpvData, DWORD dwDataSize)
{
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
						REG_OPTION_NON_VOLATILE, Wow64Key(KEY_ALL_ACCESS), NULL, &hk,
						NULL))
		{
			log("> ERROR Cannot create 64 bits key %s", lpSubKey);
			return FALSE;
		}

		if (ERROR_SUCCESS
				!= RegSetValueEx(hk, val_name, 0, dwType, (CONST BYTE *) lpvData,
						dwDataSize))
		{
			log("> ERROR Cannot set 64 bits value %s\\%s", lpSubKey, val_name);
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
			log("> ERROR Cannot create key %s", lpSubKey);
			return FALSE;
		}

		if (ERROR_SUCCESS
				!= RegSetValueEx(hk, val_name, 0, dwType, (CONST BYTE *) lpvData,
						dwDataSize))
		{
			log("> ERROR Cannot set value %s\\%s", lpSubKey, val_name);
			return FALSE;
		}

		if (ERROR_SUCCESS != RegCloseKey(hk))
			return FALSE;
	}

	return TRUE;
}

void notifyError ()
{
	LPSTR pstr;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			GetLastError(), 0, (LPSTR) &pstr, 0, NULL);
	printf("***************\n");
	printf("    ERROR      \n");
	printf("\n");
	printf("%s\n", pstr);
	printf("\n");
	printf("***************\n");
	log(">> %s", pstr);

	LocalFree(pstr);
}

void unregisterFFHook ()
{
	char szBuff[4096];
	//
	//write the default value
	//
	wsprintf(szBuff, "%s\\FFExtension", getMazingerDir());

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValueA(hKey, "{df382936-f24b-11df-96e1-9bf54f13e327}");
		RegCloseKey(hKey);
		log("Unregistered firefox extension");
	}

	else
	{
		log("Cannot unregister firefox extension");
	}
}

///////////////////////////////////////////////////////////////////////////////

void unregisterIEHook (void)
{
	//
	//As per COM guidelines, every self removable COM inprocess component
	//should export the function DllUnregisterServer for erasing all the
	//information that was printed into the registry
	//
	//

	char szKeyName[256] = "", szClsid[256] = "";
	WCHAR *lpwszClsid;

	log("Unregistering internet explorer extension");
	//
	//delete the ProgId entry
	//
	wsprintf(szKeyName, "%s\\%s", MznIEProgId, "CLSID");
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyName);
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKeyName);
	RegDeleteKey(HKEY_CLASSES_ROOT, MznIEProgId);
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, MznIEProgId);

	//
	//delete the CLSID entry for this COM object
	//
	StringFromCLSID(CLSID_Mazinger, &lpwszClsid);
	wsprintf(szClsid, "%S", lpwszClsid);
	wsprintf(szKeyName, "%s\\%s\\%s", "CLSID", szClsid, "InprocServer32");
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyName);
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKeyName);

	wsprintf(szKeyName, "%s\\%s\\%s", "CLSID", szClsid, "ProgId");
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyName);
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKeyName);

	wsprintf(szKeyName, "%s\\%s", "CLSID", szClsid);
	RegDeleteKey(HKEY_CLASSES_ROOT, szKeyName);
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKeyName);
}

void updateUserInit (const char *quitar, const char*poner)
{
	HKEY hKey;
	log("Configuring winlogon");
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
			if (strstr(token, quitar) == NULL && strstr(token, poner) == NULL) // Si no conté el que s'ha de llevar
			{
				strcat(achNewPath, ",");
				strcat(achNewPath, token);
			}
			token = strtok(NULL, ",");
		}

		log("> Userinit: %s", achNewPath);

		DWORD result = RegSetValueEx(hKey, "Userinit", 0, REG_SZ, (LPBYTE) achNewPath,
				strlen(achNewPath));
		if (result != ERROR_SUCCESS)
		{
			printf("Error setting registry value USERINIT \n");
		}

		RegCloseKey(hKey);
	}

	else
	{
		printf("Unable to deregister Mazinger\n");
	}
}

void updateGina (const char *file)
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0,
			Wow64Key(KEY_ALL_ACCESS), &hKey) == ERROR_SUCCESS)
	{
		DWORD result = RegSetValueEx(hKey, "GinaDll", 0, REG_SZ, (LPBYTE) file,
				strlen(file));
		if (result != ERROR_SUCCESS)
		{
			log("> Unable to set gina to MSGINA");
			printf("Error setting registry value GinaDll \n");
		}

		RegCloseKey(hKey);
	}

	else
	{
		log("> Unable to deregister gina");
	}
}

void removeCP ()
{
	char szKey[MAX_PATH] = "";
	char szEntry[MAX_PATH] = "";
	DWORD dwValue;

	log("Registering credential provider");

	// SMART CARD PROVIDER
	strcpy(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\"
			"{8bf9a910-a8ff-457f-999f-a5ca10b4a885}");
	strcpy(szEntry, "Disabled");
	dwValue = 0;
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, szEntry, REG_DWORD, (void*) &dwValue,
			sizeof dwValue);

	// SMARTCARD PIN PROVIDER
	strcpy(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\"
			"{94596c7e-3744-41ce-893e-bbf09122f76a}");
	strcpy(szEntry, "Disabled");

	dwValue = 0;
	HelperWriteKey(0, HKEY_LOCAL_MACHINE, szKey, szEntry, REG_DWORD, (void*) &dwValue,
			sizeof dwValue);

	// SAYAKA
	strcpy(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\"
			"{bdbb527b-40b1-44af-8c19-816f65c14016}");
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKey);

	strcpy(szKey, "CLSID\\"
			"{bdbb527b-40b1-44af-8c19-816f65c14016}");
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKey);
	strcpy(szKey, "Sayaka.Credential.Provider");
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKey);

	// SHIRO KABUTO
	strcpy(szKey, "Software\\Microsoft\\Windows\\"
			"CurrentVersion\\Authentication\\Credential Providers\\"
			"{eed73420-3734-493a-a0b1-78cc6e0f3e27}");
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKey);

	strcpy(szKey, "CLSID\\"
			"{e30dee24-e1aa-4880-a0ca-4a02e74f78f2}");
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKey);
	strcpy(szKey, "Sayaka.Credential.Provider");
	RegDelete64bitsKey(HKEY_CLASSES_ROOT, szKey);
}

bool unregisterShiroKabuto ()
{
	log("Unregistering ShiroKabuto");
	bool allowed = true;
	SC_HANDLE sch = OpenSCManager(NULL, // Current Machine
			NULL, // Active Services
			SC_MANAGER_CREATE_SERVICE);

	if (sch != NULL)
	{
		SC_HANDLE h = OpenService(sch, "ShiroKabuto", SC_MANAGER_ALL_ACCESS);
		if (h == NULL)
		{
			printf("WARNING !! Cannot remove ShiroKabuto service\n");
			notifyError();
		}

		else
		{
			SERVICE_STATUS ss;
			ControlService(h, SERVICE_CONTROL_STOP, &ss);
			if (DeleteService(h))
			{
				printf("ShiroKabuto service uninstalled\n");
			}

			else
			{
				printf("WARNING !! Cannot delete ShiroKabuto service\n");
				log("> Cannot delete ShiroKabuto service");
				notifyError();
			}
		}
	}

	else
	{
		printf("WARNING !! Cannot access ShiroKabuto service\n");
		log("> Cannot access shiro kabuto service");
		notifyError();
	}

	CloseServiceHandle(sch);

	return allowed;
}

void unregisterBoss ()
{
	log("Unregistering BOSS...\n");

	RegDeleteKeyA(HKEY_CLASSES_ROOT,
			"exefile\\shell\\Run with administrator permission\\command");
	RegDeleteKeyA(HKEY_CLASSES_ROOT,
			"exefile\\shell\\Run with administrator permission");

	TCHAR szPath[MAX_PATH];

	if (SUCCEEDED(
			SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS | CSIDL_FLAG_CREATE, NULL,
					0, szPath)))
	{
		std::string dir = szPath;
		dir += "\\Soffid ESSO";

		RecursiveRemoveDirectory(dir.c_str());
	}
}

void unregisterMazinger ()
{
	HKEY hKey;							// Registry handler
	char achNewPath[4096];

	strcpy(achNewPath, getMazingerDir());
	strcat(achNewPath, "\\KojiKabuto.exe");

	for (int i = 0; achNewPath[i]; i++)
		achNewPath[i] = tolower(achNewPath[i]);

	updateUserInit(achNewPath, "userinit");
	updateGina("MSGINA.DLL");
	removeCP();

	RegDelete64bitsKey(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SoffidESSO");

	// Delete previous mazinger version
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Soffid\\esso", 0,
			Wow64Key(KEY_ALL_ACCESS), &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, "MazingerVersion");

		RegCloseKey(hKey);
	}

	unregisterIEHook();
	unregisterFFHook();
	unregisterShiroKabuto();

	if (IsWindowsXP())
		unregisterBoss();
}

char achTmpDir[4096];

void uninstallFile (LPCSTR achFilePath)
{

	printf("Removing %s ...", achFilePath);
	log("Removing %s", achFilePath);

	DWORD result = MoveFileEx(achFilePath, NULL, MOVEFILE_WRITE_THROUGH);
	if (result != 0)
	{
		printf("Removed\n");
		log("> Removed");
	}

	else
	{
		result = MoveFileEx(achFilePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		if (result)
		{
			printf("Delayed\n");
			log("> Delayed");
		}

		else
		{
			printf("Failed\n");
			log("> Failed");
		}
	}

	fflush(stdout);
}

void uninstallResource (const char *lpszFileName)
{
	char achFilePath[5096];
	LPCSTR lpszDir = getMazingerDir();
	strcpy(achFilePath, lpszDir);
	strcat(achFilePath, "\\");
	strcat(achFilePath, lpszFileName);

	uninstallFile(achFilePath);
}

bool notifyPendingRenames ()
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
				!= RegQueryValueEx(hKey, "PendingFileRenameOperations", NULL, &dwType,
						(LPBYTE) achPath, &size))
		{
			char *buffer = (char*) malloc(size + 1);

			if (ERROR_SUCCESS
					== RegQueryValueEx(hKey, "PendingFileRenameOperations", NULL,
							&dwType, (LPBYTE) buffer, &size))
			{
				const char *mznDir = getMazingerDir();
				int mznDirLen = strlen(mznDir);
				for (int i = 0; buffer[i]; i++)
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

int uninstall (int argc, char **argv)
{
	log("************************");
	log("Uninstaling");
	log("************************");
	uninstallResource("Mazinger.exe");
	uninstallResource("MazingerHook32.dll");
	uninstallResource("MazingerHook.dll");
	uninstallResource("KojiKabuto.exe");
	uninstallResource("KojiHook.dll");
	uninstallResource("profyumi.jar");
	uninstallResource("uninstall.exe");
	uninstallResource("seycon.cer");
	uninstallResource("logon.tcl");
	uninstallResource("JetScrander.exe");
	uninstallResource("FFExtension\\chrome\\content\\about.xul");
	uninstallResource("FFExtension\\chrome\\content\\ff-overlay.xul");
	uninstallResource("FFExtension\\chrome\\content\\overlay.js");
	uninstallResource("FFExtension\\chrome\\locale\\en-US\\about.dtd");
	uninstallResource("FFExtension\\chrome\\locale\\en-US\\overlay.properties");
	uninstallResource("FFExtension\\default\\preferences\\prefs.js");
	uninstallResource("FFExtension\\chrome.manifest");
	uninstallResource("FFExtension\\install.rdf");
	uninstallResource("FFExtension\\components\\AfroditaF5.dll");
	uninstallResource("FFExtension\\components\\AfroditaF.dll");
	uninstallResource("FFExtension\\components\\AfroditaF.xpt");
	uninstallResource("AfroditaE.dll");
	uninstallResource("AfroditaE32.dll");
	uninstallResource("SayakaGina.dll");
	uninstallResource("SayakaCP.dll");
	uninstallResource("planeador.wav");
	uninstallResource("ShiroKabuto.exe");
	uninstallResource("ShiroKabuto.exe");
	uninstallResource("Afrodita-crhome.exe");
	uninstallResource("Afrodita-crhome.manifest");

	if (IsWow64())
	{
		uninstallResource("MazingerHook32.dll");
		uninstallResource("AfroditaE32.dll");
	}

	if (IsWindowsXP())
	{
		uninstallResource("Boss.exe");
	}

	unregisterMazinger();

	RecursiveRemoveDirectory(getMazingerDir());

	notifyPendingRenames();

	char ach[4096];
	printf("Press ENTER to continue\n");
	gets(ach);
	return 0;
}

extern "C" int main (int argc, char **argv)
{
	return uninstall(argc, argv);
}
