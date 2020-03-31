/*
 * Pkcs11Configuration.cpp
 *
 *  Created on: 03/02/2011
 *      Author: u07286
 */

#include "sayaka.h"
#include "Pkcs11Configuration.h"
#include "Pkcs11Handler.h"
#include "CertificateHandler.h"
#include <ssoclient.h>
#include "TokenHandler.h"
#include <winscard.h>
#include "PamHandler.h"
#include <unistd.h>

Pkcs11Configuration::Pkcs11Configuration(PamHandler *handler) {
	__TRACE__;
	m_log.init (handler);
	__TRACE__;
	this->handler = handler;
	__TRACE__;
}

class Tokenizer {
public:
	Tokenizer (char *szString) {
		sz = szString;
		pointer = 0;
		end = false;
	}
	char *next () {
		if (end) return NULL;
		while (sz[pointer] == ' ')
			pointer ++;;
		char *result = &sz[pointer];
		while (sz[pointer] != ',' && sz[pointer] != '\0')
			pointer ++;
		if (sz[pointer] == '\0')
			end = true;
		int i = pointer;
		while ( i > 0 && sz[pointer-1] == ' ')
			i--;
		sz[i] = '\0';
		pointer ++;
		return result;
	}
private:
	char *sz;
	int pointer;
	bool end;
} ;


bool Pkcs11Configuration::registerDriver(const char *achLibrary) {
	m_log.info("Loading PKCS#11 Provider %s", achLibrary);
	bool found = false;
	for (std::vector<Pkcs11Handler*>::iterator it = m_handlers.begin(); it
			!= m_handlers.end(); it++) {
		Pkcs11Handler *handler = *it;
		if (strcmp(handler->getLibraryName(), achLibrary) == 0)
			found = true;
	}
	if (!found)
	{
		m_log.info ("Loading pkcs11 driver %s", achLibrary);
		Pkcs11Handler *p11 = new Pkcs11Handler(handler, achLibrary);
		m_handlers.push_back(p11);
	}
	return !found;
}


Pkcs11Configuration::~Pkcs11Configuration() {
	clearCerts();
	clearTokens();
	for (std::vector<Pkcs11Handler*>::iterator it = m_handlers.begin(); it
			!= m_handlers.end(); it++) {
		delete *it;
	}
}


void Pkcs11Configuration::clearCerts() {
	for (std::vector<CertificateHandler*>::iterator itCert = m_certs.begin(); itCert
			!= m_certs.end(); itCert++) {
		CertificateHandler *cert = *itCert;
		delete cert;
	}
	m_certs.clear();
}

void Pkcs11Configuration::clearTokens() {
	// Do not delete. Each pkcs11handler deletes it
	m_tokens.clear();
}

std::vector<TokenHandler*>& Pkcs11Configuration::enumTokens() {
	m_log.info("Reading tokens");
	clearTokens();
	for (std::vector<Pkcs11Handler*>::iterator it = m_handlers.begin(); it
			!= m_handlers.end(); it++) {
		std::vector<TokenHandler*> tokens = (*it)->enumTokens();
		for (std::vector<TokenHandler*>::iterator itToken = tokens.begin(); itToken
				!= tokens.end(); itToken++) {
			m_tokens.push_back(*itToken);
		}
	}
	return m_tokens;
}

bool Pkcs11Configuration::detectToken (PamHandler *handler) {
	bool found = false;
	m_log.info("Waiting for card service");
	char* mszReaderNames = NULL;
	m_log.info ("Card service started");

	SCARDCONTEXT cardCtx;
	if (SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &cardCtx) == SCARD_S_SUCCESS)
	{
		DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
		DWORD result = SCardListReaders (cardCtx, NULL, (LPSTR) &mszReaderNames, &dwAutoAllocate);
		int max ;
		SCARD_READERSTATE *state;
		if (result == SCARD_S_SUCCESS)  {
			m_log.info ("Got readers\n");
			LPSTR pReader = mszReaderNames;
			max = 0;
			while ( '\0' != *pReader )
			{
				// Display the value.
				max ++;
				m_log.info ("Detected reader %s", pReader );
				// Advance to the next value.
				pReader = pReader + strlen(pReader) + 1;
			}
			state = (SCARD_READERSTATE *) malloc ( (max+1) * sizeof (SCARD_READERSTATE));
			pReader = mszReaderNames;
			int i = 0;
			while ( i < max )
			{
				state [i].szReader = pReader;
				state [i].pvUserData = NULL;
				state [i].dwCurrentState = SCARD_STATE_EMPTY;
				state [i].dwEventState = SCARD_STATE_EMPTY;
				state [i].cbAtr = 0;
				i ++;
				// Advance to the next value.
				pReader = pReader + strlen(pReader) + 1;
			}
		} else {
			m_log.info ("No reader available");
			mszReaderNames = NULL;
			max = 0;
			state  = (SCARD_READERSTATE *) malloc ( (max+1) * sizeof (SCARD_READERSTATE));
		}
		state [max].szReader = "\\\\?PnP?\\Notification";
		state [max].pvUserData = NULL;
		state [max].dwCurrentState = max << 16;
		state [max].dwEventState = SCARD_STATE_UNAWARE;
		state [max].cbAtr = 0;
		int num = max + 1;
		bool readersChanged = false;
		m_log.info ("Waiting for event\n");
		if ( SCardGetStatusChange(cardCtx, 500, state, num) != SCARD_S_SUCCESS)
			return false;
		m_log.info ("Event received\n");
		for (int i = 0; i < num; i++)
		{
			std::string status;
			if (state[i].dwEventState & SCARD_STATE_IGNORE)
				status.append ("IGNORE ");
			if (state[i].dwEventState &  SCARD_STATE_CHANGED)
				status.append ("CHANGED ");
			if (state[i].dwEventState &  SCARD_STATE_UNKNOWN)
				status.append ("UNKNOWN ");
			if (state[i].dwEventState &  SCARD_STATE_UNAVAILABLE)
				status.append ("UNAVAILABLE ");
			if (state[i].dwEventState &  SCARD_STATE_EMPTY)
				status.append ("EMPTY ");
			if (state[i].dwEventState &  SCARD_STATE_PRESENT)
				status.append ("PRESENT ");
			if (state[i].dwEventState &  SCARD_STATE_ATRMATCH)
				status.append ("ATRMATCH ");
			if (state[i].dwEventState &  SCARD_STATE_EXCLUSIVE)
				status.append ("EXCLUSIVE ");
			if (state[i].dwEventState &  SCARD_STATE_INUSE)
				status.append ("INUSE ");
			if (state[i].dwEventState &  SCARD_STATE_MUTE)
				status.append ("MUTE ");
			if ( i == num - 1) {
				int readers = state[i].dwEventState >> 16;
				m_log.info ("%s: %s (%d readers)", state[i].szReader, status.c_str(), readers);
				if (readers != num -1) {
					readersChanged = true;
				}
			} else {
				m_log.info ("%s: %s", state[i].szReader, status.c_str());
				if ((state[i].dwEventState & SCARD_STATE_CHANGED) &&
						(state[i].dwEventState & SCARD_STATE_PRESENT) &&
						!(state[i].dwCurrentState & SCARD_STATE_PRESENT))
				{
					found = true;
					std::string driver;
					std::string atr = SeyconCommon::toBase64(state[i].rgbAtr, state[i].cbAtr);
					for (size_t j = 0; j < atr.size(); j++) {
						if (atr.at(j) == '/')
							atr.replace(j, j, 1, '-');
					}
					m_log.info ("Atr: %s", atr.c_str());
					// Load handler
					std::string attribute = "sayaka.linux.";
					attribute.append(atr);
					bool driverFound;
					driverFound = SeyconCommon::readProperty(attribute.c_str(), driver);
					if (!driverFound) {
						SeyconService service;
						service.resetServerStatus();
						SeyconCommon::updateConfig(attribute.c_str());
						driverFound = SeyconCommon::readProperty(attribute.c_str(), driver);
					}
					if (driverFound) {
						m_log.info ("Registering driver %s", driver.c_str());
						registerDriver(driver.c_str());
						sleep(1);
						m_log.info ("Notifying card insertion");
						m_log.info ("Notified card insertion");
					} else {
						std::string msg = "No driver found for ATR "+atr;
						handler->notify (msg.c_str());
						SeyconService service;
						service.resetServerStatus();
					}
				}
				if ((state[i].dwEventState & SCARD_STATE_CHANGED) &&
						!(state[i].dwEventState & SCARD_STATE_PRESENT) &&
						(state[i].dwCurrentState & SCARD_STATE_PRESENT))
				{
					m_log.info ("Notifying card remove");
				}
			}
			state[i].dwCurrentState = state[i].dwEventState;
		}
		free (state);
		if (mszReaderNames != NULL)
			SCardFreeMemory( cardCtx, mszReaderNames );
	}

	return found;

}
