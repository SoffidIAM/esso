/*
 * Pkcs11Configuration.h
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#ifndef PKCS11CONFIGURATION_H_
#define PKCS11CONFIGURATION_H_

#include "Pkcs11Handler.h"
#include "CertificateHandler.h"
#include "Log.h"
#include <vector>
#include <winscard.h>

class TokenHandler;
class NotificationHandler;

class Pkcs11Configuration {
public:
	Pkcs11Configuration();
	virtual ~Pkcs11Configuration();
	void setNotificationHandler ( NotificationHandler *provider);
	std::vector<TokenHandler*> &enumTokens ();

	void init ();
	void waitForToken () ;
	void stop ();
private:
	bool registerDriver(const char *szDriver) ;
	void clearTokens () ;
	void clearCerts () ;
	std::vector<Pkcs11Handler*> m_handlers;
	std::vector<CertificateHandler*> m_certs;
	std::vector<TokenHandler*> m_tokens;
	Log m_log;
	NotificationHandler *m_provider;
	HANDLE m_hMutex;
	HANDLE m_hThreadStopEvent;
	SCARDCONTEXT cardCtx;

};

#endif /* PKCS11CONFIGURATION_H_ */
