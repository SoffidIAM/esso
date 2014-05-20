/*
 * CertificateHandler.h
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#ifndef CERTIFICATEHANDLER_H_
#define CERTIFICATEHANDLER_H_

class Pkcs11Handler;
class ICredentialProviderCredentialEvents;
class TokenHandler;

#include <pkcs11.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <openssl/x509.h>

class SeyconResponse;


class CertificateHandler {
public:
	CertificateHandler(Pkcs11Handler *handler);
	virtual ~CertificateHandler();
    unsigned char *getId() const
    {
        return m_achId;
    }

    const char *getName() const
    {
        return m_achName;
    }

    const char* getExpirationDate();

    Pkcs11Handler *getHandler() const
    {
        return m_handler;
    }

    bool getNearExpireDate() ;
    bool isValid ();

    unsigned char *getRawCert() const
    {
        return m_pvCert;
    }

    unsigned long getCertLength() const
    {
        return m_ulCertLength;
    }

    unsigned long getIdLength() const
    {
        return m_ulIdLength;
    }

    void setId(unsigned char *m_achId, int length)
    {
        this->m_achId = (unsigned char*)(malloc(length));
        memcpy(this->m_achId, m_achId, length);
        m_ulIdLength = length;
    }

    void setHandler(Pkcs11Handler *m_handler)
    {
        this->m_handler = m_handler;
    }

    void setRawCert(unsigned char *m_pvCert, int certSize)
    {
        this->m_pvCert = (unsigned char*)(malloc(certSize));
        memcpy(this->m_pvCert, m_pvCert, certSize);
        m_ulCertLength = certSize;
        parseCert();
    }


    X509* getCertContext();
    const char *getIssuer() const
    {
        return m_achIssuer;
    }

    const char *getPassword() const
    {
        return m_password.c_str();
    }

    void setPassword (const char* szPassword);

    const char *getUser() const
    {
        return m_user.c_str();
    }

    bool obtainCredentials() ;

    const char *getErrorMessage() {
    	return m_errorMessage.c_str();
    }

    void setTokenHandler (TokenHandler *pToken) {
    	m_token = pToken;
    }

    TokenHandler *getTokenHandler() { return m_token; }


private:
    char *parsex509Name (X509_NAME *n);
    bool parseResponse (SeyconResponse* resp) ;
    void checkNearExpire ();
	Pkcs11Handler *m_handler;
	unsigned char *m_achId;
	unsigned long m_ulIdLength;
	char *m_achName;
	char *m_achIssuer;
	unsigned char *m_pvCert;
	unsigned long m_ulCertLength;
	X509* m_certContext;
    void parseCert();

    std::string m_user;
    std::string m_password;
    TokenHandler *m_token;
    std::string m_errorMessage;
    char *expirationString;
};

#endif /* CERTIFICATEHANDLER_H_ */
