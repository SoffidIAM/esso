/*
 * Utils.cpp
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */


#include <windows.h>
#include <stdio.h>

#include "Utils.h"

#define TRACE if (logFile != NULL) fprintf (logFile, "En %s:%d\n", __FILE__, __LINE__);

Utils::Utils() {
}

Utils::~Utils() {
}


void Utils::bstr2str ( std::string &str, BSTR bstr)
{
	if (bstr == NULL)
	{
		str.clear ();
	}
	else
	{
		DWORD dwRequiredSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) bstr,
				SysStringLen(bstr), NULL, 0,
				NULL,NULL);
		LPSTR lpszBuffer = (LPSTR) malloc(dwRequiredSize+2);
		dwRequiredSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) bstr,
				 SysStringLen(bstr), lpszBuffer, dwRequiredSize+1,
				NULL,NULL);
		lpszBuffer[dwRequiredSize] = '\0';
		str.assign(lpszBuffer);
		free(lpszBuffer);
		SysFreeString(bstr);
	}
}


BSTR Utils::str2bstr (const char*lpsz)
{
	if (lpsz == NULL)
		return NULL;
	else
	{
		DWORD dwRequiredSize = MultiByteToWideChar(CP_UTF8, 0, lpsz,
				-1, NULL, 0);
		DWORD dwBytes = sizeof (wchar_t) * dwRequiredSize;
		LPWSTR lpwszBuffer = (LPWSTR) malloc (dwBytes+1);
		MultiByteToWideChar(CP_UTF8, 0, lpsz,
				-1, lpwszBuffer, dwBytes);
		lpwszBuffer[dwRequiredSize] = L'\0';
		BSTR bstr = SysAllocString(lpwszBuffer);
		free (lpwszBuffer);
		return bstr;
	}

}
