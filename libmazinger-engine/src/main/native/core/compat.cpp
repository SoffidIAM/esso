/*
 * compat.cpp
 *
 *  Created on: 19/04/2011
 *      Author: u07286
 */

#include <MazingerInternal.h>
#include <SeyconServer.h>

#ifdef WIN32

HANDLE hMutex = NULL;
static HANDLE getMutex() {
	if (hMutex == NULL)
	{
		hMutex = CreateMutexA(NULL, FALSE, NULL);

	}
	return hMutex;
}


bool MZNC_waitMutex () {
	HANDLE hMutex = getMutex();
	if (hMutex != NULL)
	{
		DWORD dwResult = WaitForSingleObject (hMutex, INFINITE);
		if (dwResult == WAIT_OBJECT_0)
		{
			return true;
		}
	}
	return false;
}

void MZNC_endMutex () {
	HANDLE hMutex = getMutex();
	if (hMutex != NULL)
	{
		ReleaseMutex (hMutex);
	}
}

void MZNC_destroyMutex () {
	HANDLE hMutex = getMutex();
	if (hMutex != NULL)
	{
		CloseHandle (hMutex);
	}
}

HANDLE hMutex2 = NULL;
static HANDLE getMutex2() {
	if (hMutex2 == NULL)
	{
		hMutex2 = CreateMutexA(NULL, FALSE, NULL);

	}
	return hMutex2;
}


bool MZNC_waitMutex2 () {
	HANDLE hMutex = getMutex2();
	if (hMutex != NULL)
	{
		DWORD dwResult = WaitForSingleObject (hMutex, INFINITE);
		if (dwResult == WAIT_OBJECT_0)
		{
			return true;
		}
	}
	return false;
}

void MZNC_endMutex2 () {
	HANDLE hMutex = getMutex2();
	if (hMutex != NULL)
	{
		ReleaseMutex (hMutex);
	}
}

void MZNC_destroyMutex2 () {
	HANDLE hMutex = getMutex2();
	if (hMutex != NULL)
	{
		CloseHandle (hMutex);
	}
}

HANDLE hMutex3 = NULL;
static HANDLE getMutex3() {
	if (hMutex3 == NULL)
	{
		hMutex3 = CreateMutexA(NULL, FALSE, NULL);

	}
	return hMutex3;
}


bool MZNC_waitMutex3 () {
	HANDLE hMutex = getMutex3();
	if (hMutex != NULL)
	{
		DWORD dwResult = WaitForSingleObject (hMutex, INFINITE);
		if (dwResult == WAIT_OBJECT_0)
		{
			return true;
		}
	}
	return false;
}

void MZNC_endMutex3 () {
	HANDLE hMutex = getMutex3();
	if (hMutex != NULL)
	{
		ReleaseMutex (hMutex);
	}
}

void MZNC_destroyMutex3 () {
	HANDLE hMutex = getMutex3();
	if (hMutex != NULL)
	{
		CloseHandle (hMutex);
	}
}

const char *MZNC_getCommandLine() {
	return GetCommandLineA ();
}


std::wstring MZNC_utf8towstr (const char *pszString) {
	std::wstring result;
	// Convertir de UTF-8 al joc de caracters nadiu, primer en widechar i despres en char
	int cWideCharsNeeded = 1+MultiByteToWideChar(CP_UTF8,
			0,
			pszString,
			-1, // Null terminated
			NULL,
			0);
	if (cWideCharsNeeded <= 1)
	{
		return result;
	}
	LPWSTR lpwstr = (LPWSTR) malloc (cWideCharsNeeded * sizeof (wchar_t));
	int len = MultiByteToWideChar(CP_UTF8,
			0,
			pszString,
			-1, // Null terminated
			lpwstr,
			cWideCharsNeeded);
	if (len > 0)
	{
		result.assign (lpwstr);
		free (lpwstr);
	}

	return result;
}

std::string MZNC_wstrtostr (const wchar_t* lpwstr) {
	std::string result;
	BOOL bUsedDefaultChar;
	int cCharsNeeded = 1+WideCharToMultiByte(CP_ACP,
			0,
			lpwstr,
			-1, // Null terminated
			NULL,
			0, NULL, NULL);
	if (cCharsNeeded <= 1)
	{
		return result;
	}
	LPSTR lpstr = (LPSTR) malloc (cCharsNeeded);
	int len2 = WideCharToMultiByte(CP_ACP,
			0,
			lpwstr,
			-1, // Null Terminated
			lpstr,
			cCharsNeeded, NULL, &bUsedDefaultChar);
	lpstr[len2] = '\0';
	result.assign(lpstr);
	free (lpstr);
	return result;
}

std::string MZNC_utf8tostr (const char *pszString) {
	std::wstring wstr = MZNC_utf8towstr (pszString);
	return MZNC_wstrtostr(wstr.c_str());
}

std::wstring MZNC_strtowstr (const char *pszString) {
	std::wstring result;
	if (pszString == null)
		return result;
	// Convertir de UTF-8 al joc de caracters nadiu, primer en widechar i despres en char
	int cWideCharsNeeded = 1+MultiByteToWideChar(CP_ACP,
			0,
			pszString,
			-1, // Null terminated
			NULL,
			0);
	if (cWideCharsNeeded <= 1)
	{
		return result;
	}
	LPWSTR lpwstr = (LPWSTR) malloc (cWideCharsNeeded * sizeof (wchar_t));
	int len = MultiByteToWideChar(CP_ACP,
			0,
			pszString,
			-1, // Null terminated
			lpwstr,
			cWideCharsNeeded);

	if (len > 0)
	{
		result.assign (lpwstr);
		free (lpwstr);
	}
	return result;
}

std::string MZNC_wstrtoutf8 (const wchar_t* lpwstr) {
	std::string result;
	BOOL bUsedDefaultChar;
	int cCharsNeeded = 1+WideCharToMultiByte(CP_UTF8,
			0,
			lpwstr,
			-1, // Null terminated
			NULL,
			0, NULL, NULL);
	if (cCharsNeeded <= 1)
	{
		return result;
	}
	LPSTR lpstr = (LPSTR) malloc (cCharsNeeded);
	int len2 = WideCharToMultiByte(CP_UTF8,
			0,
			lpwstr,
			-1, // Null Terminated
			lpstr,
			cCharsNeeded, NULL, NULL);
	lpstr[len2] = '\0';
	result.assign(lpstr);
	free (lpstr);
	return result;
}

std::string MZNC_strtoutf8 (const char *pszString) {
	std::wstring wstr = MZNC_strtowstr (pszString);
	return MZNC_wstrtoutf8(wstr.c_str());
}

static char *s_username = NULL;
void MZNC_setUserName (const char *username ) {
	if (s_username != NULL)
		free (s_username);
	if (username == NULL)
		s_username = NULL;
	else
		s_username = strdup (username);
}
const char * MZNC_getUserName ( ) {
	if (s_username != NULL)
		return s_username;
	static char achUserName[1024] = "";
	DWORD dwSize = sizeof achUserName;
	GetUserNameA (achUserName, &dwSize);
	return achUserName;
}


const char *MZNC_getHostName () {
    static char achHostName[ 4096 ] = "";

    std::string s;

	DWORD dwSize = sizeof achHostName;
    if ( SeyconCommon::readProperty("soffid.hostname.format", s) && s == "short")
    {
		GetComputerNameA(achHostName, &dwSize);
    }
    else
    {
		GetComputerNameA(achHostName, &dwSize);
        dwSize = sizeof achHostName;
		if ( GetComputerNameExA(ComputerNameDnsFullyQualified, achHostName, &dwSize) != 0 ||
				strlen(achHostName) == 0)
			GetComputerNameA(achHostName, &dwSize);
    }
	for (int i = 0; achHostName[i]; i++)
		achHostName[i] = tolower(achHostName[i]);

	return achHostName ;
}

#else
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <unistd.h>
#include <langinfo.h>
#include <errno.h>
#include <wchar.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <locale.h>

const char *MZNC_getCommandLine() {
	static char *cmdLine = "";
	if (cmdLine == NULL)
	{
		FILE *f =fopen ("/proc/self/cmdline", "r");
		if (f != NULL) {
			int allocated = 64;
			cmdLine =  (char*) malloc(allocated);
			char ch;
			int read = fread (&ch, 1, 1, f);
			int j = 0;
			while (read > 0) {
				if (j + 3 > allocated)
				{
					allocated += allocated;
					cmdLine = (char*) realloc (cmdLine, allocated);
				}
				switch (ch)
				{
				case '\0':
					cmdLine[j++] = ' ';
					break;
				case ' ':
				case '\'':
				case '\"':
				case '$':
				case '[':
				case '{':
				case '(':
				case ')':
				case ']':
				case '}':
					cmdLine[j++] = '\\';
				default:
					cmdLine[j++] = ch;
					break;
				}
				read = fread (&ch, 1, 1, f);
			}
			cmdLine[j-1] = '\0';
		}
	}
	return cmdLine;

}


static void ensureBufferCapacity (char * &result, char * &out, size_t &outSize, size_t &allocated, size_t needed) {
	if (allocated < needed) {
		outSize += needed - allocated ;
		allocated = needed;
		result = (char*) realloc (result, allocated);
		out = & result[allocated-outSize];
	}
}

static char * convert (const char *pszString, const char *fromCharset, const char *toCharset, int len) {
	iconv_t ict = iconv_open (toCharset == NULL? nl_langinfo(CODESET): toCharset,
			fromCharset == NULL? nl_langinfo(CODESET): fromCharset);
	if (ict == (iconv_t) -1 ) {
		fprintf (stderr, "Unable to transform to charset %s from charset %s\n",
				fromCharset, toCharset);
		exit(1);
	}
	char *oldLocale = NULL;
	if (fromCharset == NULL || toCharset == NULL) {
//		oldLocale = setlocale(LC_CTYPE, "");
	}
	size_t inSize = len;
	size_t outSize = inSize;
	size_t allocated = outSize;
	char *out = (char*) malloc( allocated );

	char *result = out;
	char *in = (char*) pszString;
	while (inSize > 0) {
		size_t s = iconv (ict, &in, &inSize, &out, &outSize);
		if (s == (size_t) -1)
		{
			if (errno == E2BIG) {
				ensureBufferCapacity (result, out, outSize, allocated, allocated + allocated);
			} else if (errno == EILSEQ ){
				in ++;
				inSize --;
			} else if (errno == EINVAL ){
				in ++;
				inSize --;
			}
		}
	}

	iconv (ict, NULL, NULL, &out, &outSize);
	iconv_close (ict);

//	if (oldLocale != NULL)
//		setlocale(LC_CTYPE, oldLocale);
	return result;
}


std::string MZNC_utf8tostr (const char *pszString) {
	char *sz = convert (pszString, "UTF-8", NULL, strlen(pszString)+1);
	std::string result = sz;
	free (sz);
	return result;
}
std::string MZNC_strtoutf8 (const char *pszString) {
	char *sz = convert (pszString, NULL, "UTF-8", strlen(pszString)+1);
	std::string result = sz;
	free (sz);
	return result;
}

std::wstring MZNC_strtowstr (const char *pszString) {
	wchar_t *sz = (wchar_t*) convert (pszString, NULL, "WCHAR_T", strlen(pszString)+1);
	std::wstring result = sz;
	free (sz);
	return result;
}

std::string MZNC_wstrtostr (const wchar_t *pszString) {
	char *sz =  convert ((const char*) pszString, "WCHAR_T", NULL, sizeof(wchar_t) * (wcslen(pszString)+1));
	std::string result = sz;
	free (sz);
	return result;
}

std::string MZNC_wstrtoutf8 (const wchar_t *pszString) {
	char *sz =  convert ((const char*) pszString, "WCHAR_T", "UTF-8", sizeof(wchar_t) * (wcslen(pszString)+1));
	std::string result = sz;
	free (sz);
	return result;
}

std::wstring MZNC_utf8towstr (const char *pszString) {
	wchar_t *sz =  (wchar_t*) convert (pszString, "UTF-8", "WCHAR_T", strlen(pszString)+1);
	std::wstring result = sz;
	free (sz);
	return result;
}

static char *s_username = NULL;

void MZNC_setUserName (const char *username ) {
	if (s_username != NULL)
		free (s_username);
	if (username == NULL || username[0] == '\0')
		s_username = NULL;
	else
		s_username = strdup (username);
}

const char * MZNC_getUserName ( ) {
	if (s_username != NULL)
		return s_username;
	const char * logName = getenv ("LOGNAME");
	return logName == NULL ? "nobody": logName;
}

const char *MZNC_getHostName () {
    static char achHostName[1024] = "";
    gethostname(achHostName, sizeof achHostName);
    achHostName[1023] = '\0';
    return achHostName ;
}



static sem_t semaphore;
static bool initialized = false;
sem_t* getSemaphore () {
	if (! initialized)
		sem_init ( &semaphore, true, 1);

	return &semaphore;
}


bool MZNC_waitMutex () {
	sem_t* s = getSemaphore();
	if (s != NULL && sem_wait(s) >= 0)
			return true;
	return false;
}

void MZNC_endMutex () {
	sem_t* s = getSemaphore();
	if (s != NULL)
		sem_post(s);
}


void MZNC_destroyMutex () {
	sem_t* s = getSemaphore();
	sem_destroy(s);
}

static sem_t semaphore2;
static bool initialized2 = false;
sem_t* getSemaphore2 () {
	if (! initialized2)
		sem_init ( &semaphore2, true, 1);

	return &semaphore2;
}

bool MZNC_waitMutex2 () {
	sem_t* s = getSemaphore2();
	if (s != NULL && sem_wait(s) >= 0)
			return true;
	return false;
}

void MZNC_endMutex2 () {
	sem_t* s = getSemaphore2();
	if (s != NULL)
		sem_post(s);
}


void MZNC_destroyMutex2 () {
	sem_t* s = getSemaphore2();
	sem_destroy(s);
}

static sem_t semaphore3;
static bool initialized3 = false;
sem_t* getSemaphore3 () {
	if (! initialized)
		sem_init ( &semaphore, true, 1);

	return &semaphore;
}


bool MZNC_waitMutex3 () {
	sem_t* s = getSemaphore();
	if (s != NULL && sem_wait(s) >= 0)
			return true;
	return false;
}

void MZNC_endMutex3 () {
	sem_t* s = getSemaphore();
	if (s != NULL)
		sem_post(s);
}


void MZNC_destroyMutex3 () {
	sem_t* s = getSemaphore();
	sem_destroy(s);
}

#endif

