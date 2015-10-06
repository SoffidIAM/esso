
#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <winuser.h>
#include <wtsapi32.h>
#include <winhttp.h>
#include <wincrypt.h>
#define SECURITY_WIN32
#include <security.h>
#include <time.h>

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

#else

#include <errno.h>
#include "ConfigFile.h"

#endif


#include <wchar.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <SeyconServer.h>
#include <stdlib.h>
#include <MZNcompat.h>
int SeyconCommon::seyconDebugLevel = 0;

__attribute__((constructor))
static void initDebugLevel () {
	int i = SeyconCommon::readIntProperty ("debugLevel");
	if (i <= 0)
		SeyconCommon::setDebugLevel(0);
	else
		SeyconCommon::setDebugLevel(i);
}

#ifdef WIN32
static HINSTANCE hWTSAPI;
typedef struct _MY_WTS_CLIENT_ADDRESS {
    DWORD AddressFamily;  // AF_INET, AF_IPX, AF_NETBIOS, AF_UNSPEC
    BYTE  Address[20];    // client network address
} MYWTS_CLIENT_ADDRESS, * PMYWTS_CLIENT_ADDRESS;



typedef BOOL (WINAPI *typeWTSQuerySessionInformation)( IN HANDLE hServer,
		IN DWORD SessionId, IN WTS_INFO_CLASS WTSInfoClass,
		OUT char* * ppBuffer, OUT DWORD * pBytesReturned);
static typeWTSQuerySessionInformation pWTSQuerySessionInformation = NULL;
typedef void (WINAPI *typeWTSFreeMemory)(PVOID pMemory);
static typeWTSFreeMemory pWTSFreeMemory = NULL;

void SeyconCommon::notifyError() {
	char* pstr;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, GetLastError(), 0, (char*) & pstr, 0, NULL);
	SeyconCommon::warn("%s\n\n", pstr);
	LocalFree(pstr);
}

void freeCitrixMemory(PVOID pMemory) {

	/*
	 *  Get handle to WTSAPI.DLL
	 */
	if (hWTSAPI == NULL && (hWTSAPI = LoadLibrary("WTSAPI32")) == NULL) {
		return;
	}

	/*
	 *  Get entry point for WTSEnumerateServers
	 */
	if (pWTSFreeMemory == NULL)
		pWTSFreeMemory = (typeWTSFreeMemory) GetProcAddress(hWTSAPI,
				"WTSFreeMemory");
	if (pWTSFreeMemory == NULL) {
		return;
	}
	(*pWTSFreeMemory)(pMemory);
}

void SeyconCommon::getCitrixInitialProgram(std::string &name) {
	char* lpszBytes;
	DWORD dwBytes = 0;
	BOOL bOK = FALSE;
	name.clear ();

	/*
	 *  Get handle to WTSAPI.DLL
	 */
	if (hWTSAPI == NULL && (hWTSAPI = LoadLibrary("WTSAPI32")) == NULL) {
		return;
	}

	/*
	 *  Get entry point for WTSEnumerateServers
	 */
	if (pWTSQuerySessionInformation == NULL)
		pWTSQuerySessionInformation
				= (typeWTSQuerySessionInformation) GetProcAddress(hWTSAPI,
						"WTSQuerySessionInformationA");
	if (pWTSQuerySessionInformation == NULL) {
		name.clear ();
		return;
	}
	bOK = (*pWTSQuerySessionInformation)(WTS_CURRENT_SERVER_HANDLE,
			WTS_CURRENT_SESSION, WTSInitialProgram, &lpszBytes, &dwBytes);
	if (bOK) {
		name.assign (lpszBytes);
		freeCitrixMemory(lpszBytes);
	} else
		name.clear ();
}

void SeyconCommon::getCitrixClientName(std::string &name) {
	char* lpszBytes;
	DWORD dwBytes = 0;
	BOOL bOK = FALSE;
	name.clear ();

	/*
	 *  Get handle to WTSAPI.DLL
	 */
	if (hWTSAPI == NULL && (hWTSAPI = LoadLibrary("WTSAPI32")) == NULL) {
		return;
	}

	/*
	 *  Get entry point for WTSEnumerateServers
	 */
	if (pWTSQuerySessionInformation == NULL)
		pWTSQuerySessionInformation
				= (typeWTSQuerySessionInformation) GetProcAddress(hWTSAPI,
						"WTSQuerySessionInformationA");
	if (pWTSQuerySessionInformation == NULL) {
		name.clear ();
		return;
	}
	bOK = (*pWTSQuerySessionInformation)(WTS_CURRENT_SERVER_HANDLE,
			WTS_CURRENT_SESSION, WTSClientName, &lpszBytes, &dwBytes);
	if (bOK) {
		name.assign (lpszBytes);
		freeCitrixMemory(lpszBytes);
	} else
		name.clear ();
}

void SeyconCommon::getCitrixClientIP(std::string &ip) {
	DWORD dwBytes = 0;
	BOOL bOK = FALSE;
	PMYWTS_CLIENT_ADDRESS wfca;

	ip.clear ();
	/*
	 *  Get handle to WTSAPI.DLL
	 */
	if (hWTSAPI == NULL && (hWTSAPI = LoadLibrary("WTSAPI32")) == NULL) {
		return;
	}

	std::string name;
	getCitrixClientName(name);
	if (name.empty ()) {
		return;
	}

	/*
	 *  Get entry point for WTSEnumerateServers
	 */
	if (pWTSQuerySessionInformation == NULL)
		pWTSQuerySessionInformation
				= (typeWTSQuerySessionInformation) GetProcAddress(hWTSAPI,
						"WTSQuerySessionInformationA");
	if (pWTSQuerySessionInformation == NULL) {
		return;
	}
	bOK = (*pWTSQuerySessionInformation)(WTS_CURRENT_SERVER_HANDLE,
			WTS_CURRENT_SESSION, WTSClientAddress, (char* *) &wfca, &dwBytes);
	if (bOK) {
		if (wfca->AddressFamily == AF_INET) {
			char ach[50];
			sprintf(ach, "%d.%d.%d.%d", wfca -> Address[2],
					wfca -> Address[3], wfca -> Address[4], wfca -> Address[5]);
			ip = ach;
			freeCitrixMemory(wfca);
		} else {
			ip = "???";
		}
	}
}

#else

void notifyError() {

	fprintf(stderr, "***************\n");
	fprintf(stderr, "    ERROR      \n");
	fprintf(stderr, "\n");
	perror("");
	fprintf(stderr, "\n");
	fprintf(stderr, "***************\n");
}

void SeyconCommon::getCitrixInitialProgram(std::string &name) {
	name.clear();
}

void SeyconCommon::getCitrixClientName(std::string &name) {
	name.clear ();
}
void SeyconCommon::getCitrixClientIP(std::string &ip) {
	ip.clear();
}
#endif

bool SeyconCommon::bNormalized() {
#ifdef WIN32
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\Current Version\\Winlogon", 0,
			KEY_READ|KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS ||
		RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\Current Version\\Winlogon", 0,
			KEY_READ, &hKey) == ERROR_SUCCESS) {
		char achUserInit[4096] = "";
		DWORD dw = FALSE;
		DWORD dwType;
		DWORD size = sizeof achUserInit;
		size = sizeof dw;
		RegQueryValueEx(hKey, "userinit", NULL, &dwType,
				(LPBYTE) & achUserInit, &size);
		RegCloseKey(hKey);
		return strstr(achUserInit, "kojikabuto") != NULL;
	} else {
		return FALSE;
	}
#else
	return true;
#endif
}

#ifndef WIN32
static ConfigFile *configFile = NULL;
#endif

bool SeyconCommon::readProperty(const char* property, std::string &value) {
#ifdef WIN32
	value.clear ();
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Soffid\\esso", 0, KEY_READ|KEY_WOW64_64KEY,
			&hKey) == ERROR_SUCCESS ||
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Soffid\\esso", 0, KEY_READ,
						&hKey) == ERROR_SUCCESS) {
		DWORD dw;
		DWORD dwType;
		DWORD dwSize2 = 0;
		char *data = NULL;
		dw = RegQueryValueEx(hKey, property, NULL, &dwType, (LPBYTE) NULL,
				&dwSize2);

		if (dw == ERROR_MORE_DATA || dw == ERROR_SUCCESS) {
			data = (char*)malloc(dwSize2 + 1);
			dw = RegQueryValueEx(hKey, property, NULL, &dwType, (LPBYTE) data,
					&dwSize2);
			data[dwSize2] = '\0';
		}
		RegCloseKey(hKey);
		if (dw == ERROR_SUCCESS && data != NULL)
		{
			value.assign(data);
			free (data);
			return true;
		}
	}
	return false;
#else
	if (configFile == NULL)
	{
		SeyconCommon::debug ("Loading config file");
		configFile = new ConfigFile ();
		configFile->load ("/etc/mazinger/config");
	}
	const char *data = configFile->getValue(property);
	if (data == NULL) {
		value.clear ();
		return false;
	}
	else
	{
		value.assign(data);
		return true;
	}
#endif
}

void SeyconCommon::writeProperty(const char *property, const char *value) {
#ifdef WIN32
			HKEY hKey;
			if (strlen(value) > 0 && (
					RegCreateKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Soffid\\esso", 0, (char*) "",
							REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, NULL, &hKey, NULL)
							== ERROR_SUCCESS ||
					RegCreateKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Soffid\\esso", 0, (char*) "",
							REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hKey, NULL)
							== ERROR_SUCCESS )
				) {
				DWORD size = strlen(value) + 1;
				DWORD dwType = REG_SZ;
				RegSetValueEx(hKey, property, 0, dwType, (LPBYTE) value, size);
				RegCloseKey(hKey);
			}
#else
			if (configFile != NULL)
			{
				configFile->setValue (property, value);
				configFile->save ("/etc/mazinger/config");
			}
#endif
}

int SeyconCommon::readIntProperty (const char*property) {
#ifdef WIN32
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Soffid\\esso", 0, KEY_READ|KEY_WOW64_64KEY,
			&hKey) == ERROR_SUCCESS ||
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Soffid\\esso", 0, KEY_READ,
			&hKey) == ERROR_SUCCESS) {
		DWORD dw = FALSE;
		DWORD dwType;
		DWORD data;
		DWORD dwSize2 = sizeof data;
		dw = RegQueryValueEx(hKey, property, NULL, &dwType, (LPBYTE) &data,
				&dwSize2);
		RegCloseKey(hKey);
		if (dw == ERROR_SUCCESS)
		{
			return data;
		}
	}
	return -1;
#else
	std::string value;
	if (!readProperty(property, value))
		return -1;
	int i;
	sscanf (value.c_str(), " %d", &i);
	return i;
#endif
}

int SeyconCommon::getCardSupport() {
	size_t dw;
	std::string client;

	dw = 3; // If Needed

	getCitrixClientName(client);
	if (client.empty())
		dw = readIntProperty("LocalCardSupport");
	else
		dw = readIntProperty("RemoteCardSupport");
	if (dw == (size_t)-1)
		dw = 3;
	return dw;
}

bool SeyconCommon::getServerList(std::string &servers) {
	return readProperty("SSOServer", servers);
}

int SeyconCommon::getServerPort() {
	std::string port;
	int dw = 750;
	if (readProperty("seycon.https.port", port))
		sscanf(port.c_str(), " %d", &dw);
	return dw;

}

int SeyconCommon::getServerAlternatePort() {
	std::string port;
	int dw = 1750;
	if (readProperty("seycon.https.alternate.port", port))
		sscanf(port.c_str(), " %d", &dw);
	return dw;

}

void SeyconCommon::setDebugLevel(int newdebugLevel) {
	seyconDebugLevel = newdebugLevel;
}

//////////////////////////////////////////////////////////
void SeyconCommon::updateConfig(const char* lpszParam) {
	debug ("Update %s param\n", lpszParam);

	SeyconService service;

	std::wstring wParam = service.escapeString (lpszParam);
	SeyconResponse *resp = service.sendUrlMessage(L"/query/config/%ls?format=text/plain&nofail=true", wParam.c_str());

	if (resp != NULL) {
		std::string status;
		resp->getToken(0, status);
		if (status == "OK") {
			std::string value;
			resp->getToken(5, value);
			writeProperty (lpszParam, value.c_str());
		}
		delete resp;
	}
}
#ifdef WIN32

static HANDLE hEventLog = NULL;
static HANDLE getEventLog () {
	if (hEventLog == NULL)
		hEventLog = RegisterEventSource (NULL, "Mazinger");
	return hEventLog;
}
//
#define MC_MAZINGER (DWORD) 0x1
//
#define MC_MAZINGER_INFO (DWORD) 0xfff0064
//
#define MC_MAZINGER_WARN (DWORD) 0x8fff0065
//
#define MC_MAZINGER_DEBUG (DWORD) 0x4fff0066


#endif

void SeyconCommon::info (const char *szFormat, ...) {
	if (seyconDebugLevel > 0)
	{
#ifdef WIN32
		char achMessage[4096];
		va_list v2;
		va_start(v2, szFormat);
		vsnprintf(achMessage, 4095, szFormat, v2);
		va_end(v2);
		const char *params = achMessage;
	    ReportEvent(getEventLog(),
	    		EVENTLOG_AUDIT_SUCCESS, MC_MAZINGER, MC_MAZINGER_INFO,
	    		NULL, 1, 0, &params, NULL);
#endif
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (stderr, "%d-%02d-%04d %02d:%02d:%02d INFO ",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
		va_list v;
		va_start(v, szFormat);
		vprintf(szFormat, v);
		va_end(v);
		if (szFormat[strlen(szFormat)-1] != '\n')
			fprintf (stderr, "\n");
		fflush (stderr);
	}
}

void SeyconCommon::warn (const char *szFormat, ...) {
#ifdef WIN32
		char achMessage[4096];
		va_list v2;
		va_start(v2, szFormat);
		vsnprintf(achMessage, 4095, szFormat, v2);
		va_end(v2);
		const char *params = achMessage;
	    ReportEvent(getEventLog(),
	    		EVENTLOG_WARNING_TYPE, MC_MAZINGER, MC_MAZINGER_WARN,
	    		NULL, 1, 0, &params, NULL);
#endif
	va_list v;
	va_start(v, szFormat);
	time_t t;
	time(&t);
	struct tm *tm = localtime (&t);
	fprintf (stderr, "%d-%02d-%04d %02d:%02d:%02d WARN ",
			tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
	vfprintf(stderr, szFormat, v);
	va_end(v);
	if (szFormat[strlen(szFormat)-1] != '\n')
		fprintf (stderr, "\n");
	fflush (stderr);
}

void SeyconCommon::debug (const char *szFormat, ...) {
	if (seyconDebugLevel > 1)
	{
#ifdef WIN32
		char achMessage[4096];
		va_list v2;
		va_start(v2, szFormat);
		vsnprintf(achMessage, 4095, szFormat, v2);
		va_end(v2);
		const char *params = achMessage;
	    ReportEvent(getEventLog(),
	    		EVENTLOG_INFORMATION_TYPE, MC_MAZINGER, MC_MAZINGER_DEBUG,
	    		NULL, 1, 0, &params, NULL);
#endif
		time_t t;
		time(&t);
		struct tm *tm = localtime (&t);
		fprintf (stderr, "%d-%02d-%04d %02d:%02d:%02d DEBUG ",
				tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
		va_list v;
		va_start(v, szFormat);
		vprintf(szFormat, v);
		va_end(v);
		if (szFormat[strlen(szFormat)-1] != '\n')
			fprintf (stderr,"\n");
		fflush (stderr);
	}

}

void SeyconCommon::wipe (std::string &str) {
	memset ((char*) str.c_str(), 0, str.size());
	str.clear ();
}

void SeyconCommon::wipe (std::wstring &str) {
	memset ((char*) str.c_str(), 0, str.size() * sizeof (wchar_t));
	str.clear ();
}

void SeyconCommon::updateHostAddress () {
	std::string serialNumber;
	SeyconCommon::readProperty("serialNumber", serialNumber);
	if (serialNumber.size() == 0) {
		const char *hostName = MZNC_getHostName();
		time_t t;
		time (&t);
		srand( t  );

		serialNumber = hostName;
		serialNumber += "-";
		char ach[20];
		time (&t);
		sprintf (ach, "%ld", (long) t);
		serialNumber += ach;
		serialNumber += "-";

		for (int i = 0; i <50; i++) {
				unsigned long n =rand();
				n = n % 62;
				char ach[2];
				ach[1] = '\0';
				if ( n < 26) {
					ach [0] = ((char) ('A' + n));
				} else if ( n < 52) {
					ach [0] = ((char) ('a'+ n - 26));
				} else {
					ach [0] = ((char) ('0' + n - 52));
				}
				serialNumber += ach;
		}
		SeyconCommon::writeProperty("serialNumber", serialNumber.c_str());
	}

	SeyconService service ;
	SeyconResponse*response= service.sendUrlMessage(L"/updateHostAddress?name=%hs&serial=%hs",
			MZNC_getHostName(), serialNumber.c_str());
	if (response != NULL && response->getResult() != NULL)  {
		if (response->getToken(0) == "ERROR") {
			SeyconCommon::warn ("Error updating host address: %hs", response->getToken(1).c_str());
		}
	}
}



int hextoint (char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else
		return -1;
}

char inttohex (int i)
{
	if (i < 10)
		return '0' + i;
	else
		return 'a' + i - 10;
}

std::wstring SeyconCommon::urlDecode (const char* str) {
	std::string result;
	const unsigned char *sz = (const unsigned char*) str;
	int i = 0;
	while (sz[i])
	{
		if (sz[i] == '%')
		{
			int hex1 = hextoint (sz[++i]);
			if (hex1 >= 0)
			{
				int hex2 = hextoint (sz[++i]);
				if (hex2 >= 0)
				{
					i++;
					result += (char) (hex1 << 4 | hex2);
				}
			}
		}
		else if (sz[i] == '+')
		{
			result += ' ';
			i++;
		}
		else
		{
			result += sz[i++];
		}
	}
	return MZNC_utf8towstr(result.c_str());
}

std::string SeyconCommon::urlEncode  (const wchar_t* str)
{
	std::string utf8 = MZNC_wstrtoutf8(str);
	const unsigned char* sz = (const unsigned char*) utf8.c_str();
	std::string result;
	int i = 0;
	while (sz[i])
	{
		if (sz[i] == ' ')
			result += "+";
		else if (sz[i] < '0' || sz[i] > '9' && sz[i] < 'A' || (sz[i] > 'Z' && sz[i] < 'a') || sz[i] > 'z' )
		{
			int s = (int) sz[i];
			if (s < 0) s += 256;
			result += "%";
			result += inttohex ( s / 16);
			result += inttohex ( s % 16);
		}
		else
			result += sz[i];
		i++;
	}
	return result;
}
