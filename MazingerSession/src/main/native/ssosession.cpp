#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <ctype.h>
#include <winuser.h>
#include <wtsapi32.h>
#include <winhttp.h>
#include <wincrypt.h>
#define SECURITY_WIN32
#include <security.h>

#else

#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>
#include <wchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#endif

#include <MZNcompat.h>
#include <stdio.h>
#include "ssosession.h"
#include "passwordbank.h"
#include "mazinger.h"
#include "ssodaemon.h"
#include "crypt.h"
#include "sessioncommon.h"
#include "httpHandler.h"
#include "appmenu.h"
#include <time.h>

#include "json/JsonAbstractObject.h"

SeyconSession::SeyconSession ()
{
	this->status = LOGIN_DENIED;
}

////////////////////////////////////////////////////////////
void SeyconSession::propagatePassword ()
{

	SeyconService service;
	// Convert user
	std::wstring wsuser = service.escapeString(user.c_str());
	// Convert password
	std::wstring wspass = service.escapeString(password.c_str());

	std::string domain;
	SeyconCommon::readProperty("SSOSoffidAgent", domain);
	std::wstring wsDomain = service.escapeString(domain.c_str());

	SeyconResponse *resp = service.sendUrlMessage(
			L"/propagatepass?user=%ls&password=%ls", wsuser.c_str(), wspass.c_str());

	SeyconCommon::wipe(wspass);

	if (resp != NULL)
		delete resp;
}

////////////////////////////////////////////////////////////
void SeyconSession::createWeakSession ()
{
	m_bOpen = true;
	m_daemon = new SsoDaemon();
	m_daemon->session = this;

	int port = m_daemon->startDaemon();
	SeyconService service;

	std::wstring wClientip;
	std::string clientIp;
	// Convert user
	std::wstring wsuser = service.escapeString(user.c_str());
	// Get client IP
#ifdef WIN32
	SeyconCommon::getCitrixClientIP(clientIp);
	wClientip = service.escapeString(clientIp.c_str());
#endif
	// Generar URL
	SeyconResponse *resp = service.sendUrlMessage(
			L"/createsession?user=%ls&clientIP=%ls&port=%d", wsuser.c_str(),
			wClientip.c_str(), port);
	if (resp != NULL)
	{
		std::string session;
		std::string status;
		resp->getToken(0, status);
		resp->getToken(1, session);

		if (status == "OK")
		{
			resp->getToken(1, m_daemon->sessionId);
		}
		else
		{
			m_daemon->sessionId.clear();
		}
		delete resp;
	}
}

//////////////////////////////////////////////////////////
void SeyconSession::weakSessionStartup (const char* lpszUser, const wchar_t* lpszPass)
{
	int i;

	m_passwordLogin = false;
	m_kerberosLogin = false;

	SeyconCommon::updateHostAddress();

	SeyconService service;

	user.clear();
	for (i = 0; lpszUser[i]; i++)
		user.append(1, tolower(lpszUser[i]));

	password.assign(lpszPass);

	if (!SeyconCommon::bNormalized())
	{
		updateConfiguration();
		// Propagar password
		propagatePassword();
		createWeakSession();
	}
	SeyconCommon::wipe(password);
}

#ifdef WIN32
static void transformCanonicalToPrincipal (wchar_t* wchUserName)
{
	wchar_t wchBuffer[512];
	wcscpy(wchBuffer, wchUserName);
	int first = -1;
	int last = -1;
	for (int i = 0; wchBuffer[i]; i++)
	{
		if (wchBuffer[i] == '/')
		{
			if (first == -1)
				first = i;
			last = i;
		}
	}
	wcscpy(wchUserName, &wchBuffer[last + 1]);
	wcscat(wchUserName, L"@");
	wcsncat(wchUserName, wchBuffer, first);
}

/*
 * Intenta realizar el login kerberos
 *
 * 0 => Error de conexión. Se intentará con el siguiente servidor
 * 1 => Login correcto
 * 2 => Login denegado
 */
ServiceIteratorResult SeyconSession::kerberosLogin (const char* hostName, size_t dwPort)
{
	TimeStamp Lifetime;
	SecBufferDesc OutBuffDesc;
	SecBuffer OutSecBuff;
	SecBufferDesc InBuffDesc;
	SecBuffer InSecBuff;
	ULONG ContextAttributes;

	const int MAX_MESSAGE = 32000;
	char outBuffer[MAX_MESSAGE];

	SeyconCommon::info("Trying kerberos login on %s:%d\n", hostName, dwPort);

	SECURITY_STATUS ss;
	char achProvider[] = "Kerberos";
	wchar_t wchUserName[256];
	char achUserName[256];
	std::string principalName;
	CredHandle hCredential;
	DWORD dw = 256;
	DWORD dw2 = 256;
	SecHandle ctxHandle;
	BOOL firstTime = TRUE;
	std::string challengeId;

	SeyconService service;

	user.assign(MZNC_getUserName());
	if (GetUserNameExW(NameUserPrincipal, wchUserName, &dw)
			&& GetUserNameExA(NameUserPrincipal, achUserName, &dw2))
	{
		principalName.assign(achUserName);
	}
	else
	{
		SeyconCommon::warn("Unable to get username principal\n");
		dw = 256;
		if (!GetUserNameExW(NameCanonical, wchUserName, &dw))
		{
			SeyconCommon::warn("Unable to get canonical username\n");
			SeyconCommon::notifyError();
			errorMessage = "Info: Local users can not access the network";
			status = LOGIN_ERROR;
			return SIR_ERROR;
		}
		SeyconCommon::info("fqdn=%ls\n", wchUserName);
		transformCanonicalToPrincipal(wchUserName);
		principalName = MZNC_wstrtostr(wchUserName);
	}
	SeyconCommon::info("principal=%s\n", principalName.c_str());

	std::string remotePrincipal = "SEYCON/";
	remotePrincipal.append(hostName);
	SeyconCommon::info("Remote principal: [%s]\n", remotePrincipal.c_str());

	DWORD dwResult = AcquireCredentialsHandle((SEC_CHAR*) principalName.c_str(),
			achProvider, SECPKG_CRED_OUTBOUND, NULL, NULL, NULL, NULL, &hCredential,
			NULL);
	if (dwResult < 0)
	{
		SeyconCommon::info("Error acquiring credential\n");
		SeyconCommon::notifyError();
		errorMessage = "Can not get the Kerberos credential";
		status = LOGIN_ERROR;
		return SIR_ERROR;
	}

	//--------------------------------------------------------------------
	//  Prepare the buffers.

	OutBuffDesc.ulVersion = 0;
	OutBuffDesc.cBuffers = 1;
	OutBuffDesc.pBuffers = &OutSecBuff;

	OutSecBuff.BufferType = SECBUFFER_TOKEN;

	InBuffDesc.ulVersion = 0;
	InBuffDesc.cBuffers = 1;
	InBuffDesc.pBuffers = &InSecBuff;

	InSecBuff.cbBuffer = 1;
	InSecBuff.BufferType = SECBUFFER_TOKEN;

	BOOL end = FALSE;
	do
	{
		OutSecBuff.cbBuffer = MAX_MESSAGE;
		OutSecBuff.pvBuffer = outBuffer;
		boolean retry = false;

		time_t start;
		time_t now;

		time(&start);
		do
		{

			ss = InitializeSecurityContext(&hCredential, firstTime ? NULL : &ctxHandle,
					(SEC_CHAR*) remotePrincipal.c_str(), // achRemotePrincipal
					ISC_REQ_MUTUAL_AUTH | ISC_REQ_INTEGRITY, // Message attributes
					0, // Reserved
					SECURITY_NETWORK_DREP, //Network data representation
					firstTime ? NULL : &InBuffDesc, // Data received from seycon server
					0, &ctxHandle, // Receives communication handler
					&OutBuffDesc, &ContextAttributes, &Lifetime);
			//-------------------------------------------------------------------
			//  If necessary, complete the token.

			if (ss < 0)
			{
				const char *msg = "??";
				if (ss == SEC_E_INSUFFICIENT_MEMORY )
					msg = ("  Insufficiente memory\n");
				else if (ss == SEC_E_INTERNAL_ERROR )
					msg = ("  Internal error\n");
				else if (ss == SEC_E_INVALID_HANDLE )
					msg = ("  Invalid handle\n");
				else if (ss == SEC_E_INVALID_TOKEN )
					msg = ("  Invalid token\n");
				else if (ss == SEC_E_LOGON_DENIED )
					msg = ("  Logon denied\n");
				else if (ss == SEC_E_NO_AUTHENTICATING_AUTHORITY )
					msg = ("  No authenticating authority\n");
				else if (ss == SEC_E_NO_CREDENTIALS )
					msg = ("  No credentials available\n");
				else if (ss == SEC_E_TARGET_UNKNOWN )
					msg = ("  Target unknown\n");
				else if (ss == SEC_E_UNSUPPORTED_FUNCTION )
					msg = ("  Unsupported function\n");
				else if (ss == SEC_E_WRONG_PRINCIPAL )
					msg = ("  Wrong principal\n");

				SeyconCommon::warn("Error generating token Kerberos: %d: %s", ss, msg);

				time(&now);
				if ( ss == SEC_E_NO_AUTHENTICATING_AUTHORITY && (now - start < 60))
				{
					Sleep(1000);
					retry = true;
				}
				else
				{
					errorMessage = "Identity server does not accect the Kerberos credential: ";
					errorMessage += msg;
					return SIR_RETRY;
				}
			}
			else
				retry = false;

		} while (retry);

		if (ss == SEC_E_OK )
		{
			SeyconCommon::info("Login successful\n");
			SecPkgContext_Names name;
			if (SEC_E_OK == QueryContextAttributes(&ctxHandle, SECPKG_ATTR_NAMES, &name))
			{
				SeyconCommon::debug("Remote principal = %s\n", name.sUserName);
				FreeContextBuffer(name.sUserName);
			}
			else
			{
				SeyconCommon::notifyError();
			}
			break;
		}
		if ((SEC_I_COMPLETE_NEEDED == ss) || (SEC_I_COMPLETE_AND_CONTINUE == ss))
		{
			ss = CompleteAuthToken(&ctxHandle, &OutBuffDesc);
			if (ss >= 0)
			{
				end = TRUE;
			}
			else
			{
				end = TRUE;
			}
		}

		// Convertir token a base64
		std::wstring msg64 = service.escapeString(
				SeyconCommon::toBase64((unsigned char*) OutSecBuff.pvBuffer,
						OutSecBuff.cbBuffer).c_str());
		std::string clientIP;
		SeyconCommon::getCitrixClientIP(clientIP);
		std::wstring wclientIP = service.escapeString(clientIP.c_str());
		//			wprintf (L"Base64escapada=%s\n", wchBase64Message);
		SeyconResponse *response;
		if (firstTime)
		{
			response =
					service.sendUrlMessage(hostName, dwPort,
							L"/kerberosLogin?action=start&principal=%ls&clientIP=%ls&cardSupport=%d&krbToken=%ls",
							wchUserName, wclientIP.c_str(),
							SeyconCommon::getCardSupport(), msg64.c_str());
		}
		else
		{
			response = service.sendUrlMessage(hostName, dwPort,
					L"/kerberosLogin?action=continue&challengeId=%hs&krbToken=%ls",
					challengeId.c_str(), msg64.c_str());
		}
		if (response == NULL)
		{
			SeyconCommon::warn("KerberosAuthentication: Error connecting to server %s",
					hostName);
			errorMessage = "Network error";
			return SIR_RETRY;
		}
		std::string status = response->getToken(0);
		if (status == "OK")
		{
			SeyconCommon::info("Authentication completed\n");
			sessionKey = response->getToken(1).c_str();
			std::string msg64 = SeyconCommon::fromBase64(response->getToken(2).c_str());
			cardNumber = response->getToken(3);
			cardCell = response->getToken(4);
			soffidUser = response->getToken(5);
			if (soffidUser.empty())
				soffidUser = user;

			InSecBuff.cbBuffer = msg64.size();
			InSecBuff.pvBuffer = (LPBYTE) malloc(msg64.size());
			memcpy(InSecBuff.pvBuffer, msg64.c_str(), msg64.size());
		}
		else if (status == "MoreDataReq")
		{
			SeyconCommon::debug("Authentication in progress\n");
			challengeId = response->getToken(1);
			std::string msg64 = SeyconCommon::fromBase64(response->getToken(2).c_str());

			InSecBuff.cbBuffer = msg64.size();
			InSecBuff.pvBuffer = (LPBYTE) malloc(msg64.size());
			memcpy(InSecBuff.pvBuffer, msg64.c_str(), msg64.size());
		}
		else if (status == "es.caib.sso.LogonDeniedException" ||
				status == "es.caib.seycon.ng.exception.LogonDeniedException")
		{
			// Login denegado
			std::string msg = response->getToken(1);
			SeyconCommon::warn("KerberosAuthentication: LoginDenied: %s", msg.c_str());
			MessageBoxA(NULL, msg.c_str(), "Unable to start session", MB_OK | MB_ICONERROR);
			errorMessage = "Unauthorized logon: ";
			errorMessage += msg;
			status = LOGIN_DENIED;
			return SIR_ERROR;
		}
		else
		{
			// Login error
			std::string msg = response->getToken(1);
			SeyconCommon::warn("KerberosAuthentication: ERROR [%s]\n", status.c_str());
			errorMessage = "Authentication error: ";
//			errorMessage.append(status.c_str());
			errorMessage.append(msg);
			status = LOGIN_DENIED;
			return SIR_ERROR;
		}
		delete response;
		firstTime = false;
	} while (true);

	return SIR_SUCCESS;
}

ServiceIteratorResult SeyconSession::iterateKerberos (const char* hostName,
		size_t dwPort)
{
	ServiceIteratorResult sir = kerberosLogin(hostName, dwPort);
	if (sir == SIR_SUCCESS)
	{
		status = LOGIN_SUCCESS;
		sir = createKerberosSession(hostName, dwPort);
		if (sir == SIR_SUCCESS)
		{
			int bEnablePB = SeyconCommon::readIntProperty("EnablePB");
			if (bEnablePB && isPasswordBankInstalled())
			{
				if (launchPasswordBank(hostName, dwPort) != SIR_SUCCESS)
					launchMazinger(hostName, dwPort, "kerberosLogin");
			}
			else
			{
				launchMazinger(hostName, dwPort, "kerberosLogin");
			}

		}
	}
	return sir;
}

ServiceIteratorResult KerberosIterator::iterate (const char* hostName, size_t dwPort)
{
	return s->iterateKerberos(hostName, dwPort);
}

#else
ServiceIteratorResult KerberosIterator::iterate (const char* hostName, size_t dwPort)
{
	return SIR_ERROR;
}
#endif

KerberosIterator::~KerberosIterator ()
{
}


//////////////////////////////////////////////////////////
int SeyconSession::kerberosSessionStartup ()
{
	m_kerberosLogin = true;
	m_passwordLogin = false;

	updateConfiguration();

	KerberosIterator it(this);
	SeyconService service;
	status = LOGIN_ERROR;

	// Realitzar el login
	ServiceIteratorResult result = service.iterateServers(it);

	return status;
}

//////////////////////////////////////////////////////////
int SeyconSession::restartSession ()
{
	if (m_kerberosLogin)
		return kerberosSessionStartup();
	else if (m_passwordLogin)
		return passwordSessionStartup(user.c_str(), password.c_str());
	else
		return LOGIN_ERROR;
}

void SeyconSession::close ()
{

	m_bOpen = false;
	SeyconService service;

	if (sessionId.size() > 0)
	{
		SeyconResponse *resp = service.sendUrlMessage(L"/logout?sessionId=%hs",
				sessionId.c_str());
		if (resp != NULL)
			free(resp);

		if (m_daemon != NULL)
		{
			m_daemon -> stopDaemon();
			m_daemon = NULL;
		}
	}
}

std::wstring SeyconSession::getCardValue ()
{
	std::wstring cardValue;
	if (m_dialog == NULL)
	{
		SeyconCommon::warn("Error: Dialog method disabled\n");
	}
	else
	{
		if (!m_dialog->askCard(cardNumber.c_str(), cardCell.c_str(), cardValue))
		{
			cardValue.clear();
		}
	}
	return cardValue;
}

void SeyconSession::notify (const char *message)
{
	if (m_dialog != NULL)
		m_dialog->notify(message);
	else
		printf("\n\n\n!!! Message from SEYCON SERVER !!!\n%s\n\n\n", message);
}

void SeyconSession::remoteClose (const char *key)
{
	if (sessionKey == key && m_dialog != NULL)
	{
		m_dialog->askAllowRemoteLogout();
	}
}

SeyconDialog::~SeyconDialog ()
{

}
