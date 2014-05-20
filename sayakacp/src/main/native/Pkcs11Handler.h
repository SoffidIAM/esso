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
#include "wincrypt.h"

class CertificateHandler;
class TokenHandler;
class SayakaProvider;

class Pkcs11Handler {
public:
	Pkcs11Handler(const char *pszLibraryName, HANDLE hMutex);
	virtual ~Pkcs11Handler();

	std::vector<CertificateHandler*>& enumCertificates (TokenHandler* token);
	std::vector<TokenHandler*>& enumTokens ();
	void setNotificationHandler(SayakaProvider *provider);
	bool sign( CertificateHandler *cert, const char *achSessionId, unsigned char **signedData, unsigned long *size);
	bool getSavedCredentials(CertificateHandler *cert, wchar_t* &szUser, wchar_t *&szPassword);
	bool saveCredentials(CertificateHandler *cert, wchar_t* szUser, wchar_t* szPassword) ;

	void setParentWindow (HWND hwnd) {
		m_hParentWindow = hwnd;
	}

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
	HINSTANCE m_hInstance;
	CK_FUNCTION_LIST_PTR m_p11;
	std::vector<TokenHandler*> m_tokens;
	std::vector<CertificateHandler*> m_certs;
//	SayakaProvider *m_provider;
	HANDLE m_hMutex;
	std::string libraryName;
	std::string expandedLibraryName;
	HWND m_hParentWindow;
};


#endif /* PKCS11HANDLER_H_ */

