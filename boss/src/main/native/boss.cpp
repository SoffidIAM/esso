#include "boss.h"
#include <ssoclient.h>
#include <SeyconServer.h>
#include <wchar.h>
#include <ctype.h>
#include <string>
#include <MZNcompat.h>
#include <stdio.h>

#include "Utils.h"
#include "logindialog.h"

HINSTANCE g_hInstance = NULL;

void doExecute (const wchar_t *szUser, const wchar_t *szPass, const wchar_t *szProgram)
{
	STARTUPINFOW si;
	memset(&si, 0, sizeof si);
	si.cb = sizeof si;
	PROCESS_INFORMATION pi;
	std::wstring cmdLine = L"RUNDLL32.EXE SHELL32.DLL,ShellExec_RunDLL ";
	cmdLine += szProgram;
	if (CreateProcessWithLogonW((WCHAR*) szUser, NULL, (WCHAR*) szPass,
			LOGON_WITH_PROFILE, NULL, (wchar_t*) cmdLine.c_str(), 0, NULL, // ENV
			NULL, // CURRENT DIR,
			&si, &pi))
	{
	}
	else
	{
		std::string msg;
		Utils::getErrorMessage(msg, GetLastError());
		MessageBox(NULL, msg.c_str(), Utils::LoadResourcesString(1000).c_str(), MB_OK);
	}
}

extern "C" int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hInst2, LPSTR cmdLine,
		int nShow)
{

	g_hInstance = hInstance;

	int numArgs = 0;
	LPCWSTR wCmdLine = GetCommandLineW();
	LPWSTR* args = CommandLineToArgvW(wCmdLine, &numArgs);
	if (numArgs >= 2)
	{
		// Calcular parametros
		int i = 0;
		while (wCmdLine[i] == L' ')
			i++;
		if (wCmdLine[i] == L'"')
		{
			i++;
			while (wCmdLine[i] != L'"' && wCmdLine[i] != L'\0')
				i++;
			if (wCmdLine[i] == L'"')
				i++;
		}
		else
		{
			while (wCmdLine[i] != L' ' && wCmdLine[i] != L'\0')
				i++;
		}
		while (wCmdLine[i] == L' ')
			i++;
		std::wstring cmd = &wCmdLine[i];
		LoginDialog ld;
		ld.program = cmd;
		if (ld.show() == IDOK)
		{
			SeyconService service;

			std::wstring szHostName = MZNC_strtowstr(MZNC_getHostName());
			std::wstring wHost;
			for (unsigned int i = 0; i < szHostName.size() && szHostName[i] != '.'; i++)
			{
				wHost.append(1, towlower(szHostName[i]));
			}

			SeyconCommon::updateHostAddress();
			std::wstring wPass = service.escapeString(ld.password.c_str());
			SeyconResponse *response = service.sendUrlMessage(
					L"gethostadmin?user=%ls&pass=%ls&host=%ls", ld.user.c_str(),
					wPass.c_str(), szHostName.c_str());

			if (response == NULL)
				return false;
			else
			{
				std::string token = response->getToken(0);

				if (token == "OK")
				{
					std::wstring sUser;
					std::wstring sPassword;
					response->getToken(1, sUser);
					response->getToken(2, sPassword);
					delete response;

					SeyconCommon::info("Executing %ls as administrator", cmd.c_str());
					doExecute(sUser.c_str(), sPassword.c_str(), cmd.c_str());
					return true;
				}

				else
				{
					std::wstring cause;
					response->getToken(1, cause);

					MessageBoxW(NULL, cause.c_str(),
							MZNC_strtowstr(Utils::LoadResourcesString(1).c_str()).c_str(),
							MB_OK | MB_ICONEXCLAMATION);
					return false;
				}
			}
		}
	}

	return 0;
}

