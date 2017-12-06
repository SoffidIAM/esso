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
#include <openssl/ossl_typ.h>
#include <ssoclient.h>
#include <MZNcompat.h>

CertificateHandler::CertificateHandler(Pkcs11Handler *handler) {
	m_handler = handler;
	m_certContext = NULL;
	m_achId = NULL;
	m_pvCert = NULL;
	m_user.clear ();
	m_password.clear ();
	expirationString = NULL;
}

CertificateHandler::~CertificateHandler() {
	if (m_achId != NULL)
		free(m_achId);
	if (m_pvCert != NULL)
		free(m_pvCert);
	if (m_certContext != NULL)
		OPENSSL_free(m_certContext);
	SeyconCommon::wipe (m_user);
	SeyconCommon::wipe (m_password);
	if (expirationString != NULL)
		OPENSSL_free(expirationString);
}


void CertificateHandler::setPassword (const char* szPassword) {
	SeyconCommon::wipe (m_password);
	m_password.assign (szPassword);
}

X509* CertificateHandler::getCertContext() {
	if (m_certContext == NULL) {
		const unsigned char *in = m_pvCert;
		d2i_X509(&m_certContext, &in, m_ulCertLength);
	}
	return m_certContext;

}

bool CertificateHandler::getNearExpireDate(){
	X509 *cert = getCertContext();
	if (cert == NULL)
		return false;

	unsigned char *expString = NULL;
	unsigned char *nowString = NULL;

	// Calculate expiration time
	ASN1_GENERALIZEDTIME *expiration = NULL;
	ASN1_TIME_to_generalizedtime (cert->cert_info->validity->notAfter, &expiration);
	ASN1_STRING_to_UTF8(&expString, expiration);


	// Calculate one month from now
	ASN1_GENERALIZEDTIME* t2 = ASN1_GENERALIZEDTIME_new();
	time_t t;
	time(&t);
	t += 30 * 24 * 60 * 60; // Un mes
	ASN1_GENERALIZEDTIME_set(t2, t);
	ASN1_STRING_to_UTF8(&nowString, t2);

	// Compare strings
	bool ok;
	if (strcmp ((char*)nowString, (char*)expString) > 0)
		ok =  true;
	else
		ok = false;
	OPENSSL_free(t2);
	OPENSSL_free(expiration);
	OPENSSL_free(expString);
	OPENSSL_free(nowString);
	return ok;

}

bool CertificateHandler::isValid(){
	X509 *cert = getCertContext();
	if (cert == NULL)
		return false;

	unsigned char *expString = NULL;
	unsigned char *actString = NULL;
	unsigned char *nowString = NULL;

	// Calculate expiration time
	ASN1_GENERALIZEDTIME *expiration = NULL;
	ASN1_TIME_to_generalizedtime (cert->cert_info->validity->notAfter, &expiration);
	ASN1_STRING_to_UTF8(&expString, expiration);

	// Calculate activation time
	ASN1_GENERALIZEDTIME *activation = NULL;
	ASN1_TIME_to_generalizedtime (cert->cert_info->validity->notBefore, &activation);
	ASN1_STRING_to_UTF8(&actString, activation);

	// Calculate one month from now
	ASN1_GENERALIZEDTIME* t2 = ASN1_GENERALIZEDTIME_new();
	time_t t;
	time(&t);
	ASN1_GENERALIZEDTIME_set(t2, t);
	ASN1_STRING_to_UTF8(&nowString, t2);

	// Compare strings
	bool ok;
	if (strcmp ((char*)nowString, (char*)expString) < 0 &&
			strcmp ((char*)nowString, (char*)actString) > 0)
		ok =  true;
	else
		ok = false;
	OPENSSL_free(t2);
	OPENSSL_free(activation);
	OPENSSL_free(actString);
	OPENSSL_free(expiration);
	OPENSSL_free(expString);
	OPENSSL_free(nowString);

	return ok;

}

const char* CertificateHandler::getExpirationDate()
{
	if (expirationString == NULL && getCertContext() != NULL)
	{
		ASN1_GENERALIZEDTIME *expiration = NULL;
		ASN1_TIME_to_generalizedtime (getCertContext()->cert_info->validity->notAfter, &expiration);
		ASN1_STRING_to_UTF8((unsigned char**)&expirationString, expiration);
	}
	return expirationString;
}


void CertificateHandler::parseCert() {

	X509 *cert = getCertContext();
	if (cert != NULL) {
		m_achIssuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
		m_achName = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
	}
}


bool CertificateHandler::obtainCredentials() {
	SeyconService service;

	SeyconResponse *resp = service.sendUrlMessage(L"/certificateLogin?action=start");

	unsigned char * pData;
	unsigned long dwSize;

	SeyconCommon::wipe(m_password);
	m_user.clear ();

	if (resp == NULL) {
		// Buscar en el cache

		std::string szUser;
		std::string szPassword ;
		if (m_handler->getSavedCredentials(this, szUser, szPassword)) {
			m_user.assign (szUser);
			m_password.assign (szPassword);
			SeyconCommon::wipe(szPassword);
			return true;
		} else {
			m_errorMessage.assign("Cannot connect to server to start session");
			return false;
		}
	} else {
		std::string status = resp->getToken(0);
		if (status == "OK") {
			std::wstring sessionId;
			resp->getToken(1, sessionId);
			std::string utf8SessionId = MZNC_wstrtoutf8(sessionId.c_str());
			if (! m_handler->sign(this, utf8SessionId.c_str(), &pData, &dwSize)) {
				m_errorMessage.assign("Smart card error. Cannot sign login request");
				delete resp;
				return false;
			}

			std::wstring data64 = service.escapeString(SeyconCommon::toBase64( pData, dwSize).c_str());
			std::wstring cert64 = service.escapeString(SeyconCommon::toBase64(m_pvCert, m_ulCertLength).c_str());

			SeyconResponse *resp = service.sendUrlMessage(L"/certificateLogin?action=continue&challengeId=%ls&cert=%ls&signature=%ls",
					sessionId.c_str(), cert64.c_str(), data64.c_str());

			if (resp == NULL)
			{
				m_errorMessage.assign ("Cannot contact Soffid server");
				return false;
			}
			return parseResponse(resp);
		} else {
			std::string msg = resp->getToken(1);

			m_errorMessage.assign ("Server has rejected login request: ");
			m_errorMessage.append(msg.c_str());
			delete resp;
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
		m_errorMessage.assign ("Error connecting to server: ");
		m_errorMessage.append( resp->getToken(1));
		return false;
	} else {
		int i = 1;
		std::string tag = resp->getToken(i);
		m_user.clear ();
		SeyconCommon::wipe(m_password);
		while (tag.size() > 0) {
			if (tag == "user") {
				m_user = resp->getToken(i+1);
			}
			if (tag == "password") {
				resp->getToken(i+1, m_password);
			}
			i += 2;
			tag = resp->getToken(i);
		}
		if (m_user.empty() || m_password.empty ()) {
			m_errorMessage.assign ("Internal error. Missing required credentials in login response.");
			return false;
		} else {
			m_handler->saveCredentials(this, m_user.c_str(), m_password.c_str());
			return true;
		}
	}
}
