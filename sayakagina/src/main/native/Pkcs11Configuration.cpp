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
#include <windows.h>
#include "NotificationHandler.h"
#include "winwlx.h"
#include <MZNcompat.h>

# include "Utils.h"

#include <vector>

static DWORD __stdcall changeNotifier(LPVOID param) {
	Pkcs11Configuration *p = (Pkcs11Configuration*) param;

	Sleep(3000);

	p->waitForToken();

	return 0;
}

Pkcs11Configuration::Pkcs11Configuration() :
	m_log("Pkcs11Configuration")
{
	m_log.info("Loading PKCS#11 Providers");
	m_provider = NULL;
	m_hMutex =	CreateMutex(NULL,              // default security attributes
	        FALSE,             // initially not owned
	        NULL);

	m_hThreadStopEvent = CreateEventW(NULL, // default security attributes
			FALSE, // manual reset event
			FALSE, // not signaled
			NULL); // No name
	CreateThread(NULL, 0, ::changeNotifier, this, 0, NULL);
}

bool Pkcs11Configuration::registerDriver(const char *achLibrary) {
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
		Pkcs11Handler *p11 = new Pkcs11Handler(achLibrary, m_hMutex);
		m_handlers.push_back(p11);
		m_provider->onProviderAdd();
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
	m_handlers.clear ();
	SCardReleaseContext(cardCtx);
}
void Pkcs11Configuration::stop() {
	SetEvent (m_hThreadStopEvent);
}

void Pkcs11Configuration::setNotificationHandler(NotificationHandler *provider) {
	m_provider = provider;
//	for (std::vector<Pkcs11Handler*>::iterator it = m_handlers.begin(); it
//			!= m_handlers.end(); it++) {
//		Pkcs11Handler *handler = *it;
//		handler -> setNotificationHandler(provider);
//	}
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



void Pkcs11Configuration::waitForToken () {
	m_log.info("Waiting for card service");
	HANDLE h = m_hThreadStopEvent;
	LPSTR mszReaderNames = NULL;
	HANDLE hStart = SCardAccessStartedEvent();
	WaitForSingleObject(hStart, INFINITE);
	m_log.info ("Card service started");

	SCardReleaseStartedEvent();

	SCARDCONTEXT cardCtx;
	if (SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &cardCtx) == SCARD_S_SUCCESS)
	{
		while ( true ) {
			DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;
			DWORD result = SCardListReaders (cardCtx, SCARD_DEFAULT_READERS, (LPSTR) &mszReaderNames, &dwAutoAllocate);
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
			do {
				if (WAIT_OBJECT_0 == WaitForSingleObject(h, 0) ) {
					m_log.info ("Card loop end");
					return;
				}
				m_log.info ("Waiting for event\n");
				while ( SCardGetStatusChange(cardCtx, INFINITE, state, num) != SCARD_S_SUCCESS) {
					if (WAIT_OBJECT_0 == WaitForSingleObject(h, 3000) ) {
						m_log.info ("Card loop end");
						return;
					}
				}
				if (WAIT_OBJECT_0 == WaitForSingleObject(h, 0) ) {
					m_log.info ("Card loop end");
					return;
				}
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
								!(state[i].dwCurrentState & SCARD_STATE_PRESENT) &&
								m_provider != NULL)
						{
							std::string driver;
							std::string atr = SeyconCommon::toBase64(state[i].rgbAtr, state[i].cbAtr);
							for (size_t j = 0; j<atr.size(); j++)
								if (atr.at(j) == '/')
									atr.replace(j, j, 1, '-');
							m_log.info ("Atr: %s", atr.c_str());
							// Load handler
							std::string attribute = "sayaka.pkcs11.";
							attribute.append(atr);
							bool driverFound;
							driverFound = SeyconCommon::readProperty(attribute.c_str(), driver);
							if (!driverFound) {
								SeyconCommon::updateConfig(attribute.c_str());
								driverFound = SeyconCommon::readProperty(attribute.c_str(), driver);
							}
							if (driverFound) {
								m_log.info ("Registering driver %s", driver.c_str());
								registerDriver(driver.c_str());
								Sleep(1000);
								m_log.info ("Notifying card insertion");
								if (m_provider != NULL) {
									m_provider->onCardInsert();
								}
								m_log.info ("Notified card insertion");
							} else {
								m_log.info ("No driver found for ATR %s", atr.c_str());
								std::wstring msg = MZNC_strtowstr(Utils::LoadResourcesString(41).c_str()).c_str();
								msg.append( MZNC_strtowstr(atr.c_str()));
								pWinLogon->WlxMessageBox(hWlx,
										NULL,
										msg.c_str(),
										MZNC_strtowstr(Utils::LoadResourcesString(31).c_str()).c_str(),
										MB_OK|MB_ICONEXCLAMATION);
							}

						}
						if ((state[i].dwEventState & SCARD_STATE_CHANGED) &&
								!(state[i].dwEventState & SCARD_STATE_PRESENT) &&
								(state[i].dwCurrentState & SCARD_STATE_PRESENT) &&
								m_provider != NULL)
						{
							m_log.info ("Notifying card remove");
							m_provider->onCardRemove();
						}
					}
					state[i].dwCurrentState = state[i].dwEventState;
				}
			} while (!readersChanged);
			free (state);
			if (mszReaderNames != NULL)
				SCardFreeMemory( cardCtx, mszReaderNames );
		}
	}
}
