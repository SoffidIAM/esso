/*
 * Pkcs11Handler.h
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#ifndef PKCS11HANDLER_H_
#define PKCS11HANDLER_H_

#include <pkcs11.h>
#include "Log.h"
#include <vector>
#include <string>

class CertificateHandler;
class TokenHandler;
class SayakaProvider;
class PamHandler;

class Pkcs11Handler {
public:
	Pkcs11Handler(PamHandler *handler, const char *pszLibraryName);
	virtual ~Pkcs11Handler();

	std::vector<CertificateHandler*>& enumCertificates (PamHandler *handler, TokenHandler* token);
	std::vector<TokenHandler*>& enumTokens ();
	void setNotificationHandler(SayakaProvider *provider);
	bool sign( CertificateHandler *cert, const char *achSessionId, unsigned char **signedData, unsigned long *size);
	bool getSavedCredentials(CertificateHandler *cert, std::string &szUser, std::string &szPassword);
	bool saveCredentials(CertificateHandler *cert, const char* szUser, const char* szPassword) ;

	const char * getLibraryName () {
		return libraryName.c_str ();
	}

private:
	void loadModule();
	void unloadModule ();

	TokenHandler* createToken (CK_SLOT_ID slot) ;

	CK_OBJECT_CLASS getCLASS(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj);
	CK_CERTIFICATE_TYPE getCERTIFICATE_TYPE(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj);
	unsigned char* getID(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj, CK_ULONG_PTR pulCount);
	unsigned char* getVALUE(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj, CK_ULONG_PTR pulCount);

	void p11_warn(const char *func, CK_RV rv) ;
	void p11_fatal(const char *func, CK_RV rv) ;

	void enumerateToken (CK_SESSION_HANDLE sess, TokenHandler *pToken);

	void addCert(CertificateHandler *cert) ;

	void evaluateCert(CK_SESSION_HANDLE sess, CK_OBJECT_HANDLE obj, TokenHandler *pToken);

	bool login (CK_SESSION_HANDLE sess, TokenHandler *token);
	void clearTokenVector () ;

	Log m_log;
	CK_FUNCTION_LIST_PTR m_p11;
	std::vector<TokenHandler*> m_tokens;
	std::vector<CertificateHandler*> m_certs;
	std::string libraryName;
	std::string expandedLibraryName;
	void *m_library;

	CK_SESSION_HANDLE session;
};


#endif /* PKCS11HANDLER_H_ */

