/*
 * Pkcs11Configuration.h
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#ifndef PKCS11CONFIGURATION_H_
#define PKCS11CONFIGURATION_H_

#include "Log.h"
#include <vector>

#include "../cert/CertificateHandler.h"
#include "../cert/Pkcs11Handler.h"
class SayakaProvider;
class TokenHandler;

class Pkcs11Configuration {
public:
	Pkcs11Configuration();
	virtual ~Pkcs11Configuration();
	void setNotificationHandler ( SayakaProvider *provider);
	std::vector<TokenHandler*> &enumTokens ();

	void init ();

	void waitForToken ();
private:
	bool registerDriver(const char *achLibrary) ;
	void clearTokens () ;
	void clearCerts () ;
	std::vector<Pkcs11Handler*> m_handlers;
	std::vector<CertificateHandler*> m_certs;
	std::vector<TokenHandler*> m_tokens;
	Log m_log;
	SayakaProvider *m_provider;
	HANDLE m_hMutex;
	HANDLE m_hThreadStopEvent;
	bool initialized;
};

#endif /* PKCS11CONFIGURATION_H_ */
