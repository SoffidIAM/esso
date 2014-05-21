/*
 * EnvironmentHandler.cpp
 *
 *  Created on: 21/01/2011
 *      Author: u07286
 */

#include "sayaka.h"

#include <ctype.h>
#include <windows.h>
#include <lm.h>
#include <security.h>
#include <userenv.h>
#include <shlobj.h>
#include <ntsecapi.h>
#include "EnvironmentHandler.h"

EnvironmentHandler::EnvironmentHandler(wchar_t* pEnv) {
	m_len = 0;
	wchar_t* p = pEnv;
	while (*p)
	{
		wprintf (L"[%s]\n", p);
		m_len += wcslen(p) + 1;
		p += wcslen(p) + 1;
	}
	m_len ++;
	m_pEnv = (wchar_t*) malloc (m_len * sizeof (wchar_t));
	memcpy (m_pEnv, pEnv, m_len * sizeof (wchar_t));

}

EnvironmentHandler::EnvironmentHandler() {
	m_pEnv = (wchar_t*) malloc( sizeof (wchar_t));
	m_pEnv[0] = L'\0';
	m_len = 1;
}


EnvironmentHandler::~EnvironmentHandler() {
}

void EnvironmentHandler::addVariable (const wchar_t *var, const wchar_t*value) {
	wprintf (L"Added %s=%s\n", var, value);
	int len1 = wcslen(var);
	int len2 = wcslen(value);
	int end = m_len -1 ;
	m_pEnv = (wchar_t*) realloc(m_pEnv, sizeof (wchar_t) * (m_len+len1+len2+2));
	wcscpy (&m_pEnv[end], var);
	m_pEnv[end + len1 ] = L'=';
	wcscpy (&m_pEnv[end+len1+1], value);
	m_pEnv[end+len1+len2+2] = L'\0';
	m_len += len1 + len2 + 2;
}

wchar_t *EnvironmentHandler::generate (HANDLE hToken, MSV1_0_INTERACTIVE_PROFILE* pProfile) {
	// ASSIGN homedrive
	if (pProfile->HomeDirectoryDrive.Length > 0)
	{
		addVariable(L"HOMEDRIVE", pProfile->HomeDirectoryDrive.Buffer);
		addVariable (L"HOMEPATH", L"\\");
		addVariable (L"HOMESHARE", pProfile->HomeDirectory.Buffer);
	} else if (pProfile->HomeDirectory.Length >= 3 && pProfile->HomeDirectory.Buffer[1] == L':') {
		wchar_t wchDrive[3];
		wchDrive[0] = pProfile->HomeDirectory.Buffer[0];
		wchDrive[1] = pProfile->HomeDirectory.Buffer[1];
		wchDrive[2] = L'\0';
		addVariable(L"HOMEDRIVE", wchDrive);
		addVariable (L"HOMEPATH", &pProfile->HomeDirectory.Buffer[2]);
	} else {
		wchar_t wchProfileDir[MAX_PATH] = L"";
		DWORD dwSize = sizeof wchProfileDir;
		GetUserProfileDirectoryW(hToken, wchProfileDir, &dwSize);
		if (wcslen(wchProfileDir) > 2 && wchProfileDir[1] == L':') {
			wchar_t wchDrive[3];
			wchDrive[0] = wchProfileDir[0];
			wchDrive[1] = wchProfileDir[1];
			wchDrive[2] = L'\0';
			addVariable(L"HOMEDRIVE", wchDrive);
			addVariable (L"HOMEPATH", &wchProfileDir[2]);
		}
	}
	// logonserver
	if (pProfile->LogonServer.Length > 0) {
		wchar_t ach[MAX_PATH];
		swprintf (ach, L"\\\\%s", pProfile->LogonServer.Buffer);
		addVariable (L"LOGONSERVER", ach);
	}
	// sessionname
	if (pProfile->LogonServer.Length > 0)
		addVariable (L"SESSIONNAME", L"Console");
	//
	/// ASSIGN APPDATA
	WCHAR AppDataPath[MAX_PATH];
	if(SHGetFolderPathW(NULL, CSIDL_APPDATA, hToken, SHGFP_TYPE_CURRENT, AppDataPath) != E_FAIL)
	{
		addVariable(L"APPDATA", AppDataPath);
	}

	printf ("************ RESULTING ENV ****************\n");
	wchar_t* p = m_pEnv;
	while (*p)
	{
		wprintf (L"[%s]\n", p);
		p += wcslen(p) + 1;
	}
	printf ("************ RESULTING ENV ****************\n");

	return m_pEnv;
}
