
#ifdef WIN32

#include <windows.h>
#include <stdio.h>
#include <winsock.h>
#include <ctype.h>
#include <winuser.h>
#include <wtsapi32.h>
#include <winhttp.h>
#include <wincrypt.h>
#define SECURITY_WIN32
#include <security.h>

#else

#include <wchar.h>

#include <libsoup/soup.h>
#include <glib.h>
#include <string>
#include <stdlib.h>
#include <iconv.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
// #include <stdargs.h>
#include <dlfcn.h>

#endif


#include <MZNcompat.h>
#include <SeyconServer.h>
#include <time.h>
#include <string>

class SeyconURLServiceIterator: public SeyconServiceIterator {
public:
	virtual ServiceIteratorResult iterate (const char* hostName, size_t dwPort);
	virtual ~SeyconURLServiceIterator ();
	const wchar_t *path;
	SeyconResponse *result;
private:
	static time_t lastError ;
};

SeyconServiceIterator::SeyconServiceIterator () {

}

SeyconServiceIterator::~SeyconServiceIterator () {

}

#ifdef WIN32


static PCCERT_CONTEXT pSeyconRootCert = NULL;
static HINTERNET hSession = NULL;

////////////////////////////////////////////////////////////////////////
static void
CALLBACK asyncCallback(HINTERNET hInternet, DWORD_PTR dwContext,
		DWORD dwInternetStatus, LPVOID lpvStatusInformation,
		DWORD dwStatusInformationLength) {
	if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SECURE_FAILURE) {
		DWORD status = *(LPDWORD) lpvStatusInformation;
		if (status == WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED)
			SeyconCommon::warn(
					"ERROR: Unable to validate certificate revocation status\n");
		else if (status == WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
			SeyconCommon::warn("ERROR: Invalid certificate\n");
		else if (status == WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
			SeyconCommon::warn("ERROR: Revoked certificate\n");
		else if (status == WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
			SeyconCommon::warn("ERROR: Invalid certificate authority\n");
		else if (status == WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
			SeyconCommon::warn("ERROR: Certificate mismatch.\n");
		else if (status == WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
			SeyconCommon::warn("ERROR: Expired certificate \n");
		else if (status == WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR)
			SeyconCommon::warn("ERROR: Unspecified SSL Error\n");
		else
			SeyconCommon::warn("ERROR: Unexpected error %d\n", status);
	}
}


ServiceIteratorResult SeyconURLServiceIterator::iterate (const char* host, size_t port) {
	BOOL bResults = FALSE;
	HINTERNET hConnect = NULL, hRequest = NULL;
	ServiceIteratorResult iteratorResult = SIR_RETRY;

	bool repeat = false;
	bool firstTime = true;

	do {

		repeat = false;

		if (hSession == NULL)
			hSession = WinHttpOpen(L"Mazinger Agent/1.0",
					WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
					WINHTTP_NO_PROXY_BYPASS, 0);


		WinHttpSetStatusCallback(hSession, asyncCallback,
				WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);

		WinHttpSetTimeouts(
				hSession,
		  5000, // 5 segons __in  int dwResolveTimeout,
		  15000, // 15 segons __in  int dwConnectTimeout,
		  60000, // 60 segons, send timeout
		  120000 // 2 minuts   receive timeout
		);

		wchar_t *path2 = wcsdup (path);
		wchar_t *query = wcsstr(path2, L"?");
		if (query != NULL)
			*query = L'\0';
		SeyconCommon::debug("Performing request https://%hs:%d%ls...\n", host, port, path2);
		free (path2);

		/*	WinHttpSetOption ( hSession, WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
		 WINHTTP_NO_CLIENT_CERT_CONTEXT,
		 0);*/
		std::wstring wsHost = MZNC_strtowstr (host);
		hConnect = WinHttpConnect(hSession, wsHost.c_str(), port, 0);

		if (hConnect) {



			hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
					WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
					WINHTTP_FLAG_SECURE);

			WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT,
					(LPVOID) 120000, 0); // 2 minuts de timeout

			WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
					WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

	#if 0
			WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT,
						(LPVOID) INFINITE, 0);
	#endif
		}

		// Send a request.
		if (hRequest) {
			if (pSeyconRootCert != NULL) {
				DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
				WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, (LPVOID)
						& flags, sizeof flags);
			}
			bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
					0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
			// Verificar CA root
			if (bResults && pSeyconRootCert != NULL) {
				// Verificar la CA ROOT
				PCCERT_CONTEXT context;
				DWORD dwSize = sizeof context;
				BOOL result = WinHttpQueryOption(hRequest,
						WINHTTP_OPTION_SERVER_CERT_CONTEXT, &context, &dwSize);
				if (!result || context == NULL) {
					bResults = NULL;
				} else {
					PCCERT_CONTEXT issuerContext = CertFindCertificateInStore(
							context->hCertStore, X509_ASN_ENCODING, 0,
							CERT_FIND_ISSUER_OF, context, NULL);
					if (!CertComparePublicKeyInfo(X509_ASN_ENCODING
							| PKCS_7_ASN_ENCODING,
							&issuerContext->pCertInfo->SubjectPublicKeyInfo,
							&pSeyconRootCert->pCertInfo->SubjectPublicKeyInfo)) {
						SeyconCommon::info("Received certificate not allowed\n");
						char pszNameString[256] = "<unknown>";
						char pszNameString2[256] = "<unknown>";

						CertGetNameStringA(pSeyconRootCert,
								CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
								pszNameString2, 128);

						CertGetNameStringA(issuerContext,
								CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
								pszNameString, 128);

						SeyconCommon::warn ("ERROR: Invalid CA %s. Should be %s\n",
								pszNameString, pszNameString2);
						bResults = NULL;
						SetLastError(ERROR_WINHTTP_SECURE_FAILURE);
					}
				}
			}
		}

		// End the request.
		if (bResults) {
			bResults = WinHttpReceiveResponse(hRequest, NULL );
		}

		// Keep checking for data until there is nothing left.
		if (bResults) {
			const DWORD chunk = 4096;
			DWORD used = 0;
			DWORD allocated = 0;
			BYTE *buffer = NULL;
			DWORD dwDownloaded;
			do {
				if (used + chunk + 1 > allocated) {
					allocated += chunk + 1;
					buffer = (LPBYTE) realloc(buffer, allocated);
				}
				dwDownloaded = 0;
				if (!WinHttpReadData(hRequest, &buffer[used], chunk, &dwDownloaded))
					dwDownloaded = -1;
				else
					used += dwDownloaded;
			} while (dwDownloaded > 0);
			if (used > 0) {
				buffer[used] = '\0';
				result = new SeyconResponse ((char*) buffer, used+1);
				delete buffer;
				iteratorResult = SIR_SUCCESS;
			}
		}

		DWORD dw = GetLastError();
		if (!bResults) {
			if (dw == ERROR_WINHTTP_CANNOT_CONNECT)
				SeyconCommon::warn("Error: Cannot connect\n");
			else if (dw == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
				SeyconCommon::warn("Error: Client CERT required\n");
			else if (dw == ERROR_WINHTTP_CONNECTION_ERROR)
				SeyconCommon::warn("Error: Connection error\n");
			else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_STATE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_INCORRECT_HANDLE_STATE\n");
			else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_INCORRECT_HANDLE_TYPE\n");
			else if (dw == ERROR_WINHTTP_INTERNAL_ERROR)
				SeyconCommon::warn("Error: ERRR_WINHTTP_INTERNAL_ERROR\n");
			else if (dw == ERROR_WINHTTP_INVALID_URL)
				SeyconCommon::warn("Error: ERROR_WINHTTP_INVALID_URL\n");
			else if (dw == ERROR_WINHTTP_LOGIN_FAILURE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_LOGIN_FAILURE\n");
			else if (dw == ERROR_WINHTTP_NAME_NOT_RESOLVED)
				SeyconCommon::warn("Error: ERROR_WINHTTP_NAME_NOT_RESOLVED\n");
			else if (dw == ERROR_WINHTTP_OPERATION_CANCELLED)
				SeyconCommon::warn("Error: ERROR_WINHTTP_OPERATION_CANCELLED\n");
			else if (dw == ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW)
				SeyconCommon::warn("Error: ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW\n");
			else if (dw == ERROR_WINHTTP_SECURE_FAILURE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_SECURE_FAILURE\n");
			else if (dw == ERROR_WINHTTP_SHUTDOWN)
				SeyconCommon::warn("Error: ERROR_WINHTTP_SHUTDOWN\n");
			else if (dw == ERROR_WINHTTP_TIMEOUT)
				SeyconCommon::warn("Error: ERROR_WINHTTP_TIMEOUT\n");
			else if (dw == ERROR_WINHTTP_UNRECOGNIZED_SCHEME)
				SeyconCommon::warn("Error: ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n");
			else if (dw == ERROR_NOT_ENOUGH_MEMORY)
				SeyconCommon::warn("Error: ERROR_NOT_ENOUGH_MEMORY\n");
			else if (dw == ERROR_INVALID_PARAMETER)
				SeyconCommon::warn("Error: ERROR_INVALID_PARAMETER\n");
			else if (dw == ERROR_WINHTTP_RESEND_REQUEST)
				SeyconCommon::warn("Error:  ERROR_WINHTTP_RESEND_REQUEST\n");
			else if (dw ==  ERROR_WINHTTP_INVALID_SERVER_RESPONSE ) {
				if (firstTime) {
					firstTime = false;
					repeat = true;
					SeyconCommon::warn("Error:  ERROR_WINHTTP_INVALID_SERVER_RESPONSE. Retrying...\n");
				} else
					SeyconCommon::warn("Error:  ERROR_WINHTTP_INVALID_SERVER_RESPONSE\n");
			} else
				SeyconCommon::warn("Error:  %d\n", dw);
		}

		// Close any open handles.
		if (hRequest)
			WinHttpCloseHandle(hRequest);
		if (hConnect)
			WinHttpCloseHandle(hConnect);

		SetLastError(dw);
	} while (repeat);
	return iteratorResult;
}


#else

static SoupSession *pSession = NULL;

struct TempBuffer {
	char *buffer;
	int size;
};



ServiceIteratorResult SeyconURLServiceIterator::iterate (const char* hostName, size_t dwPort) {
	if (pSession == NULL)
	{
		static void   (*pg_thread_init)   (GThreadFunctions *vtable);

		pg_thread_init = NULL;
		if (pg_thread_init == NULL) {
			pg_thread_init = (void   (*)   (GThreadFunctions *vtable)) dlsym (RTLD_DEFAULT, "g_thread_init");
		}
		if (pg_thread_init != NULL)
			pg_thread_init (NULL);
		g_type_init ();

		std::string fileName;

		SeyconCommon::readProperty("CertificateFile", fileName);
		if (fileName.size() == 0) {
			SeyconCommon::warn("Unknown certificate file. Please configure /etc/mazinger\n");
			return SIR_ERROR;
		}

		pSession = soup_session_sync_new_with_options ( SOUP_SESSION_SSL_CA_FILE, fileName.c_str(),
				SOUP_SESSION_IDLE_TIMEOUT, 3,
				SOUP_SESSION_TIMEOUT, 60,
				NULL);
	}

	char num[10];
	sprintf (num, "%d", dwPort);

	std::string szUri = MZNC_wstrtostr(path);
	std::string szUrl = "https://";
	szUrl += hostName;
	szUrl += ":";
	szUrl += num;
	szUrl += szUri;

	char *path2 = strdup (szUrl.c_str());
	char *query = strstr(path2, "?");
	if (query != NULL)
		*query = L'\0';

	SeyconCommon::debug("Performing request %s....", path2);

	free (path2);

	SoupMessage *msg = soup_message_new ("GET", szUrl.c_str());

	SeyconCommon::wipe(szUri);
	SeyconCommon::wipe(szUrl);


	if (msg != NULL) {
		guint status = soup_session_send_message(pSession, msg);
		if (status == 204) // HTTP-NO-DATA
		{
			SeyconCommon::debug ("Success HTTP/204 (No data)", status);
			g_object_unref(msg);
			msg = NULL;
			return SIR_ERROR;
		} else if (status != 200) // HTTP-OK
		{
			SeyconCommon::debug ("Error HTTP/%d at host: %s:%d: %s", status, hostName, dwPort,
					msg->reason_phrase);
			g_object_unref(msg);
			msg = NULL;
			return SIR_RETRY;
		} else {
			SoupBuffer *buffer = soup_message_body_flatten(msg->response_body);
			result = new SeyconResponse ((char*) buffer->data, buffer->length);
			g_object_unref(msg);
			return SIR_SUCCESS;
		}
	} else {
		SeyconCommon::warn("Unable to create soup message\n");
		return SIR_ERROR;
	}

}

#endif

SeyconURLServiceIterator::~SeyconURLServiceIterator () {

}

SeyconResponse*  SeyconService::sendUrlMessage(const char* host, int port, const wchar_t* url, ...) {

	va_list v;
	va_start (v, url);
	wchar_t wch[4000];
#ifdef WIN32
	vsnwprintf(wch, 3999, url, v);
#else
	vswprintf(wch, 3999, url, v);
#endif
	va_end (v);

	SeyconURLServiceIterator it;
	it.path = wch;
	ServiceIteratorResult r = it.iterate (host, port);
	if (r == SIR_SUCCESS)
	{
		return it.result;
	}
	else
	{
		return NULL;
	}
}

SeyconResponse* SeyconService::sendUrlMessage(const wchar_t* url, ...) {
	va_list v;
	va_start (v, url);
	wchar_t wch[4000];
#ifdef WIN32
	vsnwprintf(wch, 3999, url, v);
#else
	vswprintf(wch, 3999, url, v);
#endif
	va_end (v);
	SeyconURLServiceIterator it;
	it.path = wch;
	ServiceIteratorResult r = iterateServers (it);
	if (r == SIR_SUCCESS)
	{
		return it.result;
	}
	else
	{
		return NULL;
	}
}


void SeyconService::resetServerStatus () {

	std::string servers;

	SeyconCommon::getServerList (servers);
	SeyconCommon::debug("Server list = {%s}", servers.c_str());

	hosts.clear();
	size_t pointer = servers.find_first_not_of(", ");
	while (pointer != std::string::npos) {
		size_t pointer2 =  servers.find_first_of (", ", pointer);
		if (pointer2 == std::string::npos)
			pointer2 = servers.size();
		hosts.push_back (servers.substr (pointer, pointer2-pointer));
		pointer = servers.find_first_not_of(", ", pointer2);
	}

	time_t t;
	time (&t);
	srand((int)t);

	if (hosts.size() <= 0)
		preferedServer = -1;
	else
		preferedServer = rand() % hosts.size();
}

#ifdef WIN32
static bool isWindows2000 ()
{
	DWORD dwVersion = GetVersion ();
	WORD wMajorVersion = (LOBYTE (LOWORD(dwVersion)));
	WORD wMinorVersion = (HIBYTE (LOWORD(dwVersion)));

	if (wMajorVersion < 5 ||
		wMajorVersion == 5 && wMinorVersion == 0 )
		return true;
	else
		return false;
}
#else
static bool isWindows2000 ()
{
	return false;
}
#endif
ServiceIteratorResult SeyconService::iterateServers(SeyconServiceIterator &it) {

	if (preferedServer == -1) {
		time_t t;
		time (&t);
		if (t - lastError < 60) {
			SeyconCommon::info ("Seycon server not responding");
			return SIR_ERROR;
		}
		resetServerStatus();
	}
	time_t startTime;
	time (&startTime);
	int port;
	if (isWindows2000())
		port = SeyconCommon::getServerAlternatePort();
	else
		port = SeyconCommon::getServerPort();

	for ( unsigned int j = 0; j < hosts.size(); j++) {
		int server = (j + preferedServer) % hosts.size();
		std::string host = hosts.at(server);
		ServiceIteratorResult r = it.iterate (host.c_str(), port);
		if (r == SIR_SUCCESS) {
			preferedServer = server;
			return SIR_SUCCESS;
		}
		if (r == SIR_ERROR)
			return SIR_ERROR;
	}
	time_t now;
	time (&now);
	if ( now - startTime > 3000) { // Si ha tardat més de 3 segons, donar el servei per no operatiu
		preferedServer = -1;
		time (&lastError);
	}

	return SIR_ERROR;
}



std::vector<std::string> SeyconService::hosts;
int SeyconService::preferedServer = -1;
time_t SeyconService::lastError = 0;

SeyconService::SeyconService() {
#ifdef WIN32
	std::string fileName;
	SeyconCommon::readProperty ("CertificateFile", fileName);
	if (fileName.size () == 0)
		return;

	int allocated = 0;
	BYTE *buffer = NULL;
	int size = 0;
	int chunk = 2048;
	FILE *file = fopen(fileName.c_str(), "rb");
	if (file != NULL) {
		int read = 0;
		do {
			allocated += chunk - (allocated - size) + 1;
			buffer = (BYTE*) realloc(buffer, allocated);
			read = fread(&buffer[size], 1, chunk, file);
			size += read;
			buffer[size] = '\0';
		} while (read > 0);
		pSeyconRootCert = CertCreateCertificateContext(X509_ASN_ENCODING,
				buffer, size);
	} else {
		SeyconCommon::warn("Unable to load %s\n", fileName.c_str());
	}
#endif
}



std::wstring SeyconService:: escapeString(const wchar_t* lpszSource) {

	std::wstring target;
	int len = wcslen (lpszSource);
	int i = 0;
	while (i < len) {
		if (lpszSource[i] == L'%') {
			target.append (L"%25");
		} else if (lpszSource[i] == L'&') {
			target.append (L"%26");
		} else if (lpszSource[i] == L'?') {
			target.append (L"%3F");
		} else if (lpszSource[i] == L'+') {
			target.append (L"%2B");
		} else {
			target.append (&lpszSource[i], 1);
		}
		i++;
	}
	return target;
}

std::wstring SeyconService::escapeString(const char* lpszSource) {
	std::wstring target = escapeString (MZNC_strtowstr(lpszSource).c_str());
	return target;
}


/********************************************************************
 * SEYCON RESPONSE
 *
 */
SeyconResponse::SeyconResponse (char *utf8Result, int size) {
	result = (char*) malloc(size);
	memcpy (result, utf8Result, size);
	this->size = size;
}

SeyconResponse::~SeyconResponse () {
	if (result != NULL) {
		memset (result, 0, size);
		free (result);
	}
}

bool SeyconResponse::getToken(int id, std::wstring &value) {
	std::string utf8value;
	if (findToken(id, utf8value, false)) {
		value.assign (MZNC_utf8towstr (utf8value.c_str()));
		return true;
	} else {
		value.clear();
		return false;
	}
}

bool SeyconResponse::getTail(int id, std::string &value) {
	std::string utf8value;
	if (findToken(id, utf8value, true)) {
		value.assign (MZNC_utf8tostr (utf8value.c_str()));
		return true;
	} else {
		value.clear();
		return false;
	}
}
///////////////////////////////////////////////////////////////
bool SeyconResponse::getToken(int id, std::string &value) {
	if (findToken(id, value, false)) {
		value.assign (MZNC_utf8tostr (value.c_str()));
		return true;
	} else {
		value.clear();
		return false;
	}
};

///////////////////////////////////////////////////////////////
bool SeyconResponse::getUtf8Token(int id, std::string &value) {
	if (findToken(id, value, false)) {
		return true;
	} else {
		value.clear();
		return false;
	}
};

///////////////////////////////////////////////////////////////
bool SeyconResponse::getUtf8Tail(int id, std::string &value) {
	if (findToken(id, value, true)) {
		return true;
	} else {
		value.clear();
		return false;
	}
};


 ///////////////////////////////////////////////////////////////
bool SeyconResponse::findToken(int id, std::string &r, bool tillEnd) {
 	int j = 0;
 	int i = 0;
 	while (i < id && j < size) {
 		wchar_t c = result[j];
 		if (c == '|') {
 			j++;
 			i++;
 		} else if (c == '\0')
 			i++;
 		else
 			j++;
 	}
 	char *start = &result[j];
 	int len;
 	for (len = 0; j+len < size && start[len] != '\0' && (tillEnd || start[len] != '|') ; len++)
 	{
 	}

 	r.assign (start, len);

 	return len > 0;
}

 ///////////////////////////////////////////////////////////////


