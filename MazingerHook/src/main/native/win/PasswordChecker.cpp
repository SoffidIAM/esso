/*
 * Action.cpp
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#include <stddef.h>
#include <stdlib.h>
//#include "MazingerHook.h"
#include <MazingerInternal.h>
#include <Action.h>
#include "DomainPasswordCheck.h"
#include <vector>
#include <string>
#include <windows.h>
#include <winnetwk.h>
#include <lm.h>
#include "../resources/resource.h"
#include <SecretStore.h>
#include <ConfigReader.h>
#include <MazingerEnv.h>
#include <stdlib.h>


struct DialogInit {
	LPCWSTR domain;
	LPCWSTR pass1;
	LPCWSTR pass2;
};

static LPCWSTR getPasswordText (HWND hwnd, int id, LPCWSTR oldPass)
{
	if (oldPass != NULL) free ((void*)oldPass);

	HWND wndP = GetDlgItem (hwnd, id);
	int size = GetWindowTextLength (wndP);
	wchar_t *pass = (wchar_t*) malloc ((size+1) * (sizeof (wchar_t)));

	GetWindowTextW (wndP, pass, size+1);
	pass[size] = L'\0';
	return pass;
}


DWORD CALLBACK myDialogProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam
) {
    switch (message)
    {
    case WM_INITDIALOG:
		{
			struct DialogInit *init = (struct DialogInit*) lParam;
#ifdef _WIN64
			SetWindowLongPtr (hwnd, DWLP_USER, (LONG_PTR) init);
#else
			SetWindowLong (hwnd, DWL_USER, (DWORD) init);
#endif
			HWND hwnd2 = GetDlgItem (hwnd, IDC_DOMINI);
			SetWindowTextW (hwnd2, init->domain);

		}
        return TRUE;
    case WM_COMMAND:
    	if (wParam == IDOK)
    	{
#ifdef _WIN64
			struct DialogInit *init = (struct DialogInit*) GetWindowLongPtr(hwnd, DWLP_USER);
#else
			struct DialogInit *init = (struct DialogInit*) GetWindowLong(hwnd, DWL_USER);
#endif
			init->pass1 = getPasswordText (hwnd, IDC_PASSWORD1, init->pass1);
			init->pass2 = getPasswordText (hwnd, IDC_PASSWORD2, init->pass2);
			if (wcscmp (init->pass1, init->pass2) == 0)
				EndDialog(hwnd, 0);
			else
				MessageBox(
						hwnd,
						"La contrasenya i la seva confirmació no coincidèixen. Per favor, torni a escriure-les",
						"Atenció",
						MB_OK| MB_ICONWARNING);
    	}
    	if (wParam == IDCANCEL)
    		PostQuitMessage(1);
        return TRUE;
    case WM_HSCROLL:
        return 0;
    case WM_DESTROY:
        return TRUE;
    case WM_CLOSE:
        PostQuitMessage(1);
        DestroyWindow (hwnd);
        return TRUE;
    }
    return FALSE;
}


void checkDomainPassword (DomainPasswordCheck *dpc) {
	std::vector<wchar_t*> parsedServers;

	MZNSendDebugMessage("Testing domain %s",dpc->m_szDomain);
	if (dpc->m_szServers != NULL)
	{
		char *token = strtok (dpc->m_szServers, " ");
		while (token != NULL)
		{
		    wchar_t *wc = dpc->toUnicode(token);
			parsedServers.push_back(wc);
			token = strtok (NULL," ");
		}
	}

	for (std::vector<wchar_t*>::iterator it = parsedServers.begin();
			it != parsedServers.end();
			it ++)
	{
		wchar_t* nextServer = *it;
		NETRESOURCEW nr;
		nr.dwType = RESOURCETYPE_ANY;
		nr.lpLocalName = NULL;
		nr.lpRemoteName = nextServer;
		nr.lpProvider = NULL;
		SecretStore ss (MZNC_getUserName());


		wchar_t *wcUserSecret = dpc->toUnicode (dpc->m_szUserSecret != NULL ?
				dpc->m_szUserSecret: "user");
		wchar_t *wcPasswordSecret = dpc->toUnicode (dpc->m_szPasswordSecret != NULL ?
				dpc->m_szPasswordSecret: "password");

		wchar_t *wszUser = ss.getSecret(wcUserSecret);
		wchar_t *wszPassword = ss.getSecret(wcPasswordSecret);

		MZNSendDebugMessageW(L"Testing server %s", nextServer);
		MZNSendTraceMessageW(L"User=%s Password=%s", wszUser, wszPassword);

		DWORD result = WNetAddConnection2W (
				&nr,
				wszPassword,
				wszUser,
				CONNECT_TEMPORARY);
		if (result == NO_ERROR)
		{
			MZNSendDebugMessageW (L"Server %s OK", nextServer);
			break;
		}
		else
		{
			DWORD dwError;
			WCHAR wchErrorMessage[4096];
			WCHAR wchProvider[4096];

			WNetGetLastErrorW(&dwError, wchErrorMessage, sizeof wchErrorMessage,
					wchProvider, sizeof wchProvider);
			wchar_t *wcDomain = dpc->toUnicode (dpc->m_szDomain);

			if (result == ERROR_SESSION_CREDENTIAL_CONFLICT)
			{
				// Password incorrecte
				MZNSendDebugMessageW(L"WnetAddConnection2 %s. Error %d: Credential Conflict", nextServer, result);
			}
			else if (result == ERROR_LOGON_FAILURE)
			{
				MZNSendDebugMessageW(L"WnetAddConnection2 %s. Error %d: Logon Failure", nextServer, result);
			}
			else if (result == ERROR_PASSWORD_MUST_CHANGE || (result == ERROR_EXTENDED_ERROR && dwError == NERR_PasswordExpired))
			{
				MZNSendDebugMessageW(L"WnetAddConnection2 %s. Needs to change password", nextServer, result);
				struct DialogInit init;
				init.domain = wcDomain;

				int ok = 0;
				while (!ok) {
					init.pass1 = NULL;
					init.pass2 = NULL;
					result = DialogBoxParam (hMazingerInstance, (LPCSTR) IDD_DIALOG1, NULL, (DLGPROC) myDialogProc, (LPARAM)&init);

					if (result == 0)
					{

						MZNSendDebugMessageW(L"Changing password on %s for %s", wcDomain, wszUser);
						result = NetUserChangePassword(
								wcDomain,
								wszUser,
								wszPassword,
								init.pass1);

						if (result == NERR_Success)
						{
							ok = 1;
							ss.setSecret (wcPasswordSecret, init.pass1);
							// Executar les acciones
							dpc->executeActions("onChange");
						}
						else if (result == ERROR_ACCESS_DENIED)
						{
							ok = 1;
							MessageBox(NULL,
									"L'usuari no te permís per canviar la contrasenya",
									"Atenció", MB_OK| MB_ICONWARNING);
						}
						else if (result == ERROR_INVALID_PASSWORD)
						{
							ok = 0;
							MessageBox(NULL,
									"Hi ha hagut un error no previst. El sistema no accepta la contrasenya actual",
									"Atenció", MB_OK| MB_ICONWARNING);
						}
						else if (result == NERR_InvalidComputer)
						{
							ok = 1;
							MessageBox(NULL, "El servidor no respon", "Atenció", MB_OK| MB_ICONWARNING);
						}
						else if (result == NERR_NotPrimary)
						{
							ok = 1;
							MessageBox(NULL,
									"El servidor seleccionat no és un controlador de domini",
									"Atenció", MB_OK| MB_ICONWARNING);
						}
						else if (result == NERR_UserNotFound)
						{
							ok = 1;
							MessageBox(	NULL,
									"Hi ha hagut un error no previst. El sistema no coneix el codi d'usuari",
									"Atenció", MB_OK| MB_ICONWARNING);
						}
						else if (result == NERR_PasswordTooShort)
						{
							ok = 0;
							MessageBox(NULL,
									"La contrasenya no compleix els requisits de seguretat, o és una contrasenya ja utilitzada.\n"
									"Per favor, intenti-ho una altre vegada",
									"Avís", MB_OK| MB_ICONWARNING);
						}
						else
						{
							ok = 1;
							MessageBox(NULL,
									"Hi ha hagut un error no previst.",
									"Atenció",MB_OK| MB_ICONWARNING);
						}
					}

					if (init.pass1 != NULL) free ((void*) init.pass1);
					if (init.pass2 != NULL) free ((void*) init.pass2);
				}
				free (wcDomain);
				break ;

			} else if (result == ERROR_ACCOUNT_LOCKED_OUT){
				MZNSendDebugMessageW(L"WnetAddConnection2 %s. Error %d: Locked account", nextServer,
						result);
			} else if (result == ERROR_EXTENDED_ERROR){
				MZNSendDebugMessageW(L"WnetAddConnection2 %s. Extended Error %d: %s", nextServer,
						dwError,
						wchErrorMessage);
			} else {
				MZNSendDebugMessageW(L"WnetAddConnection2 %s. Error %d: %s", nextServer, result,
						wchErrorMessage);
			}
		}
		free (wcUserSecret);
		free (wcPasswordSecret);
		ss.freeSecret(wszUser);
		ss.freeSecret(wszPassword);

	}

	for (std::vector<wchar_t*>::iterator it = parsedServers.begin();
			it != parsedServers.end();
			it ++)
	{
		wchar_t* sz = *it;
		free(sz);
	}

}

extern "C" __declspec(dllexport) void MZNCheckPasswords() {
	MazingerEnv *pEnv = MazingerEnv::getDefaulEnv();
	if (pEnv->getData() != NULL) {
		ConfigReader *config = pEnv->getConfigReader();
		for (std::vector<DomainPasswordCheck*>::iterator it =
				config->getDomainPasswordChecks().begin(); it
				!= config->getDomainPasswordChecks().end(); it++) {
			DomainPasswordCheck *check = (*it);
			checkDomainPassword(check);
		}

	}
}
