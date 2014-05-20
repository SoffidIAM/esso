/*
 * CertificateHandler.cpp
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "CertificateHandler.h"
#include "Pkcs11Handler.h"
#include "Log.h"
#include "Utils.h"
#include <ssoclient.h>
#include "lm.h"
#include "SecurityHelper.h"

CertificateHandler::CertificateHandler(Pkcs11Handler *handler) {
	m_handler = handler;
	m_certContext = NULL;
	m_achId = NULL;
	m_pvCert = NULL;
}

CertificateHandler::~CertificateHandler() {
	if (m_achId != NULL)
		free(m_achId);
	if (m_pvCert != NULL)
		free(m_pvCert);
	if (m_certContext != NULL)
		CertFreeCertificateContext(m_certContext);
}


void CertificateHandler::setPassword (const wchar_t* szPassword) {
	this->szPassword.assign(szPassword);
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



bool CertificateHandler::obtainCredentials() {

	SeyconService service;

	service.resetServerStatus ();


	SeyconResponse *response = service.sendUrlMessage ( L"/certificateLogin?action=start" );
	SecurityHelper::getDomain(szDomain);

	if (response == NULL) {
		// Buscar en el cache

		if (m_handler->getSavedCredentials(this, szUser, szPassword)) {
			return true;
		} else {
			m_errorMessage.assign(Utils::LoadResourcesString(2));
			return false;
		}
	} else {
		std::string status = response->getToken(0);
		if (status == "OK") {
			std::string sessionId;
			std::wstring wsessionId;
			response -> getToken (1, sessionId);
			response -> getToken (1, wsessionId);

			delete response;

			unsigned char * pData;
			unsigned long dwSize;

			if (! m_handler->sign(this, sessionId.c_str(), &pData, &dwSize)) {
				m_errorMessage.assign(Utils::LoadResourcesString(3));
				return false;
			}

			// Transformar base 64 la firma
			std::string msg64 = SeyconCommon::toBase64 (pData, dwSize);
			// Transformar wide char
			std::wstring wmsg64 = service.escapeString (msg64.c_str());

			// Transformar base 64 el certificado
			std::string cert = SeyconCommon::toBase64 (m_pvCert, m_ulCertLength);
			// Transformar wide char
			std::wstring wcert = service.escapeString (cert.c_str());

			response = service.sendUrlMessage (L"/certificateLogin?action=continue&challengeId=%s&cert=%s&signature=%s",
					wsessionId.c_str(), wcert.c_str(), wmsg64.c_str());
			if (response == NULL)
			{
				m_errorMessage.assign (Utils::LoadResourcesString(4));
				return false;
			}
			bool ok = parseResponse(response);
			delete response;
			return ok;
		} else {
			m_errorMessage.assign (Utils::LoadResourcesString(5));
			m_errorMessage.append(response ->getToken(1));
			delete response;
			return NULL;
		}
	}

	return NULL;
}

bool CertificateHandler::parseResponse (SeyconResponse *resp) {
	if (resp == NULL)
	{
		return false;
	}
	std::string status = resp->getToken(0);
	if (status != "OK") {
		m_errorMessage.assign (Utils::LoadResourcesString(6));
		m_errorMessage.append(resp->getToken(1).c_str());
		return false;
	} else {
		int i = 1;
		status = resp->getToken(i);
		while (! status.empty ()) {
			if (status == "user") {
				resp->getToken (i+1, szUser);
			}
			if (status == "password") {
				resp->getToken(i+1, szPassword);
			}
			i += 2;
			status = resp->getToken(i);
		}
		if (szUser.size () == 0 || szPassword.size () == 0) {
			m_errorMessage.assign (Utils::LoadResourcesString(7));
			return false;
		} else {
			m_handler->saveCredentials(this, szUser, szPassword);
			return true;
		}
	}
}
