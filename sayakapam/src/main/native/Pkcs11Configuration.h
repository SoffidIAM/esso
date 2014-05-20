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
class SayakaProvider;
class TokenHandler;
class PamHandler;

class Pkcs11Configuration {
public:
	Pkcs11Configuration(PamHandler *handler);
	virtual ~Pkcs11Configuration();
	std::vector<TokenHandler*> &enumTokens ();

	bool detectToken (PamHandler *handler);
private:
	bool registerDriver(const char *achLibrary) ;
	void clearTokens () ;
	void clearCerts () ;
	std::vector<Pkcs11Handler*> m_handlers;
	std::vector<CertificateHandler*> m_certs;
	std::vector<TokenHandler*> m_tokens;
	Log m_log;
	PamHandler *handler;
};

#endif /* PKCS11CONFIGURATION_H_ */
