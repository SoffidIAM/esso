/*
 * CertificateHandler.cpp
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#include "../cert/CertificateHandler.h"

#include "sayaka.h"
#include "Log.h"
#include "Utils.h"
#include <ssoclient.h>
#include <MZNcompat.h>
#include "../cert/Pkcs11Handler.h"
#include "lm.h"


CertificateHandler::CertificateHandler(Pkcs11Handler *handler) {
	m_handler = handler;
	m_certContext = NULL;
	m_achId = NULL;
	m_pvCert = NULL;
	szUser = NULL;
	szPassword = NULL;
	szDomain = NULL;
}

CertificateHandler::~CertificateHandler() {
	if (m_achId != NULL)
		free(m_achId);
	if (m_pvCert != NULL)
		free(m_pvCert);
	if (m_certContext != NULL)
		CertFreeCertificateContext(m_certContext);
	Utils::freeString(szUser);
	Utils::freeString(szPassword);
	Utils::freeString(szDomain);
}


void CertificateHandler::setPassword (const wchar_t* szPassword) {
	Utils::freeString(this->szPassword);
	Utils::duplicateString(this->szPassword, szPassword);
}

PCCERT_CONTEXT CertificateHandler::getCertContext() {
	if (m_certContext == NULL) {
		m_certContext = (PCCERT_CONTEXT) CertCreateContext(
				CERT_STORE_CERTIFICATE_CONTEXT, X509_ASN_ENCODING, m_pvCert,
				m_ulCertLength, 0, NULL );
	}
	return m_certContext;

}

bool CertificateHandler::getNearExpireDate(){
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER li;

	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	__int64 l = 30; // 30 days
	l *= 24; // 24 hours a day
	l *= 60; // 60 minutes an hour
	l *= 60; // 60 seconds a minute
	l *= 1000000L; // 1.000.000 (100 nanoseconds) a asecond
	li.QuadPart += l;

	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;

	if (CertVerifyTimeValidity(&ft, getCertContext()->pCertInfo) != 0) {
		// Not valid in a month
		return true;
	} else {
		return false;
	}
}

void CertificateHandler::parseCert() {

	if (getCertContext() != NULL) {
		if (CertGetNameString(getCertContext(),
				CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, m_achName,
				sizeof m_achName) < 2) {
			Log log("CertificateHandler::parseCert");
			log.warn ("Error during CertNameToStr 0x%x\n",
					(int) GetLastError());
		}

		if (CertGetNameString(getCertContext(),
				CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG,
				NULL, m_achIssuer, sizeof m_achIssuer) < 2) {
			Log log("CertificateHandler::parseCert");
			log.warn ("Error during CertNameToStr 0x%x\n",
					(int) GetLastError());
		}
	}
}

#if ! defined(WIN64) && defined(__GNUC__) && false
typedef enum _NETSETUP_JOIN_STATUS {

    NetSetupUnknownStatus = 0,
    NetSetupUnjoined,
    NetSetupWorkgroupName,
    NetSetupDomainName
} NETSETUP_JOIN_STATUS, *PNETSETUP_JOIN_STATUS;
extern "C" DWORD __stdcall NetGetJoinInformation(
    LPCWSTR                lpServer,
    LPWSTR                *lpNameBuffer,
    PNETSETUP_JOIN_STATUS  BufferType);

#endif




void CertificateHandler::obtainDomain () {
	LPWSTR buffer = NULL;
	NETSETUP_JOIN_STATUS dwJoinStatus = NetSetupUnknownStatus;
	NetGetJoinInformation (NULL, &buffer, &dwJoinStatus);
	if (dwJoinStatus == NetSetupDomainName)
	{
		Utils::duplicateString(this->szDomain, buffer);
	}
	if (buffer != NULL)
		NetApiBufferFree (buffer);

}


bool CertificateHandler::obtainCredentials() {
	SeyconService service;

	service.resetServerStatus();


	SeyconResponse *response = service.sendUrlMessage( L"/certificateLogin?action=start");
	unsigned char * pData;
	unsigned long dwSize;

	Utils::freeString(this->szUser);
	Utils::freeString(this->szPassword);
	Utils::freeString(this->szDomain);

	obtainDomain();

	if (response == NULL) {
		// Buscar en el cache

		if (m_handler->getSavedCredentials(this, szUser, szPassword)) {
			obtainDomain();
			return true;
		} else {
			m_errorMessage.assign(Utils::LoadResourcesString(3));
			return false;
		}
	} else {
		std::string result = response->getToken(0);
		if ( result == "OK") {
			std::string sessionid;
			std::wstring wsessionid;

			response->getToken(1, sessionid);
			response->getToken(1, wsessionid);

			if (! m_handler->sign(this, sessionid.c_str(), &pData, &dwSize)) {
				m_errorMessage.assign(Utils::LoadResourcesString(4));
				delete response;
				return false;
			}

			// Transformar base 64 la firma
			std::string msg64 = SeyconCommon::toBase64(pData, dwSize);
			// Transformar wide char
			std::wstring wmsg64 = service.escapeString(msg64.c_str());

			// Transformar base 64 el certificado
			std::string cert64 = SeyconCommon::toBase64(m_pvCert, m_ulCertLength);
			std::wstring wcert64 = service.escapeString(cert64.c_str());
			delete response;

			response = service.sendUrlMessage(L"/certificateLogin?action=continue&challengeId=%ls&cert=%ls&signature=%ls",
					wsessionid.c_str(), wcert64.c_str(), wmsg64.c_str());
			if (response == NULL)
			{
				m_errorMessage.assign (Utils::LoadResourcesString(5));
				return false;
			}
			bool ok = parseResponse(response);
			delete response;
			return ok;
		} else {
			std::string msg = response->getToken(1);

			m_errorMessage.assign (Utils::LoadResourcesString(6));
			m_errorMessage.append(msg.c_str());
			delete response;
			return false;
		}
	}

	return false;

}

bool CertificateHandler::parseResponse (SeyconResponse *response) {
	if (response == NULL)
	{
		return false;
	}
	std::string status = response->getToken(0);
	if (status != "OK") {
		status = response->getToken(1);
		m_errorMessage.assign (Utils::LoadResourcesString(7));
		m_errorMessage.append(status.c_str());
		return false;
	} else {
		int i = 1;
		status = response->getToken(i);
		while (! status.empty()) {
			if (status == "user") {
				std::wstring user;
				response->getToken(i+1, user);
				Utils::duplicateString(szUser, user.c_str());
			}
			if (status == "password") {
				std::wstring password;
				response->getToken(i+1, password);
				Utils::duplicateString(szPassword, password.c_str());
			}
			i += 2;
			status = response->getToken(i);
		}
		if (szUser == NULL || szPassword == NULL) {
			m_errorMessage.assign (Utils::LoadResourcesString(8));
			return false;
		} else {
			m_handler->saveCredentials(this, szUser, szPassword);
			return true;
		}
	}
}
