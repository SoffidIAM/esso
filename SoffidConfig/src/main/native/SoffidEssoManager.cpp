/*
 * SoffidEssoManager.cpp
 *
 *  Created on: Jan 25, 2013
 *      Author: areina
 */

# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <MZNcompat.h>

# include "SoffidEssoManager.h"
# include "SeyconServer.h"
# include "Utils.h"

// Connection agent
const std::wstring SoffidEssoManager::DEF_CON_AGENT = L"Soffid ESSO";

// Session manager path
const std::string SoffidEssoManager::DEF_SESSION_MNG =
		"SYSTEM\\CurrentControlSet\\Control\\Session Manager";

// Winlogon registry path
const std::string SoffidEssoManager::DEF_WINLOGON =
		"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

// Soffid registry path
const std::string SoffidEssoManager::DEF_CAIB_REGISTRY = "\\Software\\Soffid\\esso";

// Windows current version path
const std::string SoffidEssoManager::DEF_WINCURRENT =
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion";

// Certificate name
const std::string SoffidEssoManager::DEF_CERTIFICATE_NAME = "\\root.cer";

// Soffid GINA dll
const std::string SoffidEssoManager::DEF_GINA_LOGON = "SayakaGina.dll";

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
static bool debug = true;

// Default constructor
SoffidEssoManager::SoffidEssoManager ()
{
	// TODO Auto-generated destructor stub
}

// Destructor
SoffidEssoManager::~SoffidEssoManager ()
{
	// TODO Auto-generated destructor stub
}

// Get value for GINA dll path
const std::string& SoffidEssoManager::getGinaDll () const
{
	return ginaDll;
}

// Set value for GINA dll path
void SoffidEssoManager::setGinaDll (const std::string& ginaDll)
{
	this->ginaDll = ginaDll;
}

// Get login type
const std::string& SoffidEssoManager::getLoginType () const
{
	return loginType;
}

// Set login type
void SoffidEssoManager::setLoginType (const std::string& loginType)
{
	if (loginType.size() == 0)
	{
		this->loginType = "both";
	}

	else
	{
		this->loginType = loginType;
	}
}

// Get value for previous GINA value
const std::string& SoffidEssoManager::getPreviousGinaDll () const
{
	return previousGinaDll;
}

// Set value for previous GINA value
void SoffidEssoManager::setPreviousGinaDll (const std::string& previousGinaDll)
{
	this->previousGinaDll = previousGinaDll;
}

// Get enable close session by user value
const std::string& SoffidEssoManager::getEnableCloseSession () const
{
	return enableCloseSession;
}


// Get enable close session by user value
const std::string& SoffidEssoManager::getEnableSharedSession () const
{
	return enableSharedSession;
}

// Set enable close session by user value
void SoffidEssoManager::setEnableCloseSession (const std::string& enableCloseSession)
{
	if (enableCloseSession.size() == 0)
	{
		this->enableCloseSession = "false";
	}

	else
	{
		this->enableCloseSession = enableCloseSession;
	}
}

// Set enable close session by user value
void SoffidEssoManager::setEnableSharedSession (const std::string& enableSharedSession)
{
	if (enableSharedSession.size() == 0)
	{
		this->enableSharedSession = "false";
	}

	else
	{
		this->enableSharedSession = enableSharedSession;
	}
}

// Get force login on startup value
const std::string& SoffidEssoManager::getForceLogin () const
{
	return forceLogin;
}

// Set force login on startup value
void SoffidEssoManager::setForceLogin (const std::string& forceLogin)
{
	if (forceLogin.size() == 0)
	{
		this->forceLogin = "false";
	}

	else
	{
		this->forceLogin = forceLogin;
	}
}

// Get original GINA value path
const bool SoffidEssoManager::getOriginalGina () const
{
	return originalGina;
}

// Get ESSO Server URL
const std::string& SoffidEssoManager::getServerUrl () const
{
	return serverURL;
}

// Set ESSO Server URL
void SoffidEssoManager::setServerUrl (const std::string& serverUrl)
{
	serverURL = serverUrl;
}

// Set server port
const int SoffidEssoManager::getServerPort () const
{
	return serverPort;
}

// Get server port
void SoffidEssoManager::setServerPort (int serverPort)
{
	this->serverPort = serverPort;
}

// Check x64 system
BOOL SoffidEssoManager::IsWow64 ()
{
	BOOL bIsWow64 = FALSE;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.
	if (fnIsWow64Process == NULL)
	{
		fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
				GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	}

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			// TO-DO: handle error
		}
	}

	return bIsWow64;
}

// Get the x64 registry flag
DWORD SoffidEssoManager::Wow64Key (DWORD flag)
{
	return flag | (IsWow64() ? KEY_WOW64_64KEY : 0);
}

// Check OS Windows XP
BOOL SoffidEssoManager::IsWindowsXP ()
{
	DWORD dwVersion = 0;
	WORD dwMajorVersion = 0;

	dwVersion = GetVersion();

	// Get the Windows version.
	dwMajorVersion = (LOBYTE(LOWORD(dwVersion)) );

	return (dwMajorVersion <= 5);
}

// Get Mazinger installation dir
LPCSTR SoffidEssoManager::getMazingerDir ()
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			(LPCSTR) SoffidEssoManager::DEF_WINCURRENT.c_str(), 0, Wow64Key(KEY_READ),
			&hKey) == ERROR_SUCCESS)
	{
		static char achPath[4096] = "XXXX";
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		size = sizeof achPath;

		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType, (LPBYTE) achPath, &size);
		RegCloseKey(hKey);
		strcat(achPath, "\\SoffidESSO");

		return achPath;
	}

	else
	{
		return FALSE;
	}
}

// Get Soffid GINA path
std::string SoffidEssoManager::MazingerGinaPath ()
{
	std::string delim = "\\";

	return (getMazingerDir() + delim + SoffidEssoManager::DEF_GINA_LOGON);
}

// Write log
void SoffidEssoManager::log (const char *szFormat, ...)
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
				tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
		va_list v;
		va_start(v, szFormat);
		vfprintf(logFile, szFormat, v);
		va_end(v);
		if (szFormat[strlen(szFormat) - 1] != '\n')
			fprintf(logFile, "\n");
	}
}

// Read URL server
LPSTR SoffidEssoManager::readURL (HINTERNET hSession, const wchar_t* host, int port,
		LPCWSTR path, BOOL allowUnknownCA, size_t *pSize)
{
	BOOL bResults = FALSE;
	HINTERNET hConnect = NULL, hRequest = NULL;

	DWORD dwDownloaded = -1;
	BYTE *buffer = NULL;

	if (debug)
	{
		log("Connecting to %ls:%d...\n", host, port);
	}

	hConnect = WinHttpConnect(hSession, host, port, 0);

	if (hConnect)
	{
		if (debug)
		{
			log("Performing request %ls...\n", path);
		}

		hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER,
				WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	}

	// Send a request.
	if (hRequest)
	{
		if (debug)
			log("Sending request ...\n");

		WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, NULL, 0);

		if (allowUnknownCA)
		{
			DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
			WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, (LPVOID) &flags,
					sizeof flags);
		}

		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
				WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	}

	if (bResults && allowUnknownCA)
	{
		// Agreagar la CA ROOT
		PCERT_CONTEXT context;
		DWORD dwSize = sizeof context;
		BOOL result = WinHttpQueryOption(hRequest, WINHTTP_OPTION_SERVER_CERT_CONTEXT,
				&context, &dwSize);

		if (!result)
		{
			log("Cannot get context\n");
//			notifyError();
		}

		PCCERT_CONTEXT issuerContext = CertFindCertificateInStore(context->hCertStore,
				X509_ASN_ENCODING, 0, CERT_FIND_ISSUER_OF, context, NULL);
		HCERTSTORE systemStore = CertOpenStore((LPCSTR) 13, // CERT_STORE_PROV_SYSTEM_REGISTRY_W
				0, (HCRYPTPROV) NULL, (2 << 16) | // CERT_SYSTEM_STORE_LOCAL_MACHINE
						0x1000, // CERT_STORE_MAXIMUM_ALLOWED
				L"ROOT");
		CertAddCertificateContextToStore(systemStore, issuerContext,
				1 /*CERT_STORE_ADD_NEW*/, NULL);

		CertFreeCertificateContext(issuerContext);
		CertFreeCertificateContext(context);
	}

	// End the request.
	if (bResults)
	{
		if (debug)
			log("Waiting for response....\n");

		bResults = WinHttpReceiveResponse(hRequest, NULL);
	}

	// Keep checking for data until there is nothing left.
	DWORD used = 0;
	if (bResults)
	{
		const DWORD chunk = 4096;
		DWORD allocated = 0;
		do
		{
			if (used + chunk > allocated)
			{
				allocated += chunk;
				buffer = (LPBYTE) realloc(buffer, allocated);
			}

			dwDownloaded = 0;
			if (!WinHttpReadData(hRequest, &buffer[used], chunk, &dwDownloaded))
				dwDownloaded = -1;

			else
				used += dwDownloaded;
		} while (dwDownloaded > 0);

		buffer[used] = '\0';
	}

	DWORD dw = GetLastError();
	if (!bResults && debug)
	{
		if (dw == ERROR_WINHTTP_CANNOT_CONNECT)
			log("Error: Cannot connect\n");
		else if (dw == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
			log("Error: Client CERT required\n");
		else if (dw == ERROR_WINHTTP_CONNECTION_ERROR)
			log("Error: Connection error\n");
		else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_STATE)
			log("Error: ERROR_WINHTTP_INCORRECT_HANDLE_STATE\n");
		else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE)
			log("Error: ERROR_WINHTTP_INCORRECT_HANDLE_TYPE\n");
		else if (dw == ERROR_WINHTTP_INTERNAL_ERROR)
			log("Error: ERROR_WINHTTP_INTERNAL_ERROR\n");
		else if (dw == ERROR_WINHTTP_INVALID_URL)
			log("Error: ERROR_WINHTTP_INVALID_URL\n");
		else if (dw == ERROR_WINHTTP_LOGIN_FAILURE)
			log("Error: ERROR_WINHTTP_LOGIN_FAILURE\n");
		else if (dw == ERROR_WINHTTP_NAME_NOT_RESOLVED)
			log("Error: ERROR_WINHTTP_NAME_NOT_RESOLVED\n");
		else if (dw == ERROR_WINHTTP_OPERATION_CANCELLED)
			log("Error: ERROR_WINHTTP_OPERATION_CANCELLED\n");
		else if (dw == ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW)
			log("Error: ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW\n");
		else if (dw == ERROR_WINHTTP_SECURE_FAILURE)
			log("Error: ERROR_WINHTTP_SECURE_FAILURE\n");
		else if (dw == ERROR_WINHTTP_SHUTDOWN)
			log("Error: ERROR_WINHTTP_SHUTDOWN\n");
		else if (dw == ERROR_WINHTTP_TIMEOUT)
			log("Error: ERROR_WINHTTP_TIMEOUT\n");
		else if (dw == ERROR_WINHTTP_UNRECOGNIZED_SCHEME)
			log("Error: ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n");
		else if (dw == ERROR_NOT_ENOUGH_MEMORY)
			log("Error: ERROR_NOT_ENOUGH_MEMORY\n");
		else if (dw == ERROR_INVALID_PARAMETER)
			log("Error: ERROR_INVALID_PARAMETER\n");
		else if (dw == ERROR_WINHTTP_RESEND_REQUEST)
			log("Error:  ERROR_WINHTTP_RESEND_REQUEST\n");
		else if (dw != ERROR_SUCCESS)
		{
			log("Unkonwn error %d\n", dw);
		}
//		notifyError();
	}

	// Close any open handles.
	if (hRequest)
		WinHttpCloseHandle(hRequest);
	if (hConnect)
		WinHttpCloseHandle(hConnect);
	if (hSession)
		WinHttpCloseHandle(hSession);

	SetLastError(dw);

	if (pSize != NULL)
		*pSize = used;

	return (LPSTR) buffer;
}

static void
CALLBACK asyncCallback(HINTERNET hInternet, DWORD_PTR dwContext,
		DWORD dwInternetStatus, LPVOID lpvStatusInformation OPTIONAL,
		DWORD dwStatusInformationLength) {
	if (debug) {
		if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SECURE_FAILURE) {
			DWORD status = *(LPDWORD) lpvStatusInformation;
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
				SoffidEssoManager::log(
						"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
				SoffidEssoManager::log(
						"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
				SoffidEssoManager::log(
						"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
				SoffidEssoManager::log(
						"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
				SoffidEssoManager::log(
						"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR)
				SoffidEssoManager::log(
						"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR\n");
		}
	}
}

// Set URL
bool SoffidEssoManager::SaveURLServer (const char* url)
{
	URL_COMPONENTS urlComp;		// Initialize the URL_COMPONENTS structure.

	// Set required component lengths to non-zero so that they are cracked.
	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);
	WCHAR szScheme[15];
	urlComp.lpszScheme = szScheme;
	urlComp.dwSchemeLength = 14;
	WCHAR szHostName[256];
	urlComp.lpszHostName = szHostName;
	urlComp.dwHostNameLength = 255;
	WCHAR szPath[5096];
	urlComp.lpszUrlPath = szPath;
	urlComp.dwUrlPathLength = 4095;

	int strLen = mbstowcs(NULL, url, strlen(url) + 1);
	wchar_t* wUrl = (wchar_t*) malloc(strLen * sizeof(wchar_t));
	mbstowcs(wUrl, url, strlen(url) + 1);

	// Use WinHttpOpen to obtain a session handle.
	HINTERNET hSession = WinHttpOpen(SoffidEssoManager::DEF_CON_AGENT.c_str(),
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if (hSession)
	{
		if (!WinHttpCrackUrl(wUrl, wcslen(wUrl), 0, &urlComp))
		{
			MessageBox(NULL, Utils::LoadResourcesString(8).c_str(),
					Utils::LoadResourcesString(1001).c_str(),
					MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
		WinHttpSetStatusCallback(hSession, asyncCallback,
				WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);
	}
	else
		return false;

	// Obtener la lista de host
	size_t size;
	if (debug)
		log("Connecting to https://%ls:%d/cert\n", szHostName, urlComp.nPort);

	LPCSTR cert = readURL(hSession, szHostName, urlComp.nPort, L"/cert", true, &size);

	// Check error obtain certificate
	if (cert == NULL)
	{
		log("Error getting certificate");
		return FALSE;
	}

	std::string fileName = getMazingerDir();
	fileName += SoffidEssoManager::DEF_CERTIFICATE_NAME;

	FILE *f = fopen(fileName.c_str(), "wb");

	if (f == NULL)
	{
		log("Error generating file %s", fileName.c_str());
		return FALSE;
	}

	else
	{
		fwrite(cert, size, 1, f);
		fclose(f);
	}

	// Write certificate
	SeyconCommon::writeProperty("CertificateFile", fileName.c_str());

	// Write ESSO Server
	SeyconCommon::writeProperty("SSOServer", MZNC_wstrtostr(szHostName).c_str());

	char szPort[100];
	sprintf(szPort, "%d", urlComp.nPort);

	// Write server prot
	SeyconCommon::writeProperty("seycon.https.port", szPort);

	return true;
}

// Load Soffid ESSO configutration
bool SoffidEssoManager::LoadConfiguration ()
{
	bool loadConfigOK = true;	// Load config status
	std::string regValue;				// Registry value readed

	// Load enable close session
	SeyconCommon::readProperty("EnableCloseSession", regValue);
	setEnableCloseSession(regValue);
	SeyconCommon::readProperty("enableLocalAccounts", regValue);
	setEnableSharedSession(regValue);

	// Load login on startup
	SeyconCommon::readProperty("ForceStartupLogin", regValue);
	setForceLogin(regValue);

	// Load GINA dll
	// Check Windows XP
	if (IsWindowsXP())
	{
		LoadGinaConfiguration();
	}

	// Load login type
	SeyconCommon::readProperty("LoginType", regValue);
	setLoginType(regValue);

	// Load server URL
	LoadServerURL();

	return loadConfigOK;
}

void SoffidEssoManager::LoadGinaConfiguration ()
{
	HKEY hKey;										// Handle to open registry key
	DWORD dwType;								// Type of data stored
	char winlogon[4096];						// GINA path
	DWORD size = sizeof winlogon;	// Size GINA path value
	std::string regValue;						// Registry value readed

	// Check open key content
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, DEF_WINLOGON.c_str(), 0,
			Wow64Key(KEY_ALL_ACCESS), &hKey) == ERROR_SUCCESS)
	{
		RegQueryValueEx(hKey, "GinaDll", NULL, &dwType, (LPBYTE) &winlogon, &size);
	}

	// Check default windows logon changed
	if (strcmp(winlogon, "msgina.dll") == 0)
	{
		originalGina = true;
	}

	else
	{
		originalGina = false;
	}

	setGinaDll(winlogon);

	// Load previous gina dll
	SeyconCommon::readProperty("PreviousGinaDll", regValue);
	setPreviousGinaDll(regValue);
}

// Save configuration
bool SoffidEssoManager::SaveConfiguration ()
{
	bool saveConfigOK = true;		// Save config status

	// Save enable close session
	SeyconCommon::writeProperty("EnableCloseSession", getEnableCloseSession().c_str());

	// Save enable close session
	SeyconCommon::writeProperty("enableLocalAccounts", getEnableSharedSession().c_str());

	// Save login on startup
	SeyconCommon::writeProperty("ForceStartupLogin", getForceLogin().c_str());

	// Save GINA dll (only Windows XP OS)
	SaveGINAConfiguration();

	// Save login type
	SeyconCommon::writeProperty("LoginType", getLoginType().c_str());

	// Save server URL
	setServerUrl(getServerUrl());

	return saveConfigOK;
}

// Save GINA configuration
bool SoffidEssoManager::SaveGINAConfiguration ()
{
	bool saveGINAConfig = true;	// Checker for save process

	if (IsWindowsXP())
	{
		UpdateGina(getGinaDll().c_str());

		// Update previous GINA path
		SeyconCommon::writeProperty("PreviousGinaDll", getPreviousGinaDll().c_str());
	}

	return saveGINAConfig;
}

// Update GINA value path
void SoffidEssoManager::UpdateGina (const char *ginaPath)
{
	HKEY hKey;

	// Open registry key
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SoffidEssoManager::DEF_WINLOGON.c_str(), 0,
			KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		DWORD result = RegSetValueEx(hKey, "GinaDll", 0, REG_SZ, (LPBYTE) ginaPath,
				strlen(ginaPath));

		if (result != ERROR_SUCCESS)
		{
			printf("Error setting registry value GinaDll \n");
			log("Cannot configure ginadll");
		}

		RegCloseKey(hKey);
	}

	else
	{
		printf("Unable to deregister Mazinger\n");
		log("Cannot open HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
	}
}

// Load server URL
void SoffidEssoManager::LoadServerURL ()
{
	std::string firstServer;	// First server of list
	std::string servers;		// List of available servers
	size_t pos;					// End post for first server

	SeyconCommon::getServerList(servers);

	// Search delimiter of servers on list
	pos = servers.find(",");

	// Check search first server
	if (pos != std::string::npos)
	{
		setServerUrl(servers.substr(0, pos));
	}

	else
	{
		setServerUrl(servers);
	}

	// Get server port
	setServerPort(SeyconCommon::getServerPort());
}

// Get formated URL Server
std::string SoffidEssoManager::GetFormatedURLServer ()
{
	char serverPort[10];	// Server port

	itoa(getServerPort(), serverPort, 10);

	return ("https://" + getServerUrl() + ":" + serverPort);
}
