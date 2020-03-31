#include "Installer.h"
#include <string>
#include <winhttp.h>
#include <stdio.h>

using namespace std;

static bool debug = false;

BOOL HelperWriteKey(int bits, HKEY roothk, const char *lpSubKey, LPCTSTR val_name,
		DWORD dwType, const void *lpvData, DWORD dwDataSize);
		

static bool setProperty(const char *attribute, const char* value) {
	return HelperWriteKey(64, HKEY_LOCAL_MACHINE, "Software\\Soffid\\esso", attribute, 
			REG_SZ, (void*) value, lstrlen(value));

}

static void
CALLBACK asyncCallback(HINTERNET hInternet, DWORD_PTR dwContext,
		DWORD dwInternetStatus, LPVOID lpvStatusInformation OPTIONAL,
		DWORD dwStatusInformationLength) {
	if (debug) {
		if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SECURE_FAILURE) {
			DWORD status = (DWORD) lpvStatusInformation;
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
				wprintf(
						L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
				wprintf(
						L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
				wprintf(
						L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
				wprintf(
						L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
				wprintf(
						L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID\n");
			if (status & WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR)
				wprintf(
						L"Invalid cert WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR\n");
		}
	}
}

static LPSTR readURL(HINTERNET hSession, LPWSTR host, int port, LPCWSTR path,
		BOOL allowUnknownCA, size_t *pSize) {
	BOOL bResults = FALSE;
	HINTERNET hConnect = NULL, hRequest = NULL;

	DWORD dwDownloaded = -1;
	BYTE *buffer = NULL;

	WinHttpSetStatusCallback(hSession, asyncCallback,
			WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);

	if (debug) {
		wprintf(L"Connecting to %s:%d...\n", host, port);
	}
	hConnect = WinHttpConnect(hSession, host, port, 0);

	if (hConnect) {
		if (debug) {
			wprintf(L"Performing request %s...\n", path);
		}

		hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL,
				WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
				WINHTTP_FLAG_SECURE);
	}

	// Send a request.
	if (hRequest) {
		if (debug)
			wprintf(L"Sending request ...\n");
		WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, NULL, 0);
		if (allowUnknownCA) {
			DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
			WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS,
					(LPVOID) &flags, sizeof flags);
		}
		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
				0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	}

	if (bResults && allowUnknownCA) {
		// Agreagar la CA ROOT
		PCERT_CONTEXT context;
		DWORD dwSize = sizeof context;
		BOOL result = WinHttpQueryOption(hRequest,
				WINHTTP_OPTION_SERVER_CERT_CONTEXT, &context, &dwSize);
		if (!result) {
			wprintf(L"Cannot get context\n");
			notifyError();
		}
		PCCERT_CONTEXT issuerContext = CertFindCertificateInStore(
				context->hCertStore, X509_ASN_ENCODING, 0, CERT_FIND_ISSUER_OF,
				context, NULL);
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
	if (bResults) {
		if (debug)
			wprintf(L"Waiting for response....\n");

		bResults = WinHttpReceiveResponse(hRequest, NULL);
	}

	// Keep checking for data until there is nothing left.
	DWORD used = 0;
	if (bResults) {
		const DWORD chunk = 4096;
		DWORD allocated = 0;
		do {
			if (used + chunk > allocated) {
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
	if (!bResults) {
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
		else if (dw != ERROR_SUCCESS) {
			log ("Unkonwn error %d\n", dw);
		}
		notifyError();
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

bool configure(LPCSTR mazingerDir, const char* url) {
	URL_COMPONENTS urlComp;
	// Initialize the URL_COMPONENTS structure.
	ZeroMemory(&urlComp, sizeof(urlComp));
	// Set required component lengths to non-zero
	// so that they are cracked.
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
	wchar_t* wUrl = (wchar_t*) malloc (strLen *sizeof (wchar_t));
	mbstowcs (wUrl, url, strlen(url)+1);

	// Use WinHttpOpen to obtain a session handle.
	HINTERNET hSession = WinHttpOpen(L"Eris Agent/1.0",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if (hSession) {
		WinHttpSetStatusCallback(hSession, asyncCallback,
				WINHTTP_CALLBACK_FLAG_SECURE_FAILURE, 0);
		WinHttpCrackUrl(wUrl, wcslen(wUrl), 0, &urlComp);
	}

	// Obtener la lista de host
	size_t size;
	log("Connecting to https://%ls:%d/cert", szHostName, urlComp.nPort);

	LPCSTR cert = readURL(hSession, szHostName, urlComp.nPort, L"/cert",
			true, &size);

	if (cert == NULL) {
		log ("Error getting certificate");
		notifyError();

		return FALSE;
	}

	std::string fileName = mazingerDir;
	fileName += "\\root.cer";

	FILE *f = fopen(fileName.c_str(), "wb");
	if (f == NULL) {
		log ("Error generating file %s", fileName.c_str());
		return FALSE;
	} else {
		fwrite(cert, size, 1, f);
		fclose(f);
	}
	if (!setProperty("CertificateFile", fileName.c_str())) {
		log ("Error setting certificateFile");
		notifyError();
	}
	
	char hostName[4096];
	wcstombs(hostName, szHostName, 4095);

	if (!setProperty("SSOServer", hostName)) {
		log ("Error setting SSOServer");
		notifyError();
	}
	char szPort[100];
	sprintf(szPort, "%d", urlComp.nPort);
	if (!setProperty("seycon.https.port", szPort)) {
		log ("Error setting seycon.https.port");
		notifyError();
	}
	return true;

}

