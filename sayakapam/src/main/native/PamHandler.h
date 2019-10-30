/*
 * PamHandler.h
 *
 *  Created on: 19/05/2011
 *      Author: u07286
 */

#ifndef PAMHANDLER_H_
#define PAMHANDLER_H_

#include "Log.h"
#define PAM_SM_AUTH
#include <security/pam_modules.h>
#include "Pkcs11Configuration.h"
#include <ssoclient.h>

// #define __TRACE__ printf ( ">> %s: %d\n", __FILE__, __LINE__ )
#define __TRACE__ NULL

class PamDialog;

class PamHandler {
public:
	PamHandler(pam_handle_t *pamh);
	virtual ~PamHandler();

	int authenticate();
	int createSession();
	int closeSession();
	int changePassword ();
	void progressMessage (const char*);
	void notify (const char*);
	int readPassword (const char *msg = NULL);
	int readNewPassword (const char *msg = NULL);
	int readCardValue (const char *msg);
	int genericQuestion (const char *prompt, std::string &result);
	void clear ();
	void enableDebug();

	const char *getPassword () {return password;};
	const char *getNewPassword () {return newpassword;};
	const char *getCardValue () {return cardValue;};

	static PamHandler* getPamHandler (pam_handle_t *pamh);

	void parse (int argc, const char **argv);

	pam_handle_t* getPamHandler () { return m_pamh; };

	// Gestió del dialog socket
	bool isDialogConnected () { return dialogSocket != -1; }
	std::string sendDialogMessage (const char *msg);

	Log m_log;

private:
	pam_handle_t *m_pamh;
	char *user;
	char *service;
	char *password;
	char *newpassword;
	char *cardValue;
	std::string fullName;
	std::string group;
	int authenticatePassword();
	int authenticatePkcs11();
	int readPasswordInternal (const char *msg, char *&member);
	void ensureNewPassword ();
	void createLocalUser ();
	void obtainUserData();
	void getPamUser () ;
	int readPin(TokenHandler *token);
	Pkcs11Configuration p11config;
	bool success;
	bool onlySession;
	bool m_pkcs11;

	PamDialog *pamDialog;
	SeyconSession session;
    void storeLocalPassword();
    void dialogThread ();
    void handleDialogRequest (int socket);
    int createSocket (const char *name);
    int dialogSocket;
    int listenSocket;


    static int conv(int num_msg, const struct pam_message **msg,
		struct pam_response **resp, void *appdata_ptr);

    static void* pamSocketDialogThread (void *);
};


#endif /* PAMHANDLER_H_ */
