
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

#include <glib.h>
#include <string>
#include <stdlib.h>
#include <iconv.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/stat.h>
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
	bool waitForNetwork = true;
private:
	static time_t lastError ;
};

SeyconServiceIterator::SeyconServiceIterator () {

}

SeyconServiceIterator::~SeyconServiceIterator () {

}

#ifdef WIN32


int nCerts = 0;
static PCCERT_CONTEXT *pSeyconRootCert = NULL;

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

		HINTERNET hSession = WinHttpOpen(L"Mazinger Agent/1.1",
					WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
					WINHTTP_NO_PROXY_BYPASS, 0);

		if (hSession == NULL)
			return SIR_ERROR;

		WinHttpSetStatusCallback(hSession, asyncCallback,
				WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);

		WinHttpSetTimeouts(
				hSession,
		  5000, // 5 segons __in  int dwResolveTimeout,
		  15000, // 15 segons __in  int dwConnectTimeout,
		  60000, // 60 segons, send timeout
		  120000 // 2 minuts   receive timeout
		);

//		wchar_t *path2 = wcsdup (path);
//		wchar_t *query = wcsstr(path2, L"?");
//		if (query != NULL)
//			*query = L'\0';
//		SeyconCommon::debug("Performing request https://%hs:%d%ls...\n", host, port, path2);
//		free (path2);

		/*	WinHttpSetOption ( hSession, WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
		 WINHTTP_NO_CLIENT_CERT_CONTEXT,
		 0);*/
		std::wstring wsHost = MZNC_strtowstr (host);
		hConnect = WinHttpConnect(hSession, wsHost.c_str(), port, 0);

		if (hConnect) {
			hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
					WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
					WINHTTP_FLAG_SECURE);

			unsigned long timeout = 120000;
			WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT,
					(LPVOID) &timeout, sizeof timeout); // 2 minuts de timeout

			WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
					WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

	#if 0
			WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT,
						(LPVOID) INFINITE, 0);
	#endif
		}

		// Send a request.
		if (hRequest) {
			if (nCerts > 0) {
				DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
				WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, (LPVOID)
						& flags, sizeof flags);
			}
			bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
					0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
			// Verificar CA root
			if (bResults && nCerts > 0) {
				// Verificar la CA ROOT
				PCCERT_CONTEXT context;
				DWORD dwSize = sizeof context;
				BOOL result = WinHttpQueryOption(hRequest,
						WINHTTP_OPTION_SERVER_CERT_CONTEXT, &context, &dwSize);
				if (!result || context == NULL) {
					SeyconCommon::debug("No cert loaded");
					bResults = NULL;
				} else {
					PCCERT_CONTEXT issuerContext = CertFindCertificateInStore(
							context->hCertStore, X509_ASN_ENCODING, 0,
							CERT_FIND_ISSUER_OF, context, NULL);
					if (issuerContext == NULL) {
						issuerContext = context;
					}
					bool found = false;
					for (int i = 0; !found && i < nCerts; i++) {
						if (pSeyconRootCert[i] != NULL) {
							if (CertComparePublicKeyInfo(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
									&issuerContext->pCertInfo->SubjectPublicKeyInfo,
									&pSeyconRootCert[i]->pCertInfo->SubjectPublicKeyInfo)) {
								found = true;
							}
						}
					}
					if (! found) {
						std::string ignore;
						SeyconCommon::readProperty("ignoreCert", ignore);
						if (ignore != "true")
						{
							SeyconCommon::info("Received certificate not allowed\n");
							char pszNameString[256] = "<unknown>";

							CertGetNameStringA(issuerContext,
									CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
									pszNameString, 128);

							SeyconCommon::warn ("ERROR: Invalid CA %s.\n",
									pszNameString);
							bResults = NULL;
							SetLastError(ERROR_WINHTTP_SECURE_FAILURE);
						}
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
				SeyconCommon::warn("Error: Cannot connect to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
				SeyconCommon::warn("Error: Client CERT required connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_CONNECTION_ERROR)
				SeyconCommon::warn("Error: Connection error connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_STATE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_INCORRECT_HANDLE_STATE connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_INCORRECT_HANDLE_TYPE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_INCORRECT_HANDLE_TYPE connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_INTERNAL_ERROR)
				SeyconCommon::warn("Error: ERRR_WINHTTP_INTERNAL_ERROR connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_INVALID_URL)
				SeyconCommon::warn("Error: ERROR_WINHTTP_INVALID_URL connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_LOGIN_FAILURE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_LOGIN_FAILURE connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_NAME_NOT_RESOLVED)
				SeyconCommon::warn("Error: ERROR_WINHTTP_NAME_NOT_RESOLVED connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_OPERATION_CANCELLED)
				SeyconCommon::warn("Error: ERROR_WINHTTP_OPERATION_CANCELLED connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW)
				SeyconCommon::warn("Error: ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_SECURE_FAILURE)
				SeyconCommon::warn("Error: ERROR_WINHTTP_SECURE_FAILURE connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_SHUTDOWN)
				SeyconCommon::warn("Error: ERROR_WINHTTP_SHUTDOWN connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_TIMEOUT)
				SeyconCommon::warn("Error: ERROR_WINHTTP_TIMEOUT connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_UNRECOGNIZED_SCHEME)
				SeyconCommon::warn("Error: ERROR_WINHTTP_UNRECOGNIZED_SCHEME connecting to %s:%d\n", host, port);
			else if (dw == ERROR_NOT_ENOUGH_MEMORY)
				SeyconCommon::warn("Error: ERROR_NOT_ENOUGH_MEMORY connecting to %s:%d\n", host, port);
			else if (dw == ERROR_INVALID_PARAMETER)
				SeyconCommon::warn("Error: ERROR_INVALID_PARAMETER connecting to %s:%d\n", host, port);
			else if (dw == ERROR_WINHTTP_RESEND_REQUEST)
				SeyconCommon::warn("Error:  ERROR_WINHTTP_RESEND_REQUEST connecting to %s:%d\n", host, port);
			else if (dw ==  ERROR_WINHTTP_INVALID_SERVER_RESPONSE ) {
				if (firstTime) {
					firstTime = false;
					repeat = true;
					SeyconCommon::warn("Error:  ERROR_WINHTTP_INVALID_SERVER_RESPONSE connecting to %s:%d. Retrying...\n", host, port);
				} else
					SeyconCommon::warn("Error:  ERROR_WINHTTP_INVALID_SERVER_RESPONSE  connecting to %s:%d\n", host, port);
			} else
				SeyconCommon::warn("Error:  %d  connecting to %s:%d\n", (int) dw, host, port);
		}

		// Close any open handles.
		if (hRequest)
			WinHttpCloseHandle(hRequest);
		if (hConnect)
			WinHttpCloseHandle(hConnect);
		if (hSession)
			WinHttpCloseHandle(hSession);

		SetLastError(dw);
	} while (repeat);
	return iteratorResult;
}


#else

struct TempBuffer {
	char *buffer;
	int size;
};


static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	struct TempBuffer *p = (struct TempBuffer*) userdata;
	if (p->size == 0)
	{
		p->buffer = (char*) malloc (nmemb);
		if (p->buffer == NULL) return 0;
		memcpy (p->buffer, ptr, nmemb);
		p->size = nmemb;
	}
	else
	{
		p->buffer = (char*) realloc (p->buffer, nmemb + p->size);
		if (p->buffer == NULL) return 0;
		memcpy(&p->buffer[p->size], ptr, nmemb);
		p->size += nmemb;
	}
	return nmemb;
}

static bool init = false;
static std::string certsFileName;

ServiceIteratorResult SeyconURLServiceIterator::iterate (const char* hostName, size_t dwPort) {
	std::string certs;
	std::string fileName;
	SeyconCommon::readProperty("CertificateFile", fileName);
	SeyconCommon::readProperty("certs", certs);

	if (!init) {
		init = true;
	    curl_global_init(CURL_GLOBAL_ALL);
	}

	if (certs.size() > 0 && certsFileName.empty()) {
		char *home = getenv("HOME");
		if (home == NULL) {
			certsFileName = "/etc/mazinger/certs.pem";
		} else {
			certsFileName = getenv("HOME");
			certsFileName += "/.config";
			mkdir(certsFileName.c_str(), 0700);
			certsFileName += "/mazinger";
			mkdir(certsFileName.c_str(), 0700);
			certsFileName += "/certs.pem";
		}

		FILE *f = fopen (certsFileName.c_str(), "w");
		if (f != NULL) {
			const char* dup = certs.c_str();
			bool first = true;
			int column = 0;
			for (size_t i = 0; dup[i]; i++) {
				if (first) {
					fprintf(f , "-----BEGIN CERTIFICATE-----\n");
					first = false;
					column = 0;
				}
				if (dup[i] == ' ') {
					fprintf(f , "\n-----END CERTIFICATE-----\n");
					first = true;
					column = 0;
				} else {
					if (column == 64) {
						fprintf (f, "\n");
						column = 0;
					}
					fwrite(&dup[i], 1, 1, f);
					column ++;
				}
			}
			fclose(f);
		}
	}

	if (fileName.size() == 0 && certs.size() == 0) {
		SeyconCommon::warn("Unknown certificate file. Please configure /etc/mazinger\n");
		return SIR_ERROR;
	}
	std::string timeoutStr;
	SeyconCommon::readProperty("Timeout", timeoutStr);
	static int timeout = 60;
	sscanf ( timeoutStr.c_str(), "%d", &timeout);

	bool repeat;
	int retries = 0;
	do
	{
		repeat = false;
		char num[10];
		sprintf (num, "%d", (int) dwPort);

		std::string szUri = MZNC_wstrtostr(path);
		std::string szUrl = "https://";
		szUrl += hostName;
		szUrl += ":";
		szUrl += num;
		szUrl += szUri;

		SeyconCommon::debug("Performing request %s....", szUrl.c_str());

		  // Sets return buffer
		struct TempBuffer write_data;
		write_data. size = 0;
		write_data. buffer = NULL;

		CURL *msg = curl_easy_init();
		if (msg == NULL) {
			SeyconCommon::warn("Unable to create curl message\n");
			return SIR_ERROR;
		}
		curl_easy_setopt(msg, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(msg, CURLOPT_HEADER, 0L);
		curl_easy_setopt(msg, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(msg, CURLOPT_NOSIGNAL, 1L);
		//  curl_easy_setopt(ch, CURLOPT_SSLCERTTYPE, "PEM");
		curl_easy_setopt(msg, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(msg, CURLOPT_URL, szUrl.c_str());
		if (certs.size() > 0) {
			SeyconCommon::debug("Using certs %s", certsFileName.c_str());
			curl_easy_setopt(msg, CURLOPT_CAINFO, certsFileName.c_str());
		} else {
			curl_easy_setopt(msg, CURLOPT_CAINFO, fileName.c_str());
		}
		curl_easy_setopt(msg, CURLOPT_DNS_CACHE_TIMEOUT, 3);
		curl_easy_setopt(msg, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(msg, CURLOPT_TIMEOUT, 10);
		curl_easy_setopt(msg, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(msg, CURLOPT_WRITEDATA, &write_data);

		CURLcode rv = curl_easy_perform(msg);

		SeyconCommon::wipe(szUri);
		SeyconCommon::wipe(szUrl);

		if (rv != CURLE_OK) {
			curl_easy_cleanup(msg);
			SeyconCommon::warn ("Error connecting to host: %s:%d: %s.", hostName, dwPort,
					curl_easy_strerror(rv));
			return SIR_RETRY;
		} else {
			long status;
		    curl_easy_getinfo(msg, CURLINFO_RESPONSE_CODE, &status);
			curl_easy_cleanup(msg);
			if (status == 204) // HTTP-NO-DATA
			{
				SeyconCommon::debug ("Success HTTP/204 (No data)", status);
				return SIR_ERROR;
			}
			else if (status == 7 && retries < 2) // Ćonnexion closed
			{
				retries ++;
				repeat = true;
				sleep(3);
			}
			else if (status != 200) // HTTP-OK
			{
				SeyconCommon::debug ("Error HTTP/%d at host: %s:%d: %s", status, hostName, dwPort,
						curl_easy_strerror(rv));
				return SIR_RETRY;
			} else {
				result = new SeyconResponse ((char*) write_data.buffer, write_data.size);
				return SIR_SUCCESS;
			}
		}
	} while (repeat);

	return SIR_ERROR;
}

#endif

SeyconURLServiceIterator::~SeyconURLServiceIterator () {

}

SeyconResponse*  SeyconService::sendUrlMessage(const char* host, int port, const wchar_t* url, ...) {
#ifdef WIN32
	if (nCerts == 0) {
		return NULL;
	}
#endif

	va_list v;
	va_start (v, url);
	wchar_t wch[32000];
	wch[0] == L'\0';
#ifdef WIN32
	vsnwprintf(wch, 31999, url, v);
#else
	vswprintf(wch, 31999, url, v);
#endif
	va_end (v);

	if (wch[0] == L'\0')
		SeyconCommon::debug("URL too big problem invoking %ls", url);

	SeyconCommon::debug("Invoking %ls", wch);
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
#ifdef WIN32
	if (nCerts == 0) {
		return NULL;
	}
#endif

	va_list v;
	va_start (v, url);
	wchar_t wch[32000];
	wch[0] = wch[31999] = '\0';
#ifdef WIN32
	vsnwprintf(wch, 31998, url, v);
#else
	vswprintf(wch, 31998, url, v);
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


void SeyconService::resetCertificates () {
#ifdef WIN32
	pSeyconRootCert = NULL;
	nCerts = 0;
#else
	certsFileName.clear();
#endif
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
		SeyconCommon::debug("Server {%s}", servers.substr (pointer, pointer2-pointer).c_str());
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
	if (nCerts == 0) {
		std::string certs;
		SeyconCommon::readProperty ("certs", certs);
		if (certs.size() > 0) {
			int pos = 0;
			char *dup = strdup(certs.c_str());
			char *start = dup;
			for (pos = 0; dup[pos]; pos++) {
				if (dup[pos] == ' ') {
					nCerts ++;
				}
			}
			pSeyconRootCert = (PCCERT_CONTEXT *) malloc ( nCerts * sizeof (PCCERT_CONTEXT));
			nCerts = 0;
			for (pos = 0; dup[pos]; pos++) {
				if (dup[pos] == ' ') {
					dup[pos] = '\0';
					std::string cert = SeyconCommon::fromBase64(start);
					pSeyconRootCert[nCerts++] = CertCreateCertificateContext(X509_ASN_ENCODING, (PBYTE) cert.c_str(), cert.size());
					start = &dup[pos+1];
				}
			}
		} else {
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
				nCerts = 1;
				pSeyconRootCert = (PCCERT_CONTEXT *) malloc ( 1 * sizeof (PCCERT_CONTEXT));
				pSeyconRootCert[0] = CertCreateCertificateContext(X509_ASN_ENCODING,
						buffer, size);
				fclose (file);
			} else {
				SeyconCommon::warn("Unable to load certificate file %s\n", fileName.c_str());
			}
		}
	}
#endif
}


static std::wstring escape( const char *lpszSource) {
	std::string target;
	int len = strlen (lpszSource);
	int i = 0;
	while (i < len) {
		if (lpszSource[i] == '%') {
			target.append ("%25");
		} else if (lpszSource[i] == ' ') {
			target.append ("+");
		} else if (lpszSource[i] == '%') {
			target.append ("%25");
		} else if (lpszSource[i] == '&') {
			target.append ("%26");
		} else if (lpszSource[i] == '?') {
			target.append ("%3F");
		} else if (lpszSource[i] == '+') {
			target.append ("%2B");
		} else if (lpszSource[i] == '#') {
			target.append ("%23");
		} else if (lpszSource[i] >= 255 || lpszSource[i] < 32) {
			target.append("%");
			char ach[10];
			sprintf(ach, "%02x", lpszSource[i] );
			target.append (ach);
		} else {
			target.append (&lpszSource[i], 1);
		}
		i++;
	}
	return MZNC_strtowstr(target.c_str());
}

std::wstring SeyconService:: escapeString(const wchar_t* lpszSource) {
	std::string s = MZNC_wstrtoutf8(lpszSource);
	return escape(s.c_str());
}

std::wstring SeyconService::escapeString(const char* lpszSource) {
	std::string s = MZNC_strtoutf8(lpszSource);
	return escape(s.c_str());
}


/********************************************************************
 * SEYCON RESPONSE
 *
 */
SeyconResponse::SeyconResponse (char *utf8Result, int size) {
	result = (char*) malloc(size+1);
	memcpy (result, utf8Result, size);
	result[size] = '\0';
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


