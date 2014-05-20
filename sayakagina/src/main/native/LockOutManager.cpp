/*
 * LockOutManager.cpp
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "winwlx.h"
#include <windows.h>
#include <security.h>

#include "logindialog.h"
#include "LockOutManager.h"
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
#include "LoginStatus.h"

#include "LocalAdminHandler.h"

LockOutManager::LockOutManager():
	m_log("LockOutManager") {
}

LockOutManager::~LockOutManager()
{
	if (p11Config != NULL)
		p11Config->setNotificationHandler(NULL);
}

bool LockOutManager::doLogin ()
{
	bool success = false;
	HANDLE hNewuser;
	bool repeat;
	do
	{
		repeat = false;

		if (LogonUserW ((wchar_t*) szUser.c_str(), (wchar_t*) szDomain.c_str(),
				(wchar_t*) szPassword.c_str(), LOGON32_LOGON_UNLOCK,
				LOGON32_PROVIDER_DEFAULT, &hNewuser))
		{
			m_bSameUser = false;
			if (SecurityHelper::IsSameUser(hNewuser,
					LoginStatus::current.hToken, &m_bSameUser) &&
				m_bSameUser )
			{
				success = true;
			} else if (SecurityHelper::IsAdmin(hNewuser))
			{
				success = true;
			}
			else
			{
				std::wstring s = MZNC_strtowstr(Utils::LoadResourcesString(24).c_str()).c_str();
				s.append (LoginStatus::current.szOriginalUser.c_str());
				s.append (MZNC_strtowstr(Utils::LoadResourcesString(25).c_str()).c_str());
				pWinLogon->WlxMessageBox(hWlx, NULL, s.c_str(),
					MZNC_strtowstr(Utils::LoadResourcesString(26).c_str()).c_str(),
					MB_OK|MB_ICONEXCLAMATION);
			}
			CloseHandle (hNewuser);
		}
		else
		{
			DWORD lastError = GetLastError ();
			if ((lastError == ERROR_PASSWORD_MUST_CHANGE) ||
				(lastError == ERROR_PASSWORD_EXPIRED))
			{
				pWinLogon->WlxMessageBox(hWlx, NULL,
					MZNC_strtowstr(Utils::LoadResourcesString(27).c_str()).c_str(),
					MZNC_strtowstr(Utils::LoadResourcesString(28).c_str()).c_str(),
					MB_OK|MB_ICONEXCLAMATION);
				ChangePasswordManager cpm;
				if (cpm.changePassword(szUser, szDomain, szPassword))
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
					Utils::getErrorMessage(msg, GetLastError());
					m_log.warn (L"Cannot unblock the machine: %s", msg.c_str());
					pWinLogon->WlxMessageBox(hWlx, NULL, msg.c_str(),
						MZNC_strtowstr(Utils::LoadResourcesString(26).c_str()).c_str(),
						MB_OK|MB_ICONEXCLAMATION);
					return false;
				}
			}
		}
	} while (repeat);

 	return success;
}

int LockOutManager::process()
{
	CardNotificationHandler notifier (&loginDialog.m_hWnd);

	SecurityHelper::getDomain (loginDialog.domainName);

	wchar_t wchComputerName[MAX_COMPUTERNAME_LENGTH + 1 ];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerNameW(wchComputerName, &dwSize)) {
		loginDialog.hostName.assign(wchComputerName);
		szLocalHostName.assign (wchComputerName);
	}
	m_log.info (L"Domain = %ls", loginDialog.domainName.c_str());
	m_log.info (L"Host = %ls", loginDialog.hostName.c_str());
	loginDialog.user = LoginStatus::current.szOriginalUser;
	loginDialog.domain = LoginStatus::current.szDomain;

	int result = processCard ();
	while (result == 0) {
		p11Config->setNotificationHandler(&notifier);
		result = loginDialog.show ();
		m_log.info ("LoginDialog: return code %d", result);
		if (p11Config != NULL)
			p11Config->setNotificationHandler(NULL);
		if ( result == WLX_SAS_ACTION_LOGON) {
			szPassword = loginDialog.password;
			szUser = loginDialog.user;
			szDomain = loginDialog.domain;
			if ( doLogin() )
			{
				if (m_bSameUser)
					return WLX_SAS_ACTION_UNLOCK_WKSTA;
				else {
					m_log.info("Forcing logoff");
					return WLX_SAS_ACTION_FORCE_LOGOFF;
				}
			}
			else
				return WLX_SAS_ACTION_NONE;
		}
		else if (result == WLX_SAS_ACTION_NONE)
		{
			if (notifier.isCardInserted() ) {
				result = processCard ();
			}
		} else if (result != WLX_SAS_ACTION_SHUTDOWN &&
				result != WLX_SAS_ACTION_SHUTDOWN_HIBERNATE &&
				result != WLX_SAS_ACTION_SHUTDOWN_REBOOT &&
				result != WLX_SAS_ACTION_SHUTDOWN_SLEEP
				)
		{
			result = WLX_SAS_ACTION_NONE;
		}
	}
	return result;
}

int LockOutManager::processCard () {
	int result = 0;
	std::vector<TokenHandler*> tokens;
	if (p11Config != NULL)
		tokens = p11Config->enumTokens();
	if (tokens.size () > 0 )
	{
		LPCSTR szPin = enterPin();
		if (szPin != NULL)
		{
			result = WLX_SAS_ACTION_NONE;
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
							result = WLX_SAS_ACTION_UNLOCK_WKSTA;
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

