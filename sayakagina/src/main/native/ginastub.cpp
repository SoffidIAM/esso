/*++

 Copyright 1996 - 1997 Microsoft Corporation

 Module Name:

 ginastub.c

 Abstract:

 This sample illustrates a pass-thru "stub" gina which can be used
 in some cases to simplify gina development.

 A common use for a gina is to implement code which requires the
 credentials of the user logging onto the workstation.  The credentials
 may be required for syncronization with foreign account databases
 or custom authentication activities.

 In this example case, it is possible to implement a simple gina
 stub layer which simply passes control for the required functions
 to the previously installed gina, and captures the interesting
 parameters from that gina.  In this scenario, the existing functionality
 in the existent gina is retained.  In addition, the development time
 is reduced drastically, as existing functionality does not need to
 be duplicated.

 When dealing with credentials, take steps to maintain the security
 of the credentials.  For instance, if transporting credentials over
 a network, be sure to encrypt the credentials.

 Author:

 Scott Field (sfield)    18-Jul-96

 --*/

#include "sayaka.h"

#include "winwlx.h"
#include "ginastub.h"
#include "Utils.h"

#include <security.h>
#include <userenv.h>
#include <shlobj.h>
#include "SecurityHelper.h"
#include "EnvironmentHandler.h"
#include "LoggedOutManager.h"
#include "LockOutManager.h"
#include "Log.h"
#include "LoginStatus.h"
#include "RenameExecutor.h"
#include <MZNcompat.h>

static HANDLE hLsa = NULL;
MSV1_0_INTERACTIVE_PROFILE *pInteractiveProfile;
WLX_DISPATCH_VERSION_1_3 *pWinLogon;
HANDLE hWlx;
wchar_t *pUserEnvironment;
static Log g_log("GinaStub");
Pkcs11Configuration *p11Config = NULL;

extern bool doSayakaLogin(HANDLE hLsa, PLUID pAuthenticationId, PSID pLogonSid,
		PHANDLE phToken, PWLX_MPR_NOTIFY_INFO pMprNotifyInfo, PVOID *pProfile,
		MSV1_0_INTERACTIVE_PROFILE **pInteractiveProfile);

extern void dumpError(const char *achTitle);

extern "C" {
//
// Location of the real msgina.
//

#define REALGINA_PATH   TEXT("MSGINA.DLL")

//#define __TRACE {}
#define __TRACE {printf( "File: "__FILE__" Line: %d\n", __LINE__); }

//
// winlogon function dispatch table
//

PWLX_DISPATCH_VERSION_1_0 g_pWinlogon;

//
// Functions pointers to the real msgina which we will call.
//

BOOL
WINAPI
WlxNegotiate(DWORD dwWinlogonVersion, DWORD *pdwDllVersion) {

	Log l("ginastub");
	l.info("WlxNegotiate %d", dwWinlogonVersion);
	if (dwWinlogonVersion > WLX_VERSION_1_4)
		*pdwDllVersion = WLX_VERSION_1_4;
	else
		*pdwDllVersion = dwWinlogonVersion;
	return TRUE;

}

BOOL
WINAPI
WlxInitialize(LPWSTR lpWinsta, HANDLE hWlxParam, PVOID pvReserved,
		PVOID pWinlogonFunctions, PVOID *pWlxContext) {

	pWinLogon = (WLX_DISPATCH_VERSION_1_3*) pWinlogonFunctions;
	hWlx = hWlxParam;

#if 0
	HINSTANCE hDll;
	if( !(hDll = LoadLibrary( REALGINA_PATH )) ) {
		return FALSE;
	}

	PGWLXINITIALIZE GWlxInitialize;
	GWlxInitialize = (PGWLXINITIALIZE)GetProcAddress( hDll, "WlxInitialize" );
	if( !GWlxInitialize ) {
		return FALSE;
	}

	GWlxInitialize(lpWinsta, hWlxParam, pvReserved, pWinlogonFunctions, pWlxContext);
#endif

	ULONG_PTR dwOldValue;
	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, TRUE,
			&dwOldValue);
	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_SMART_CARD, FALSE,
			&dwOldValue);

	printf("Registering winlogon\n");
	if (!SecurityHelper::RegisterLogonProcess("Winlogon", &hLsa)) {
		printf("Registering winlogon FAILED\n");
		return FALSE;
	}

	Log::enableSysLog();
	g_log.info("WlxInitialized");

	return true;
}

VOID
WINAPI
WlxDisplaySASNotice(PVOID pWlxContext) {
	cancelMessage();

	ULONG_PTR dwOldValue;
	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, TRUE,
			&dwOldValue);
	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_SMART_CARD, FALSE,
			&dwOldValue);

	displayWelcomeMessage();

}

static void createEnvironment() {
	LPVOID pEnv2 = NULL;
	if (!CreateEnvironmentBlock(&pEnv2, LoginStatus::current.hToken, TRUE)) {
		printf("Cannot create environment block\n");
		pUserEnvironment = NULL;
		return;
	}
	pUserEnvironment = (wchar_t*) pEnv2;
}

static const wchar_t* getUserEnvironmentVariable(const wchar_t*variableName) {
	const wchar_t * env = pUserEnvironment;
	wchar_t varName[128];
	wcscpy(varName, variableName);
	wcscat(varName, L"=");
	int len = wcslen(varName);
	while (env[0] != '\0') {
		if (wcsncmp(env, varName, len) == 0) {
			env += len;
			return env;
		}
		env += wcslen(env) + 1;
	}
	return NULL;
}

int
WINAPI
WlxLoggedOutSAS(PVOID pWlxContext, DWORD dwSasType, PLUID pAuthenticationId,
		PSID pLogonSid, PDWORD pdwOptions, PHANDLE phToken,
		PWLX_MPR_NOTIFY_INFO pMprNotifyInfo, PVOID *pProfile) {

	cancelMessage();

	LoggedOutManager mgr;
	mgr.setAuthenticationId(pAuthenticationId);
	mgr.setDwSasType(dwSasType);
	mgr.setLogonSid(pLogonSid);
	mgr.setMprNotifyInfo(pMprNotifyInfo);
	mgr.setProfile(pProfile);
	mgr.setLsa(hLsa);
	mgr.setPhToken(phToken);
	mgr.setPdwOptions(pdwOptions);
	int result;
	result = mgr.process();

	if (result == WLX_SAS_ACTION_LOGON) {
		PWLX_DESKTOP pDesktop;
		Log log("GinaStub");
		log.info("Making desktop");
		displayMessage(NULL,
				MZNC_strtowstr(Utils::LoadResourcesString(15).c_str()).c_str());
	}

	return result;
}

bool runUserInit(STARTUPINFOW &si, PVOID pEnvironment) {
	bool ok = false;
	HKEY hKey;
	Log log("userinit");

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0,
			KEY_READ, &hKey) == ERROR_SUCCESS) {
		wchar_t *achPath;
		DWORD dwType;
		DWORD size = 0;
		if (RegQueryValueExW(hKey, L"UserInit", NULL, &dwType, (LPBYTE) NULL,
				&size) == ERROR_SUCCESS) {
			achPath = (wchar_t*) malloc((size + 1) * sizeof(wchar_t));
			RegQueryValueExW(hKey, L"UserInit", NULL, &dwType, (LPBYTE) achPath,
					&size);
			log.info(L"Userinit: %s", achPath);
			wchar_t *token = wcstok(achPath, L",");
			while (token != NULL) {
				PROCESS_INFORMATION pi;
				if (CreateProcessAsUserW(LoginStatus::current.hToken, NULL,
						token, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT,
						pEnvironment, 0, &si, &pi)) {
					log.info(L"Process created %s\n", token);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					ok = true;
				}
				token = wcstok(NULL, L",");
			}
			free(achPath);
		} else {
			printf("Cannot read userinit entry\n");
		}
	} else {
		printf("Cannot read userinit registry\n");
	}
	return ok;
}

BOOL
WINAPI
WlxActivateUserShell(PVOID pWlxContext, PWSTR pszDesktopName,
		PWSTR pszMprLogonScript, PVOID pEnvironment) {
	__TRACE
	int result = true;
	Log log("GinaStub");
	__TRACE

	log.info("Impersonating");
	if (!ImpersonateLoggedOnUser(LoginStatus::current.hToken)) {
		return false;
	}

	log.info("Impersonated");
	createEnvironment();
	log.info("Created environment");
	STARTUPINFOW si = { sizeof si, 0, pszDesktopName };
	PROCESS_INFORMATION pi;

	// Connectar unitat local
	const wchar_t *wchHomeDrive = getUserEnvironmentVariable(L"HOMEDRIVE");
	const wchar_t *wchHomeShare = getUserEnvironmentVariable(L"HOMESHARE");
	if (wchHomeShare != NULL && wchHomeDrive != NULL && wchHomeDrive[0] != L'\0' && wchHomeShare[0] != L'\0') {
		log.info(L"Connecting %s -> %s", wchHomeDrive, wchHomeShare);
		displayMessage(GetThreadDesktop(GetCurrentThreadId()),
				MZNC_strtowstr(Utils::LoadResourcesString(16).c_str()).c_str());
		NETRESOURCEW nr;
		nr.dwType = RESOURCETYPE_ANY;
		nr.lpLocalName = (wchar_t*) wchHomeDrive;
		nr.lpRemoteName = (wchar_t*) wchHomeShare;
		nr.lpProvider = NULL;
		WNetAddConnection2W(&nr, NULL, NULL, 0);
	}

	cancelMessage();

	log.info("Running userinit");
	result = runUserInit(si, pUserEnvironment);

	if (result && pszMprLogonScript != NULL) {
		log.info("Running logon script %s", pszMprLogonScript);
		CreateProcessAsUserW(LoginStatus::current.hToken, NULL,
				pszMprLogonScript, 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT,
				pUserEnvironment, 0, &si, &pi);
	}

	ULONG_PTR dwOldValue;
	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, TRUE,
			&dwOldValue);
	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_SMART_CARD, FALSE,
			&dwOldValue);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	RevertToSelf();

	log.info("End activate shell");
	return true;

}

int
WINAPI
WlxLoggedOnSAS(PVOID pWlxContext, DWORD dwSasType, PVOID pReserved) {
	g_log.info("WlxLoggedOnSAS");


	int r = WLX_SAS_ACTION_NONE;
	__TRACE
	if (dwSasType == WLX_SAS_TYPE_CTRL_ALT_DEL) {
		cancelMessage();
		r = securityDialog();
	}
	g_log.info("WlxLoggedOnSAS exit");

	if ( r == WLX_SAS_ACTION_LOGOFF ||
			r == WLX_SAS_ACTION_SHUTDOWN ||
			r == WLX_SAS_ACTION_FORCE_LOGOFF ||
			r == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF ||
			r == WLX_SAS_ACTION_SHUTDOWN_REBOOT ||
			r == WLX_SAS_ACTION_SHUTDOWN_SLEEP ||
			r == WLX_SAS_ACTION_SHUTDOWN_SLEEP2 ||
			r == WLX_SAS_ACTION_DELAYED_FORCE_LOGOFF) {
		displayMessage (NULL,
				MZNC_strtowstr(Utils::LoadResourcesString(17).c_str()).c_str());
	}

	return r;
}

VOID
WINAPI
WlxDisplayLockedNotice(PVOID pWlxContext) {
	displayLockMessage();
}

BOOL
WINAPI
WlxIsLockOk(PVOID pWlxContext) {
	__TRACE
	return true;
}

int
WINAPI
WlxWkstaLockedSAS(PVOID pWlxContext, DWORD dwSasType) {
	g_log.info("WkstaLockedSas");
	LockOutManager mgr;
	DWORD result;

	DWORD dwOldValue;
	DWORD dwNewValue;

	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, FALSE,
			&dwOldValue);

	if (dwSasType == WLX_SAS_TYPE_SC_INSERT)
		result = mgr.processCard();
	else
		result = mgr.process();

	g_log.info("WkstaLockedSas. Request =%d Response=%d ", (int) dwSasType,
			(int) result);

	g_log.info("WkstaLockedSas exit");

	pWinLogon->WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, dwOldValue,
			&dwNewValue);

	if ( result == WLX_SAS_ACTION_LOGOFF ||
			result == WLX_SAS_ACTION_SHUTDOWN ||
			result == WLX_SAS_ACTION_FORCE_LOGOFF ||
			result == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF ||
			result == WLX_SAS_ACTION_SHUTDOWN_REBOOT ||
			result == WLX_SAS_ACTION_SHUTDOWN_SLEEP ||
			result == WLX_SAS_ACTION_SHUTDOWN_SLEEP2 ||
			result == WLX_SAS_ACTION_DELAYED_FORCE_LOGOFF) {
		displayMessage (NULL,
				MZNC_strtowstr(Utils::LoadResourcesString(17).c_str()).c_str());
	}

	return result;
}

BOOL
WINAPI
WlxIsLogoffOk(PVOID pWlxContext) {

	displayMessage(NULL,
			MZNC_strtowstr(Utils::LoadResourcesString(17).c_str()).c_str());
	return true;
}

VOID
WINAPI
WlxLogoff(PVOID pWlxContext) {
	CloseHandle(LoginStatus::current.hToken);
	LoginStatus::current.hToken = NULL;
	cancelMessage();
}

VOID
WINAPI
WlxShutdown(PVOID pWlxContext, DWORD ShutdownType) {
	displayMessage(NULL,
			MZNC_strtowstr(Utils::LoadResourcesString(18).c_str()).c_str());

	Log l("Shutdown");
	l.info("Deleting p11 config");
	if (p11Config != NULL) {
		p11Config->stop();
		p11Config = NULL;
	}
	l.info("Deleted p11 config");
}

//
// NEW for version 1.1
//

BOOL
WINAPI
WlxScreenSaverNotify(PVOID pWlxContext, BOOL * pSecure) {
	__TRACE
//	*pSecure = true;
	__TRACE
	return TRUE;
}

BOOL
WINAPI
WlxStartApplication(PVOID pWlxContext, PWSTR pszDesktopName, PVOID pEnvironment,
		PWSTR pszCmdLine) {
	__TRACE
	if (!ImpersonateLoggedOnUser(LoginStatus::current.hToken)) {
		return false;
	}
	STARTUPINFOW si = { sizeof si, 0, pszDesktopName };
	PROCESS_INFORMATION pi;
	if (!CreateProcessAsUserW(LoginStatus::current.hToken, NULL, pszCmdLine, 0,
			0, FALSE, CREATE_UNICODE_ENVIRONMENT, pUserEnvironment, 0, &si,
			&pi)) {
		RevertToSelf();
		return false;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	RevertToSelf();

	return true;

}

BOOL WINAPI WlxDisplayStatusMessage(PVOID pWlxContext, HDESK hDesktop,
		DWORD dwThisOptions, PWSTR pTitle, PWSTR pMessage) {
	__TRACE
	displayMessage(hDesktop, pMessage);
	__TRACE
	return true;
}

BOOL WINAPI WlxGetStatusMessage(PVOID pWlxContext, DWORD *pdwOptions,
		PWSTR pMessage, DWORD dwBufferSize) {
	__TRACE
	pMessage[0] = '\0';
	return false;
}

BOOL WINAPI WlxNetworkProviderLoad(PVOID pWlxContext,
		PWLX_MPR_NOTIFY_INFO pNprNotifyInfo) {
	__TRACE
	BOOL x = 0;
	return x;
}

BOOL WINAPI WlxRemoveStatusMessage(PVOID pWlxContext) {

	__TRACE
	cancelMessage();
	__TRACE
	return true;
}

BOOL WINAPI WlxGetConsoleSwitchCredentials(PVOID pWlxContext, PVOID pInfo) {
	__TRACE
	return false;
}

}

HINSTANCE hSayakaDll;

extern "C" BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hSayakaDll = hinstDLL;
		g_log.info("*******\n\n\n");
		g_log.info("Started\n\n\n");
		RenameExecutor r;
		r.execute();
		p11Config = new Pkcs11Configuration;
	}
	if (dwReason == DLL_PROCESS_DETACH) {
		Log l("DllMain");
		g_log.info("Stopped\n\n\n");
		g_log.info("*******");
		cancelMessage();
	}
	return TRUE;
}

extern "C" void
WINAPI
Test() {
	MessageBox(NULL, "Test", "SayakaGina", MB_ICONINFORMATION | MB_OK);
}
