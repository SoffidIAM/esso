#include <stdio.h>
#include <ssoclient.h>
#include "passwordbank.h"
#include "httpHandler.h"

#ifdef WIN32
#include <windows.h>
#include <io.h>

static const char* getPasswordBankFile ()
{
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
			0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		static char achPath[4096] = "";
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		size = sizeof achPath;
		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType, (LPBYTE) achPath, &size);
		RegCloseKey(hKey);
		strcat(achPath,
				"\\PasswordBank\\PasswordBank Single Sign On Client\\PasswordBank_SSOClient.exe");
		return achPath;
	}
	else
	{
		return FALSE;
	}
}

int SeyconSession::isPasswordBankInstalled ()
{
	SeyconCommon::debug("IS PASSWORD BANK INSTALLED? \n");
	LPCSTR lpszPath = getPasswordBankFile();
	SeyconCommon::debug("Looking for pb on %s\n", lpszPath);
	if (lpszPath == NULL)
		return FALSE;
	else
	{
		FILE *f = fopen(lpszPath, "r");
		if (f != NULL)
		{
			SeyconCommon::debug("PB FOUND\n");
			fclose(f);
			return TRUE;
		}
		else
		{
			SeyconCommon::debug("PB NOT FOUND\n");
			return FALSE;
		}
	}
}

ServiceIteratorResult SeyconSession::launchPasswordBank (const char* wchHostName,
		int dwPort)
{
	SeyconService service;
	SeyconResponse *response = service.sendUrlMessage(
			L"/kerberosLogin?action=pbticket&challengeId=%hs", sessionKey.c_str());
	// Parse results
	if (response != NULL)
	{
		std::string status = response->getToken(0);
		if (status == "OK")
		{
			std::wstring ticket;
			response->getToken(1, ticket);
			// Salvar el ticket
			LPCSTR profile = getenv("USERPROFILE");
			if (profile != NULL)
			{
				std::string dir;
				dir.assign(profile);
				dir.append("\\ssoclient");
#ifdef __GNUC__
				mkdir(dir.c_str());
#else
				CreateDirectoryA(achDir, NULL);
#endif
				dir.append("\\ssoticket");
				FILE *f = fopen(dir.c_str(), "wb");
				fwrite(ticket.c_str(), sizeof(wchar_t), ticket.size(), f);
				fclose(f);
				LPCSTR path = getPasswordBankFile();

				STARTUPINFO si;
				memset(&si, 0, sizeof si);
				si.cb = sizeof si;

				PROCESS_INFORMATION pi;
				CreateProcess(path, NULL, // CommandLine
						NULL, // Process attr
						NULL, // Thread attr
						FALSE, // Inherit handles
						NULL, // Flags
						NULL, // Environment
						NULL, // Current dir
						&si,  // Startup info
						&pi); // Resturn info
				return SIR_SUCCESS;
			}
		}
	}
	return SIR_ERROR;
}
#else
int SeyconSession::isPasswordBankInstalled ()
{
	return false;
}

ServiceIteratorResult SeyconSession::launchPasswordBank (const char* achHostName, int dwPort)
{
	return SIR_ERROR;
}

#endif
