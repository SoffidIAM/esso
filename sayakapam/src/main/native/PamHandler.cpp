/*
 * PamHandler.cpp
 *
 *  Created on: 19/05/2011
 *      Author: u07286
 */

#include "PamHandler.h"
#include <security/_pam_macros.h>
#include <security/pam_appl.h>
#include <security/pam_ext.h>
#include <stdio.h>
#include <wctype.h>
#include <ssoclient.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pcreposix.h>
#include <MZNcompat.h>
#include "Pkcs11Configuration.h"
#include "Pkcs11Handler.h"
#include "TokenHandler.h"
#include "CertificateHandler.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include "PamScriptDialog.h"

#define MAZINGER_LOOP_INDICATOR "MazingerPasswordLoop"
class PamDialog: public SeyconDialog {
public:
	PamDialog (PamHandler *handler) {
		this->handler = handler;
	}
	virtual bool askCard (const char* targeta, const char* cella, std::wstring &result);
	virtual bool askNewPassword (const char* reason, std::wstring &password) ;
	virtual DuplicateSessionAction askDuplicateSession (const char *details);
	virtual bool askAllowRemoteLogout ();
	virtual void notify (const char *message);
	virtual void progressMessage (const char *message);
private:
	PamHandler *handler;
};


PamHandler::PamHandler(pam_handle_t *pamh):
		p11config(this)
{
	__TRACE__;
	user = NULL;
	password = NULL;
	newpassword = NULL;
	service = NULL;
	m_pamh = pamh;
	m_pkcs11 = true;
	__TRACE__;
	pamDialog = new PamDialog (this);
	__TRACE__;
	session.setDialog(pamDialog);
	__TRACE__;
	m_log.init (this);
	m_log.info("Starting pam handler");
	success = false;
	dialogSocket = -1;
	listenSocket = -1;
	onlySession = true;
}

PamHandler::~PamHandler() {
	clear  ();
	delete pamDialog;
}


void PamHandler::getPamUser () {
	if (user != NULL)
	{
		free (user);
		user = NULL;
	}
	if (pam_get_user(m_pamh, (const char**) &user, "User: ") != PAM_SUCCESS || !user || !*user)
	{
		m_log.warn("Unable to retrieve the PAM user name.\n");
	}

	user = strdup(user);

}

void PamHandler::registerLocalUser() {
	std::string localUsers;
	SeyconCommon::readProperty("localUsers", localUsers);
	int pos = 0;
	do {
		std::string token;
		int i = localUsers.find(',', pos);
		if (i < 0) {
			token = localUsers.substr(pos);
			pos = localUsers.length();
		} else {
			token = localUsers.substr(pos, i - pos);
			pos = i + 1 ;
		}
		while (token.length() > 0 && token[0] == ' ')
			token = token.substr(1);
		while (token.length() > 0 && token[token.length()-1] == ' ')
			token = token.substr(0, token.length()-1);

		if (token == user)
			return; //User found
	} while ( pos < localUsers.length());
	if (localUsers.empty())
		localUsers = user;
	else
		localUsers = localUsers + " , " + user;
	SeyconCommon::writeProperty("localUsers", localUsers.c_str());
}

int PamHandler::conv (int num_msg, const struct pam_message **msg,
	struct pam_response **resp, void *appdata_ptr) {
	PamHandler *handler = (PamHandler*) appdata_ptr;
	pam_response *resp2 = (pam_response*) malloc (num_msg * sizeof (pam_response));
	*resp = resp2;
	for (int i = 0; i < num_msg; i++) {
		resp2[i].resp_retcode = 0;
		if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF) {
			resp2[i].resp = strdup(handler->newpassword == NULL? handler->password: handler->newpassword);
		}
		else
		{
			resp2[i].resp = NULL;
		}
	}
	return PAM_SUCCESS;
}

void PamHandler::storeLocalPassword()
{

    struct pam_conv conv;
    conv.appdata_ptr = this;
    conv.conv = PamHandler::conv;
    pam_handle_t *pamh2 = NULL;
    int pr = pam_start("chpasswd", user, &conv, &pamh2);
    if (pr != PAM_SUCCESS  || pamh2 == NULL ) {
		notify("Unable to store password cache");
	} else {
		pam_putenv(pamh2, MAZINGER_LOOP_INDICATOR "=1" );
//		pam_set_item(pamh2, PAM_AUTHTOK_TYPE, strdup( "MazingerInternal") );
		pr = pam_chauthtok(pamh2, 0);
		if (pr != PAM_SUCCESS) {
			printf ("PAMERROR %d\n", pr);
			notify("Unable to store password cache");
		}
		pam_close_session(pamh2, 0);
		registerLocalUser();
	}
}

int PamHandler::authenticate()
{
	success = false;
	onlySession = false;
//	const char *tty;

	int retval = pam_get_item(m_pamh, PAM_SERVICE,
			(const void **)(const void *)&service);
	if (retval != PAM_SUCCESS)
	{
		m_log.warn("Unable to retrieve the PAM service name.\n");
		return (PAM_AUTH_ERR);
	}
	service = strdup(service);

	getPamUser();
	if (user == NULL)
		return PAM_AUTH_ERR;

	m_log.info("Authentication request for user \"%s\" (%s)\n",
			user, service);

	MZNC_setUserName(user);

	int r;
	if (m_pkcs11 && p11config.detectToken(this)) {
		r = authenticatePkcs11 ();
	} else {
		r = authenticatePassword(PAM_AUTHTOK);
	}
	if (r == PAM_SUCCESS) {
		storeLocalPassword();
		success = true;
	}
	m_log.info ("Result = %d success=%d", r, (int) success);

	MZNC_setUserName(NULL);

	return r;
}

int PamHandler::authenticatePkcs11 () {
	std::vector<TokenHandler*> v = p11config.enumTokens();
	for (std::vector<TokenHandler*>::iterator it = v.begin(); it != v.end(); it++)
	{
		TokenHandler* t = *it;
		if (readPin(t) == PAM_SUCCESS) {
			m_log.info ("Parseando certificados\n");
			std::vector<CertificateHandler*> certs = t->getHandler()->enumCertificates(this, t);
			m_log.info ("Certificados encontrados: %d\n", certs.size());
			for (std::vector<CertificateHandler*>::iterator it2 = certs.begin(); it2 != certs.end(); it2++){
				CertificateHandler *cert = *it2;
				m_log.info ("Certificate %s", cert->getName());
				if (cert->obtainCredentials())
				{
					if (strcmp (cert->getUser(), user) != 0) {
						notify("L'usuari no coincideix amb el certificat utilitzat");
					} else {
						password = strdup (cert->getPassword());
						pam_set_item(m_pamh, PAM_AUTHTOK, strdup(password));
						int r = authenticatePassword(PAM_AUTHTOK);
						if ( r == PAM_SUCCESS)
							return PAM_SUCCESS;
					}
				} else {
					notify (cert->getErrorMessage());
				}

			}
		}

	}
	return authenticatePassword (PAM_AUTHTOK);
}

void PamHandler::notify(const char *message)
{
	struct pam_conv *conv = NULL;
	pam_get_item(m_pamh, PAM_CONV, (const void**)&conv);
	if (conv != NULL && conv ->conv != NULL)
	{
		struct pam_message pm;
		pm.msg = message;
		pm.msg_style = PAM_ERROR_MSG;
		struct pam_message *pmp = &pm;
		struct pam_response *resp;
		conv->conv(1, (const struct pam_message **)&pmp, &resp, conv->appdata_ptr);
	}
}

void PamHandler::progressMessage(const char *constChar)
{
	struct pam_conv *conv = NULL;
	pam_get_item(m_pamh, PAM_CONV, (const void**)&conv);
	if (conv != NULL && conv ->conv != NULL)
	{
		struct pam_message pm;
		pm.msg = constChar;
		pm.msg_style = PAM_TEXT_INFO;
		struct pam_message *pmp = &pm;
		struct pam_response *resp;
		conv->conv(1, (const struct pam_message **)&pmp, &resp, conv->appdata_ptr);
	}
}


int PamHandler::authenticatePassword(int token)
{
	m_log.info("Reading token %d\n", token);

	int rv = readPassword(token);
	if (rv == PAM_SUCCESS) {
		if (! SeyconCommon::loadCerts())
			return PAM_AUTHINFO_UNAVAIL;
		std::wstring wsz = MZNC_strtowstr(newpassword == NULL? password: newpassword);
		int r = session.passwordSessionPrepare (user, wsz.c_str());
		if ( r == LOGIN_SUCCESS) {
			createLocalUser();
			return PAM_SUCCESS;
		}
		else if ( r == LOGIN_UNKNOWNUSER)
		{
			return PAM_USER_UNKNOWN;
		}
		else if (r == LOGIN_ERROR)
		{
			notify(session.getErrorMessage());
			return PAM_AUTHINFO_UNAVAIL;
		}
		else
		{
			notify(session.getErrorMessage());
			return PAM_AUTH_ERR;
		}
	} else {
		return rv;
	}
}

int PamHandler::readPassword(int token, const char *prompt)
{
	if (password != NULL)
	{
		free (password);
		password = NULL;
	}
	m_log.info("Reading token %d\n", token);
	int rv = pam_get_item(m_pamh, token, (const void **)&password);
    if (rv == PAM_SUCCESS && password) {
//    	m_log.info("Got token %d=%s\n", token, password);
        password = strdup(password);
    } else if (token == PAM_OLDAUTHTOK && getuid() == 0) {
    	return PAM_AUTHINFO_UNAVAIL;
    } else {
    	if (prompt == NULL) {
    		char *service;
    		rv = pam_get_item (m_pamh, PAM_SERVICE, (const void**) &service);
   			prompt = "Password:";
    	}
    	rv = readPasswordInternal(prompt, password);
		if ( rv == PAM_SUCCESS ) {
//	    	m_log.info("Got token from input %s\n", password);
			pam_set_item(m_pamh, token, strdup(password));
		}
    }
	return rv;
}

int PamHandler::readNewPassword(const char *prompt)
{
	bool repeat;
	int r;
	do {
		repeat = false;
		char *newpassword1 = NULL;
		char *newpassword2 = NULL;
		r = readPasswordInternal(prompt == NULL? "New password: ": prompt, newpassword1);
		if (r == PAM_SUCCESS) {
			int r = readPasswordInternal(prompt == NULL? "Repeat new password: ": prompt, newpassword2);
			if ( r == PAM_SUCCESS ) {
				if (strcmp (newpassword1, newpassword2) == 0) {
					newpassword = strdup (newpassword1);
					if (password != NULL)
						pam_set_item(m_pamh, PAM_OLDAUTHTOK, strdup(password));
					pam_set_item(m_pamh, PAM_AUTHTOK, strdup(newpassword));
				} else {
					notify("Password mismatch. Try again");
					repeat = true;
				}
			}
		}
		if (newpassword1 != NULL) {
			memset (newpassword1, 0, strlen(newpassword1));
			free (newpassword1);
		}
		if (newpassword2 != NULL) {
			memset (newpassword2, 0, strlen(newpassword2));
			free (newpassword2);
		}
	} while (repeat);
	return r;
}

int PamHandler::readCardValue(const char *prompt)
{
	return readPasswordInternal(prompt == NULL? "New password: ": prompt, cardValue);
}

int PamHandler::createSession()
{
	if (! onlySession)
	{
		if (success) {

			PamScriptDialog psd (this);
			ScriptDialog::setScriptDialog(&psd);
			getPamUser();
			if (user == NULL || password == NULL)
				return PAM_AUTH_ERR;
			MZNC_setUserName(user);
			std::wstring wsz = MZNC_strtowstr(newpassword == NULL? password: newpassword);
			int r = session.passwordSessionStartup(user, wsz.c_str());
			ScriptDialog::setScriptDialog(NULL);
			MZNC_setUserName(NULL);

			if ( r == LOGIN_SUCCESS) {

				char achSession[512];
				sprintf (achSession,"MZN_SESSION=%s", session.getSessionId());
				pam_putenv(m_pamh, achSession);
				pthread_t thread2;
				pthread_create(&thread2, NULL, pamSocketDialogThread, this);

				return PAM_SUCCESS;
			} else if (r == LOGIN_UNKNOWNUSER){
//				notify("Using local user"); Hack for sudo -i in Ubuntu
				return PAM_SUCCESS;
			} else {
				notify(session.getErrorMessage());
				return PAM_AUTH_ERR;
			}
		}
		else {
			session.executeOfflineScript();
			return PAM_SUCCESS;
		}
	} else
		return PAM_SUCCESS;
}

int PamHandler::closeSession()
{
	if (! onlySession ) {
		MZNC_setUserName(user);
		PamScriptDialog psd (this);
		ScriptDialog::setScriptDialog(&psd);

		if (listenSocket != -1) {
			int l = listenSocket;
			listenSocket = -1;
			close (l);
		}
		if (success && session.isOpen()) session.close();

		MZNC_setUserName(NULL);
		ScriptDialog::setScriptDialog(NULL);
	}

	return PAM_SUCCESS;
}

int PamHandler::readPasswordInternal(const char *prompt, char *& member)
{
	char *result = NULL;
	m_log.info("Asking token\n");
	int rv = pam_prompt (m_pamh, PAM_PROMPT_ECHO_OFF, &result, "%s", prompt);
//	m_log.info("Got token from input %s\n", result);
	if (result == NULL)
		return PAM_ABORT;
	if (rv == PAM_SUCCESS)
		member = result;
	return rv;
}

int PamHandler::genericQuestion (const char *prompt, std::string &result)
{
	result.clear();
	char *r = NULL;
	int rv = pam_prompt (m_pamh, PAM_PROMPT_ECHO_ON, &r, "%s", prompt);
	if (r == NULL)
		return PAM_ABORT;
	if (rv == PAM_SUCCESS) {
		result = r;
		free (r);
	}
	return rv;
    return PAM_SUCCESS;
}


void PamHandler::enableDebug()
{
	SeyconCommon::setDebugLevel(2);
}

int PamHandler::readPin(TokenHandler *token)
{
	int rv;
	struct pam_conv *conv = NULL;
	struct pam_message* msg[2];
	struct pam_response *resp;
	char msg1[512];
	sprintf (msg1, "Card %s", token->getTokenDescription());
	rv = pam_get_item(m_pamh, PAM_CONV, (const void **)&conv);
	if (rv != PAM_SUCCESS) {
			return PAM_AUTHINFO_UNAVAIL;
	}
	if ((conv == NULL) || (conv->conv == NULL)) {
			return PAM_AUTHINFO_UNAVAIL;
	}
	msg[0] = new pam_message;
	msg[0]->msg_style = PAM_TEXT_INFO;
	msg[0]->msg = msg1;
	msg[1] = new pam_message;
	msg[1]->msg = "PIN: ";
	msg[1]->msg_style = PAM_PROMPT_ECHO_OFF;
	rv = conv->conv(2, (const pam_message **)msg, &resp, conv->appdata_ptr);
	delete msg[0];
	delete msg[1];
	if (rv != PAM_SUCCESS) {
			return PAM_AUTHINFO_UNAVAIL;
	}
	if ((resp == NULL) || (resp[1].resp == NULL)) {
			return PAM_AUTHINFO_UNAVAIL;
	}
	token->setPin(resp[1].resp);
	/* overwrite memory and release it */
	memset(resp[1].resp, 0, strlen(resp[1].resp));
    return PAM_SUCCESS;
}

void PamHandler::clear()
{
	if (user != NULL) {
		memset (user, 0, strlen(user));
		free (user);
		user = NULL;
	}
	if (password != NULL) {
		memset (password, 0, strlen (password));
		free (password);
		password = NULL;
	}
	if (newpassword != NULL) {
		memset (newpassword, 0, strlen (newpassword));
		free (newpassword);
		newpassword = NULL;
	}
	if (service != NULL) {
		memset (service, 0, strlen (service));
		free (service);
		service = NULL;
	}
}


void PamHandler::createLocalUser()
{
	// Verificar que l'usuari existenx
	struct passwd *pwd = getpwnam (user);
	if (pwd == NULL)
	{
		obtainUserData();
		int pid = fork ();
		if (pid) {
			int status;
			do {
				waitpid (pid, &status, 0);
			} while (!WIFEXITED(status));
			sleep(2);
			pam_set_item(m_pamh, PAM_USER, user);
		} else {
			execl("/usr/sbin/adduser", "adduser", "--disabled-password", user,
					"--gecos", fullName.empty()? user: fullName.c_str(),
					NULL);
		}
	}
}

static char* parseXML (const char* document, const char *tag) {
	char achPattern[512];
	std::string result;
	result.clear ();
	sprintf (achPattern, "<%s>", tag);
	const char *match = strstr(document,tag);
	if (match != NULL) {
		match += strlen (tag) + 1;
		while (*match && *match != '<') {
			result.append(1, *match);
			match ++;
		}
	}
	if (result.empty()) {
		return strdup ("");
	} else {
		return strdup(result.c_str());
	}
}

void PamHandler::obtainUserData () {
	char achUrl[1024];
	sprintf (achUrl, "/query/user/%s", user);

	SeyconService service;
	SeyconResponse *resp = service.sendUrlMessage(L"/query/user/%s", user);
	fullName.clear ();
	group.clear();
	if (resp != NULL)
	{
		char *name = parseXML(resp->getResult(), "USU_NOM");
		char *lli1 = parseXML(resp->getResult(), "USU_PRILLI");
		char *lli2 = parseXML(resp->getResult(), "USU_SEGLLI");
		char *grup = parseXML(resp->getResult(), "GRU_CODI");
		fullName.assign(name); free(name);
		fullName.append(" ");
		fullName.append(lli1); free (lli1);
		fullName.append(" ");
		fullName.append(lli2); free (lli2);
		group.assign(grup); free (grup);
		delete resp;
	}

}

static void PamHandlerCleanup (pam_handle_t *pamh, void *data, int error_status) {
	PamHandler *handler = (PamHandler*) data;
	delete handler;
}


PamHandler* PamHandler::getPamHandler (pam_handle_t *pamh) {
	__TRACE__;
	PamHandler *handler = NULL;
	__TRACE__;
	if (pam_get_data(pamh, "pam_sayaka", (const void**)&handler) == PAM_NO_MODULE_DATA) {
		__TRACE__;
		handler = new PamHandler(pamh);
		__TRACE__;
		pam_set_data (pamh, "pam_sayaka", handler, PamHandlerCleanup);
		__TRACE__;
	}
	__TRACE__;
	return handler;
}

void PamHandler::parse (int argc, const char **argv) {
	__TRACE__;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "DEBUG") == 0) {
			enableDebug();
		} else if (strcmp (argv[i], "no_pkcs11") == 0) {
			m_pkcs11 = false;
		}

	}
	__TRACE__;
}

int PamHandler::changePassword () {
	const char *loop = pam_getenv(m_pamh, MAZINGER_LOOP_INDICATOR);
	if (loop != NULL && strcmp (loop, "1") == 0)
	{
		int r = readPasswordInternal("New password: ", newpassword);
		if (r == PAM_SUCCESS)
			pam_set_item(m_pamh, PAM_AUTHTOK, newpassword);
		return PAM_USER_UNKNOWN;
	}
	int retval = pam_get_item(m_pamh, PAM_SERVICE,
			(const void **)(const void *)&service);
	if (retval != PAM_SUCCESS)
	{
		m_log.warn("Unable to retrieve the PAM service name.\n");
		return (PAM_AUTH_ERR);
	}
	service = strdup(service);

	getPamUser();
	if (user == NULL)
		return PAM_AUTH_ERR;

	m_log.info("Change Password request for user \"%s\" (service %s)\n",
			user, service);

	int r;
	std::string msg = "Changing user password ";
	msg.append (user);
	notify(msg.c_str());

	MZNC_setUserName(user);

	if (m_pkcs11 && p11config.detectToken(this)) {
		r = authenticatePkcs11 ();
	} else {
		r = authenticatePassword(PAM_OLDAUTHTOK);
	}

//	m_log.info("password results = %d\n", r);
//	m_log.info("old password = %s\n", password);
//	m_log.info("new password = %s\n", newpassword);

	MZNC_setUserName(NULL);

	if (r == PAM_SUCCESS) {
		ensureNewPassword ();
		std::wstring wsz = MZNC_strtowstr(newpassword);
		int change = session.changePassword(wsz.c_str());
		if (change == LOGIN_SUCCESS) {
			m_log.info ("Server changed");
//			storeLocalPassword();
			r = PAM_SUCCESS;
		} else  {
			notify(session.getErrorMessage());
			r = PAM_ABORT;
		}
	}
	else if (r == PAM_USER_UNKNOWN)
		r = PAM_AUTHINFO_UNAVAIL;
	else if (r == PAM_AUTH_ERR)
		r = PAM_ABORT;
	else if (r == PAM_AUTHINFO_UNAVAIL) {
		ensureNewPassword ();
		notify ("Unable to get previous password. The password is being changed only in the local system");
	} else if ( r == PAM_USER_UNKNOWN) {
		ensureNewPassword ();
	}

	m_log.info ("Result = %d", r);
	return r;

}

void PamHandler::ensureNewPassword () {
	char *p;
	int rv = pam_get_item(m_pamh, PAM_AUTHTOK, (const void **)&p);
    if (rv != PAM_SUCCESS || p == NULL) {
        readNewPassword(NULL);
    }

}

std::string PamHandler::sendDialogMessage (const char *msg) {
	std::string result;
	if (isDialogConnected())  {
		if (send (dialogSocket, msg, strlen(msg)+1, 0) <= 0) {
			close (dialogSocket);
			dialogSocket = -1;
		}
		do {
			char r;
			int read = recv (dialogSocket, &r, 1, 0);
			if (read <= 0) {
				close (dialogSocket);
				dialogSocket = -1;
				break;
			}
			if (r == '\0') break;
			result += r;
		} while (true);
	}
	return result;
}


bool PamDialog::askCard (const char* targeta, const char* cella, std::wstring &result) {
	char msg[512];
	sprintf (msg, "Enter value for card %s",
			targeta);
	handler->notify(msg);
	sprintf (msg, "Cell %s: ", cella);
	int rv = handler->readCardValue(msg);
	if (rv == PAM_SUCCESS) {
		result = MZNC_strtowstr(handler->getCardValue());
		return true;
	}
	else
		return false;

}

bool PamDialog::askNewPassword (const char* reason, std::wstring &password) {
	handler->notify(reason);
	int rv = handler->readNewPassword();
	if (rv == PAM_SUCCESS) {
		password = MZNC_strtowstr(handler->getNewPassword());
		return true;
	}
	else
		return false;
}

DuplicateSessionAction PamDialog::askDuplicateSession (const char *details) {
	std::string result;

	handler->m_log.info("Avisando de sesión duplicada");

	std::string msg ="Warning. You already have a opened session..\n";
	msg += details;
	msg += "\nC[l]ose-it [W]ait o [C]ancel\n";
	handler->notify (msg.c_str());
	if (handler->genericQuestion("Acció: ", result) != PAM_SUCCESS) {
		return dsaCancel;
	}
	if (result.length() == 0) {
		return dsaCancel;
	} else if (toupper(result.at(0)) == 'L')  {
		return dsaCloseOther;
	} else if (toupper(result.at(0)) == 'E') {
		return dsaWait;
	} else {
		return dsaCancel;
	}
}

bool PamDialog::askAllowRemoteLogout () {
	if (handler->isDialogConnected()) {
		handler->sendDialogMessage("LOGOUT\n");
	}
	return true;
}

void PamDialog::notify (const char *message) {
	if (handler->isDialogConnected()) {
		std::string msg = "NOTIFY\n";
		msg += message;
		std::string result = handler->sendDialogMessage(msg.c_str());
	} else {
		handler->notify(message);
//		std::string result;
//		handler->genericQuestion("Pitjau intro: ", result);
	}
}

void PamDialog::progressMessage (const char *message) {
	handler->notify(message);
}
