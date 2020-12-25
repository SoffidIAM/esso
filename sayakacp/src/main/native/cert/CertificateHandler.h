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
class SeyconResponse;

#include <pkcs11.h>
#include <windows.h>

#include <wincrypt.h>
#include <string>

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

    FILETIME getExpirationDate()
    {
        return getCertContext()->pCertInfo->NotAfter;
    }

    Pkcs11Handler *getHandler() const
    {
        return m_handler;
    }

    bool getNearExpireDate() ;

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


    PCCERT_CONTEXT getCertContext();
    const char *getIssuer() const
    {
        return m_achIssuer;
    }

    wchar_t *getDomain() const
    {
        return szDomain;
    }

    wchar_t *getPassword() const
    {
        return szPassword;
    }

    void setPassword (const wchar_t* szPassword);
    wchar_t *getUser() const
    {
        return szUser;
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
	void obtainDomain () ;
    bool parseResponse (SeyconResponse *response) ;
    void checkNearExpire ();
	Pkcs11Handler *m_handler;
	unsigned char *m_achId;
	unsigned long m_ulIdLength;
	char m_achName[512];
	char m_achIssuer[512];
	unsigned char *m_pvCert;
	unsigned long m_ulCertLength;
	PCCERT_CONTEXT m_certContext;
    void parseCert();

    wchar_t *szUser;
    wchar_t *szPassword;
    wchar_t *szDomain;
    TokenHandler *m_token;
    std::string m_errorMessage;
};

#endif /* CERTIFICATEHANDLER_H_ */
