/*
 * LoggedOutManager.cpp
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "winwlx.h"
#include <windows.h>
#include <security.h>

#include "logindialog.h"
#include "LoggedOutManager.h"
#include "lm.h"
#include "Utils.h"
#include "time.h"

#include "SecurityHelper.h"
#include "ChangePasswordManager.h"
#include "EnvironmentHandler.h"

#include <MZNcompat.h>

#include "NotificationHandler.h"
#include "TokenHandler.h"
#include "Pkcs11Handler.h"
#include "CertificateHandler.h"
#include "CardNotificationHandler.h"

#include "LocalAdminHandler.h"

LoggedOutManager::LoggedOutManager():
	m_log ("LoggedOutManager")
{

}

LoggedOutManager::~LoggedOutManager() {
	if (p11Config != NULL)
		p11Config->setNotificationHandler(NULL);
}

static void _duplicateString (LPWSTR &pointer, std::wstring &value) {
	pointer = (LPWSTR) malloc ( (1+value.size()) * sizeof (wchar_t));
	wcscpy (pointer, value.c_str());
}

bool LoggedOutManager::chanceToChangePassword() {
	SYSTEMTIME st;
	FileTimeToSystemTime((const FILETIME*) & pInteractiveProfile->PasswordLastSet, &st);
	m_log.info ("PasswordLastSet = %d-%d-%d %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	FileTimeToSystemTime((const FILETIME*) & pInteractiveProfile->PasswordCanChange, &st);
	m_log.info ("PasswordCanChange = %d-%d-%d %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	FileTimeToSystemTime((const FILETIME*) & pInteractiveProfile->PasswordMustChange, &st);
	m_log.info ("PasswordMustChange = %d-%d-%d %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	LONGLONG v = pInteractiveProfile->PasswordMustChange.QuadPart;
	LONGLONG oneDay = 10000000; // 1 second
	oneDay *= 60; // 1 minute
	oneDay *= 60; // 1 hour
	oneDay *= 24; // 1 day
	v -= oneDay * 5;
	FileTimeToSystemTime((const FILETIME*) & v, &st);
	m_log.info ("Change Time = %d-%d-%d %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);


	GetSystemTime(&st);
	m_log.info ("System Time = %d-%d-%d %d:%d:%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER li;
	li.HighPart = ft.dwHighDateTime;
	li.LowPart = ft.dwLowDateTime;
	if (v < li.QuadPart) {
		long dataExpiracio = pInteractiveProfile->PasswordMustChange.QuadPart / oneDay;
		long today = li.QuadPart / oneDay;
		int dies = dataExpiracio - today;
		m_log.info ("%d days left to expire password", dies);
		std::wstring msg;
		if (dies == 0)
			msg = MZNC_strtowstr(Utils::LoadResourcesString(32).c_str()).c_str();
		else if (dies == 1)
			msg = MZNC_strtowstr(Utils::LoadResourcesString(33).c_str()).c_str();
		else {
			wchar_t wchDies[5];
			swprintf (wchDies, L"%d", dies);
			msg = MZNC_strtowstr(Utils::LoadResourcesString(34).c_str()).c_str();
			msg += wchDies;
			msg += MZNC_strtowstr(Utils::LoadResourcesString(35).c_str()).c_str();
		}
		msg += MZNC_strtowstr(Utils::LoadResourcesString(36).c_str()).c_str();

		int result = pWinLogon->WlxMessageBox(hWlx, NULL, msg.c_str(),
				MZNC_strtowstr(Utils::LoadResourcesString(1000).c_str()).c_str(),
				MB_YESNO|MB_ICONINFORMATION);
		if (result == IDYES) {
			ChangePasswordManager cpm;
			cpm.changePassword(szUser, szDomain, szPassword);
		}
	}

	return false;
}

bool LoggedOutManager::doLogin ()
{
	DWORD win32Error;
	pInteractiveProfile = 0;
	boolean repeat = false;
	wchar_t *profilePath;

	std::wstring oldPassword;
	oldPassword.assign (szPassword);

	displayMessage(NULL, MZNC_strtowstr(Utils::LoadResourcesString(37).c_str()).c_str());
	LoginStatus::current.szOriginalUser = szUser;

	do
	{
		repeat = false;
		m_log.info (L"Trying login %s@%s:******", szUser.c_str(), szDomain.c_str());

		// attempt the login
		if (SecurityHelper::CallLsaLogonUser(hLsa, szDomain.c_str(), szUser.c_str(),
				szPassword.c_str(), Interactive, pAuthenticationId, phToken,
				&pInteractiveProfile, &win32Error))
		{
			repeat = chanceToChangePassword();
		}
		else
		{
			m_log.info ("lsa failed");
			if ((win32Error == ERROR_PASSWORD_MUST_CHANGE) ||
				(win32Error == ERROR_PASSWORD_EXPIRED))
			{
				ChangePasswordManager cpm;
				if (!cpm.changePassword(szUser, szDomain, szPassword))
					return false;
				else
					repeat = true;
			}
			else
			{
				if (szDomain == szLocalHostName)
				{
					m_log.info("Trying server supplied credentials");
					// Intentar recuperar user i password del servidor
					LocalAdminHandler handler;
					if (handler.getAdminCredentials(szUser, szPassword, szDomain))
					{
						szUser.assign (handler.szUser);
						szPassword.assign(handler.szPassword);
						repeat = true;
					}

					else
						return false;
				}
				if (!repeat)
				{
					std::wstring msg;
					Utils::getErrorMessage(msg, win32Error);
					m_log.warn (L"Cannot start session: %s", msg.c_str());
					pWinLogon->WlxMessageBox(hWlx, NULL, msg.c_str(),
							MZNC_strtowstr(Utils::LoadResourcesString(38).c_str()).c_str(),
							MB_OK|MB_ICONEXCLAMATION);
					return false;
				}
			}
		}
	} while (repeat);

	m_log.info ("logon done");

	recordLogonTime();

	if (!SecurityHelper::ExtractProfilePath(&profilePath, pInteractiveProfile))
	{
		std::wstring msg;

		Utils::getLastError(msg);
		m_log.warn (L"Cannot extract profile path %s", msg.c_str());

		pWinLogon->WlxMessageBox(hWlx, NULL,
				msg.c_str(), MZNC_strtowstr(Utils::LoadResourcesString(38).c_str()).c_str(),
				MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	m_log.info ("profile got");

	// Assume that WinLogon provides a buffer large enough to hold a logon SID,
	// which is of fixed length. It'd be nice if WinLogon would tell us how big
	// its buffer actually was, but it appears this is assumed
	bool success = false;
	if (SecurityHelper::GetLogonSid(*phToken, pLogonSid))
	{
		WLX_PROFILE_V2_0 *pGinaProfile;

		m_log.info (L"Got logon sid en profile path [%s]", profilePath);
		if (SecurityHelper::AllocWinLogonProfile(&pGinaProfile, profilePath))
		{
			m_log.info ("Creating profile");
			*pProfile = pGinaProfile;
			EnvironmentHandler h;
			pGinaProfile->pszEnvironment = h.generate(*phToken, pInteractiveProfile);
			m_log.info ("Created profile");
			 // copy login information for network providers
			_duplicateString(pMprNotifyInfo->pszUserName, szUser);
			 _duplicateString(pMprNotifyInfo->pszDomain, szDomain);
			 _duplicateString(pMprNotifyInfo->pszPassword, szPassword);
			 if (szPassword == oldPassword)
				 _duplicateString (pMprNotifyInfo	->pszOldPassword, oldPassword);
			 else
				 pMprNotifyInfo->pszOldPassword = NULL;
			 m_log.info ("XXX");

			 if (pMprNotifyInfo->pszUserName && pMprNotifyInfo->pszDomain &&
				 pMprNotifyInfo->pszPassword)
			 {
				 success = true;
			 }
		 }
	}
	else
	{
		std::wstring msg;
		Utils::getLastError(msg);

		m_log.info(L"Cannot get logon sid: %s", msg.c_str());
		pWinLogon->WlxMessageBox(hWlx, NULL, msg.c_str(),
			MZNC_strtowstr(Utils::LoadResourcesString(38).c_str()).c_str(),
			MB_OK|MB_ICONEXCLAMATION);
	}

	m_log.info ("logon sid got");

	if (!success)
	{
		CloseHandle(*phToken);
		*phToken = 0;
	}

	m_log.info("User = %ls", LoginStatus::current.szUser.c_str());
	m_log.info("Original User = %ls", LoginStatus::current.szOriginalUser.c_str());
	m_log.info("Login done");

	cancelMessage();

	return success;
}

int LoggedOutManager::process()
{
	if (p11Config != NULL) {
		std::vector<TokenHandler*> tokens = p11Config->enumTokens();
		m_log.info ("Detected %d tokens", tokens.size());
		if (tokens.size () > 0 )
			return processCard ();
	}
	CardNotificationHandler notifier (&loginDialog.m_hWnd);

	SecurityHelper::getDomain(loginDialog.domainName);

	wchar_t wchComputerName[MAX_COMPUTERNAME_LENGTH + 1 ];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerNameW(wchComputerName, &dwSize)) {
		szLocalHostName.assign (wchComputerName);
		loginDialog.hostName.assign(wchComputerName);
	}
	m_log.info (L"Domain = %ls", loginDialog.domainName.c_str());
	m_log.info (L"Host = %ls", loginDialog.hostName.c_str());
	if (p11Config != NULL)
		p11Config->setNotificationHandler(&notifier);
	int result = loginDialog.show();
	if (p11Config != NULL)
		p11Config->setNotificationHandler(NULL);
	if ( result == WLX_SAS_ACTION_LOGON) {
		szPassword = loginDialog.password;
		szUser = loginDialog.user;
		szDomain = loginDialog.domain;
		if ( !doLogin() )
			result = WLX_SAS_ACTION_NONE;
	}
	else if (result == WLX_SAS_ACTION_NONE)
	{
		if (notifier.isCardInserted()) {
			result = processCard ();
		}
	} else if (result != WLX_SAS_ACTION_SHUTDOWN &&
			result != WLX_SAS_ACTION_SHUTDOWN_HIBERNATE &&
			result != WLX_SAS_ACTION_SHUTDOWN_REBOOT &&
			result != WLX_SAS_ACTION_SHUTDOWN_SLEEP)
	{
		result = WLX_SAS_ACTION_NONE;
	}

	return result;
}

int LoggedOutManager::processCard () {
	int result = WLX_SAS_ACTION_NONE;
	if (p11Config == NULL)
		return result;
	std::vector<TokenHandler*> tokens = p11Config->enumTokens();
	if (tokens.size () > 0 )
	{
		LPCSTR szPin = enterPin();
		if (szPin != NULL)
		{
			displayMessage(NULL,
					MZNC_strtowstr(Utils::LoadResourcesString(29).c_str()).c_str());
			std::vector<CertificateHandler*> certs;
			for (std::vector<TokenHandler*>::iterator it = tokens.begin(); it != tokens.end(); it++)
			{
				TokenHandler *token = *it;
				token->setPin(szPin);
				std::vector<CertificateHandler*> &tokenCerts = token->getHandler()->enumCertificates(token);
				for (std::vector<CertificateHandler*>::iterator itCert = tokenCerts.begin();
						itCert != tokenCerts.end(); itCert++){
					CertificateHandler *cert = *itCert;
					certs.push_back(cert);
				}
			}

			if (certs.size() == 0)
			{

				pWinLogon->WlxMessageBox(hWlx, NULL,
						MZNC_strtowstr(Utils::LoadResourcesString(30).c_str()).c_str(),
						MZNC_strtowstr(Utils::LoadResourcesString(31).c_str()).c_str(),
						MB_OK|MB_ICONEXCLAMATION);
			} else {
				int i = certs.size() == 1 ? 0 : selectCert(certs);

				if (i >= 0) {
					CertificateHandler *pCert = certs[i];
					if (pCert->obtainCredentials() ) {
						szUser = pCert->getUser();
						szPassword = pCert->getPassword();
						szDomain = pCert->getDomain();
						if (doLogin()) {
							pCert->getHandler()->saveCredentials(pCert, szUser, szPassword);
							result = WLX_SAS_ACTION_LOGON;
						}
					} else {
						MessageBox (NULL, pCert->getErrorMessage(),
								Utils::LoadResourcesString(31).c_str(),
								MB_OK|MB_ICONEXCLAMATION);
					}
				}
			}
			cancelMessage();
		}
	}
	return result;
}

void LoggedOutManager::recordLogonTime () {
	wchar_t *achBuffer = (wchar_t*) malloc (sizeof (wchar_t) * 50);

	time_t t;
	time(&t);

	struct tm * now = localtime(&t);

	swprintf (achBuffer, L"%d/%d/%d %d:%02d:%02d",
			now->tm_mday, 1+now->tm_mon, 1900+now->tm_year,
			now->tm_hour, now->tm_min, now->tm_sec);
	LoginStatus::current.szLogonTime.assign (achBuffer);
	LoginStatus::current.szUser = szUser;
	LoginStatus::current.szDomain = szDomain;
	LoginStatus::current.hToken = *phToken;
	free (achBuffer);
}
