#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include <ssoclient.h>
#include <time.h>
#include <msg.h>
#include <ctype.h>
#include <string>
#include <MZNcompat.h>
#include "Tokenizer.h"

// #define TRAMPOTA

static const char *DEF_SHIRO_LOG_FILE = "c:\\temp\\shiro.log";

const wchar_t *achShiroAccount = L"ShiroKabuto";
const char *shiroIdEntry = "ShiroId";

wchar_t achAdminGroup[200] = L"";
const int PASSWORD_LENGTH = 15;
wchar_t achPassword[PASSWORD_LENGTH + 1];
bool debug = false;

HANDLE hEventLog;

// una semana = 7 x 24 x 60 x 60
#define PASSWORD_DURATION 604800

void OpenEventLog ()
{
	char achModule[MAX_PATH];
	GetModuleFileNameA(NULL, achModule, sizeof achModule);
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\ShiroKabuto", 0,
			(LPSTR) "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,
			NULL) == ERROR_SUCCESS)
	{
		DWORD dwValue = 1;
		RegSetValueEx(hKey, "CategoryCount", 0, REG_DWORD, (LPBYTE) &dwValue,
				sizeof dwValue);
		RegSetValueEx(hKey, "CategoryMessageFile", 0, REG_SZ, (LPBYTE) achModule,
				(DWORD) strlen(achModule));
		RegSetValueEx(hKey, "EventMessageFile", 0, REG_SZ, (LPBYTE) achModule,
				(DWORD) strlen(achModule));
		dwValue = EVENTLOG_AUDIT_SUCCESS | EVENTLOG_ERROR_TYPE
				| EVENTLOG_INFORMATION_TYPE;
		RegSetValueEx(hKey, "CategoryCount", 0, REG_DWORD, (LPBYTE) &dwValue,
				sizeof dwValue);
		RegCloseKey(hKey);
	}

	hEventLog = RegisterEventSource(NULL, "ShiroKabuto");
}

const wchar_t * errorMessage (int error)
{
	switch (error)
	{
		case NERR_InvalidComputer:
			return L"Invalid Computer";

		case NERR_NotPrimary:
			return L"Not a domain controller";

		case NERR_GroupExists:
			return L"Group already exists";

		case NERR_UserExists:
			return L"User already exists";

		case NERR_PasswordTooShort:
			return L"Password too short";

		default:
			static wchar_t msg[200];
			if (error < NERR_BASE)
			{
				wchar_t *buffer;
				if (FormatMessageW(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL, error, 0, (LPWSTR) &buffer, 0, NULL))
				{
					wcsncpy(msg, buffer, 199);
					msg[199] = L'\0';
					return msg;
				}
			}
			swprintf(msg, L"Uknown error %d", error);
			return msg;
	}
}

const wchar_t* getAdminGroup ()
{
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = { SECURITY_NT_AUTHORITY };
	PSID pSIDAdmin;
	// Create a SID for the BUILTIN\Administrators group.
	if (!AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSIDAdmin))
	{
		printf("AllocateAndInitializeSid (Admin) error %u\n",
				(unsigned int) GetLastError());
		return NULL;
	}

	wchar_t achName[200];
	DWORD size = sizeof achName;
	wchar_t achDomain[200];
	DWORD size2 = sizeof achDomain;
	SID_NAME_USE use;
	if (LookupAccountSidW(NULL, pSIDAdmin, achName, &size, achDomain, &size2, &use))
	{
		wprintf(L"Admin group = %s\n", achName);
		wcscpy(achAdminGroup, achName);
	}
	return achAdminGroup;
}

const wchar_t* generatePassword ()
{
	int i;
	for (i = 0; i < PASSWORD_LENGTH; i++)
	{
		do
		{
			int r = rand() % 62;
			if (r < 10)
				achPassword[i] = '0' + r;
			else if (r < 36)
				achPassword[i] = 'a' + r - 10;
			else
				achPassword[i] = 'A' + r - 36;
			// Evitar caracteres conflictivos
		} while (achPassword[i] == '1' || achPassword[i] == 'l' || achPassword[i] == '0'
				|| achPassword[i] == 'O' || achPassword[i] == 'I');
	}
	achPassword[i] = L'\0';
	return achPassword;
}

/** @brief Is machine on domain
 *
 * Implements the functionality to check if the current machine are on domain.
 * @return
 * <ul>
 * 		<li>
 * 			@code true @endcode
 * 			If the current machine are on domain.
 * 		</li>
 *
 * 		<li>
 * 			@code false @endcode
 * 			If the current machine are not on domain.
 * 		</li>
 * 	</ul>
 */
bool isInDomain ()
{
	bool inDomain = false;
	LPWSTR buffer = NULL;
	NETSETUP_JOIN_STATUS dwJoinStatus = NetSetupUnknownStatus;
	NetGetJoinInformation(NULL, &buffer, &dwJoinStatus);

	if (dwJoinStatus == NetSetupDomainName)
	{
		inDomain = true;
	}
	else
	{
		inDomain = false;
	}

	if (buffer != NULL)
	{
		NetApiBufferFree(buffer);
	}

	return inDomain;
}

bool notifyNewPassword (const wchar_t* password)
{
	SeyconService service;
	std::string host;
	bool updateShiroId = false;

	std::string shiroId;
	SeyconCommon::readProperty(shiroIdEntry, shiroId);

	std::string serial;
	SeyconCommon::readProperty("serialNumber", serial);

	if (shiroId.empty())
	{
		updateShiroId = true;
		for (int i = 0; i < 12; i++)
		{
			int n = rand() % 10;
			shiroId.append(1, '0' + n);
		}
	}

	wchar_t wchHostName[128];
	DWORD dwSize = 128;
	GetComputerNameExW(ComputerNameDnsFullyQualified, wchHostName, &dwSize);
	for (int i = 0; wchHostName[i]; i++)
		wchHostName[i] = towlower(wchHostName[i]);

	SeyconResponse *resp = service.sendUrlMessage(
			L"/sethostadmin?host=%s&user=%s&pass=%s&shiroId=%s&serial=%s",
			service.escapeString(wchHostName).c_str(), achShiroAccount,
			service.escapeString(password).c_str(),
			service.escapeString(shiroId.c_str()).c_str(),
			service.escapeString(serial.c_str()).c_str());
	if (resp == NULL)
		return false;

	std::string status = resp->getToken(0);
	if (status == "OK")
	{
		const wchar_t *params = achShiroAccount;
		ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, SHIRO_CATEGORY, SHIRO_SUCCESS,
				NULL, 1, 0, (LPCWSTR*) &params, NULL);
		delete resp;
		if (updateShiroId)
		{
			SeyconCommon::writeProperty(shiroIdEntry, shiroId.c_str());
		}
		return true;
	}

	else
	{
		std::wstring cause;
		resp->getToken(1, cause);
		const wchar_t *params = cause.c_str();
		delete resp;

		if (!ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, SHIRO_CATEGORY,
				SHIRO_REMOTEERROR, NULL, 1, 0, (LPCWSTR*) &params, NULL))
		{
			wprintf(L"Error generant log: %s", errorMessage(GetLastError()));
		}
		return false;
	}
}

bool canChangePassword (USER_INFO_2*ui2)
{
	bool allowed = true;

// Verificar si esta en la lista de Excluidos
	std::string localUsers;
	SeyconCommon::readProperty("localUsers", localUsers);
	std::string user = MZNC_wstrtostr(ui2->usri2_name);
	if (!localUsers.empty())
	{
		Tokenizer tok(localUsers, " ,");
		while (tok.NextToken())
		{
			const std::string token = tok.GetToken();
			if (token == user)
			{
				return false;
			}
		}
	}

	SC_HANDLE sch = OpenSCManager(NULL, // Current Machine
			NULL, // Active Services
			SC_MANAGER_ENUMERATE_SERVICE);
	if (sch != NULL)
	{
		DWORD dwBytesNeeded;
		DWORD dwServices;
		EnumServicesStatusW(sch, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0,
				&dwBytesNeeded, &dwServices, NULL);
		if (ERROR_MORE_DATA == GetLastError())
		{
			ENUM_SERVICE_STATUSW* services = (ENUM_SERVICE_STATUSW*) malloc(
					dwBytesNeeded);
			EnumServicesStatusW(sch, SERVICE_WIN32, SERVICE_STATE_ALL, services,
					dwBytesNeeded, &dwBytesNeeded, &dwServices, NULL);
			for (unsigned int i = 0; i < dwServices; i++)
			{
				SC_HANDLE sh = OpenServiceW(sch, services[i].lpServiceName,
						SC_MANAGER_CONNECT);
				DWORD dwSize;
				QueryServiceConfigW(sh, NULL, 0, &dwSize);
				if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
				{
					QUERY_SERVICE_CONFIGW *serviceConfig =
							(QUERY_SERVICE_CONFIGW*) malloc(dwSize);

					QueryServiceConfigW(sh, serviceConfig, dwSize, &dwSize);
					if (serviceConfig->lpServiceStartName[0] == L'.'
							&& serviceConfig->lpServiceStartName[1] == L'\\'
							&& wcscmp(&serviceConfig->lpServiceStartName[2],
									ui2->usri2_name) == 0)
					{
						allowed = false;
					}

					free(serviceConfig);
				}

				CloseServiceHandle(sh);
			}

			free(services);
		}
	}
	CloseServiceHandle(sch);

	return allowed;
}

bool isShiroActive ()
{
	std::string shiroId;

	if (SeyconCommon::readProperty(shiroIdEntry, shiroId))
		return true;

	else
		return false;
}

void updateAccount (USER_INFO_2*ui2, const wchar_t* password, bool force)
{
	if (wcscmp(ui2->usri2_name, achShiroAccount) != 0
			&& (ui2->usri2_password_age > PASSWORD_DURATION || force))
	{
		if (isShiroActive())
		{
			if (canChangePassword(ui2))
			{
				// Ha de canviar la contrasenya
				wprintf(L"** Changing password\n");
				ui2->usri2_password = (wchar_t*) password;
				DWORD error;
				int result = NetUserSetInfo(NULL, ui2->usri2_name, 2, (LPBYTE) ui2,
						&error);
				if (result != NERR_Success)
				{
					wprintf(L"Unable to update %s Account: %s\n", ui2->usri2_name,
							errorMessage(result));
					const wchar_t *params[2] = { ui2->usri2_name, errorMessage(result) };
					ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, SHIRO_CATEGORY,
							SHIRO_LOCALERROR, NULL, 2, 0, (LPCWSTR*) &params, NULL);
				}

				else
				{
					const wchar_t *params = ui2->usri2_name;
					ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, SHIRO_CATEGORY,
							SHIRO_SUCCESS, NULL, 1, 0, (LPCWSTR*) &params, NULL);
				}
			}

			else
			{
				wprintf(L"Has pending services\n");
				const wchar_t *params[2] = { ui2->usri2_name, L"Has pending services" };
				ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, SHIRO_CATEGORY,
						SHIRO_LOCALERROR, NULL, 2, 0, (LPCWSTR*) &params, NULL);
			}
		}
	}
}

void init ()
{
	OpenEventLog();
}

void updateOtherAccounts (const wchar_t *password, bool force)
{
	if (isInDomain())
	{

		NET_API_STATUS result;
		LPBYTE buffer;
		DWORD read;
		DWORD maxRead;
		DWORD resumeHandle = 0;
		result = NetUserEnum(
				NULL, // Local Host
				2, // level
				FILTER_NORMAL_ACCOUNT, &buffer, MAX_PREFERRED_LENGTH, &read, &maxRead,
				&resumeHandle);
		if (result == NERR_Success)
		{
			USER_INFO_2 *ui2 = (USER_INFO_2*) buffer;
			for (unsigned int i = 0; i < read; i++)
			{
				wprintf(L"\n\nUser: %s - %s\n", ui2[i].usri2_name, ui2[i].usri2_comment);
				updateAccount(&ui2[i], password, force);
			}
		}

		else
		{
			wprintf(L"Unable to enum accounts: %s\n", errorMessage(result));
		}
	}
}

void loop ()
{
	LPBYTE buffer;

#ifdef TRAMPOTA
	{
		DWORD result = NetUserGetInfo(
				NULL, // Local Host
				L"Administrador",
				2,// level
				&buffer);

		USER_INFO_2* ui2 = (USER_INFO_2*) buffer;
		if (result != NERR_Success)
		{
			wprintf (L"Unable to find user Administrador: %s\n",
					errorMessage (result));
			return;
		}

		else
		{
			ui2->usri2_password = (wchar_t*) L"Tramp0ta";
			DWORD error;
			int result = NetUserSetInfo(NULL, ui2->usri2_name, 2, (LPBYTE) ui2, &error );
			if (result != NERR_Success)
			{
				wprintf (L"Unable to update Administrador: [%s]\n",
						errorMessage (result));
				return;
			}
			else
			{	00
				wprintf (L"Password de administrador cambiada\n");
			}
		}
	}

#endif

	if (debug)
		printf("Locating user ShiroKabuto\n");

	DWORD result = NetUserGetInfo(NULL, // Local Host
			achShiroAccount, 2, // level
			&buffer);

	if (result == NERR_UserNotFound)
	{
		if (debug)
			printf("Creating user ShiroKabuto\n");

		const wchar_t *pass = generatePassword();
		if (notifyNewPassword(pass))
		{
			USER_INFO_2 ui2;
			memset(&ui2, 0, sizeof ui2);
			ui2.usri2_name = (wchar_t*) achShiroAccount;
			ui2.usri2_password = (wchar_t*) pass;
			ui2.usri2_priv = USER_PRIV_USER;
			ui2.usri2_comment = (wchar_t*) L"Autogenerated by Mazinger";
			ui2.usri2_flags = UF_NORMAL_ACCOUNT;
			ui2.usri2_full_name = (wchar_t*) L"Shiro Kabuto";
			ui2.usri2_acct_expires = TIMEQ_FOREVER;
			DWORD error;
			result = NetUserAdd(NULL, 2, (LPBYTE) &ui2, &error);
			if (result != NERR_Success)
			{
				wprintf(L"Unable to create Shiro Account: %s\n",
						errorMessage(result));
				return;
			}

			LOCALGROUP_MEMBERS_INFO_3 lgmi3;
			lgmi3.lgrmi3_domainandname = (wchar_t*) achShiroAccount;

			result = NetLocalGroupAddMembers(NULL, getAdminGroup(), 3,
					(LPBYTE) &lgmi3, 1);
			if (result != NERR_Success)
			{
				wprintf(L"Unable to add Shiro Account to Administrators group: %s\n",
						errorMessage(result));
				return;
			}

			std::string currentHostName = MZNC_getHostName();
			SeyconCommon::writeProperty("ShiroHostName", currentHostName.c_str());
			updateOtherAccounts(pass, false);
		}
	}

	else if (result == NERR_Success)
	{

		std::string oldHostName;
		std::string currentHostName = MZNC_getHostName();
		SeyconCommon::readProperty("ShiroHostName", oldHostName);
		bool force;

		if (debug)
			printf("Updating user Shirokabuto\n");

		int firstDot = currentHostName.find('.');
		if (firstDot != std::string::npos)
		{
			currentHostName = currentHostName.substr(0, firstDot);
		}

		firstDot = oldHostName.find('.');
		if (firstDot != std::string::npos)
		{
			oldHostName = oldHostName.substr(0, firstDot);
		}

		if (oldHostName.length() > 0 && oldHostName != currentHostName)
		{
			SeyconCommon::info(
					"Host name changed. Shiro accounts needs to change password");
			force = true;
		}

		else
		{
			force = false;
		}

		if (debug)
			printf("Buffer = %lx\n", buffer);

		USER_INFO_2 *ui2 = (USER_INFO_2*) buffer;
		if (debug)
			printf("ui2 = %lx\n", ui2);

		if (debug)
			printf("ui2->name = %ls\n", ui2->usri2_name);

		if (ui2->usri2_password_age > PASSWORD_DURATION || force)
		{
			// Ha de canviar la contrasenya
			bool repeat = false;

			do
			{
				repeat = false;
				const wchar_t *pass = generatePassword();
				if (debug)
					printf("Sending passsword %ls\n", pass);

				if (debug)
					printf("ui2->name = %ls\n", ui2->usri2_name);

				if (notifyNewPassword(pass))
				{
					if (debug)
						printf("Sent passsword %ls\n", pass);

					if (debug)
						printf("ui2 = %lx\n", ui2);

					if (debug)
						printf("ui2->name = %ls\n", ui2->usri2_name);

					ui2->usri2_password = (wchar_t*) pass;

					if (debug)
						printf("ui2 = %lx\n", ui2);

					if (debug)
						printf("ui2->name = %ls\n", ui2->usri2_name);

					ui2->usri2_flags = UF_NORMAL_ACCOUNT;

					if (debug)
						printf("ui2 = %lx\n", ui2);

					if (debug)
						printf("ui2->name = %ls\n", ui2->usri2_name);

					DWORD error;

					if (debug)
						printf("Changing password for %ls\n", achShiroAccount);

					result = NetUserSetInfo(NULL, achShiroAccount, 2, (LPBYTE) ui2,
							&error);

					if (debug)
						printf("Changed password?\n");

					if (result != NERR_Success)
					{
						if (debug)
							printf("Password change failed\n");

						wprintf(L"Unable to update Shiro account: %s\n",
								errorMessage(result));

						const wchar_t *params[2] = { achShiroAccount, errorMessage(
								result) };
						ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, SHIRO_CATEGORY,
								SHIRO_LOCALERROR, NULL, 2, 0, (LPCWSTR*) &params,
								NULL);
						repeat = true;
					}

					else
					{
						if (debug)
							printf("Password changed\n");

						updateOtherAccounts(pass, force);

						if (debug)
							printf("Password changed for other administrators\n");

						SeyconCommon::writeProperty("ShiroHostName",
								currentHostName.c_str());
					}
				}
			} while (repeat);
		}

		NetApiBufferFree(buffer);
	}

	else
	{
		const wchar_t *params[2] = { L"Error locating user ShiroKabuto",
				errorMessage(result) };
		ReportEventW(hEventLog, EVENTLOG_ERROR_TYPE, SHIRO_CATEGORY,
				SHIRO_LOCALERROR, NULL, 2, 0, (LPCWSTR*) &params, NULL);

		if (debug)
			printf("Error locating user ShiroKabuto: %ls\n", errorMessage(result));
	}
}

SERVICE_STATUS_HANDLE serviceStatusHandle;
SERVICE_STATUS serviceStatus;
HANDLE hStopEvent;

void updateServiceStatus (DWORD dwStatus)
{
	serviceStatus.dwCurrentState = dwStatus;
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

// Operaciones de control sobre el servicio
VOID __stdcall shiroServiceCtrlHandler ( IN DWORD Opcode)
{
	switch (Opcode)
	{
		case SERVICE_CONTROL_STOP:
			updateServiceStatus(SERVICE_STOP_PENDING);
			SetEvent(hStopEvent);
			return;
	}

	SetServiceStatus(serviceStatusHandle, &serviceStatus);
	return;
}

time_t firstTime = 0;
static void updateHostAddress ()
{
	time_t t;
	time(&t);
	if (t - firstTime > 180) // 180 segundos = 30 minutos
	{
		SeyconCommon::updateHostAddress();
	}
}

void __stdcall shiroServiceStart (DWORD argc, LPTSTR *argv)
{
#if 0
	fclose (stdout);
	FILE *f = fopen (DEF_SHIRO_LOG_FILE, "a");

	if (f != NULL)
	setbuf(f, NULL);
#endif

	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	serviceStatusHandle = RegisterServiceCtrlHandler(argv[0],
			(LPHANDLER_FUNCTION) shiroServiceCtrlHandler);

	if (serviceStatusHandle == (SERVICE_STATUS_HANDLE) 0)
	{
		wprintf(L"Error installing handler %s\n", errorMessage(GetLastError()));
		return;
	}

	updateServiceStatus(SERVICE_START_PENDING);

	init();

	hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	updateServiceStatus(SERVICE_RUNNING);

	while (WAIT_OBJECT_0 != WaitForSingleObject(hStopEvent, 300000))
	{
		updateServiceStatus(SERVICE_RUNNING);
		loop();
		updateHostAddress();
		printf("Waiting ....\n");
	}

	updateServiceStatus(SERVICE_STOPPED);

	return;
}

SERVICE_TABLE_ENTRY DispatchTable[2] = { { (char*) "ShiroKabuto", shiroServiceStart }, {
		NULL, NULL } };

bool registerService ()
{
	return StartServiceCtrlDispatcher(DispatchTable);
}

extern "C" int main ()
{
	time_t t;
	time(&t);
	t += GetCurrentProcessId();
	srand(t);

	if (registerService())
	{
		return 0;
	}

	printf("Executing manually\n");
	init();
	debug = true;
	loop();
}
