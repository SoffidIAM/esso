/*
 * ssoclient.h
 *
 *  Created on: 19/07/2010
 *      Author: u07286
 */

#ifndef SSOCLIENT_H_
#define SSOCLIENT_H_

#include <string>
#include <vector>

#ifdef WIN32
#include <winsock.h>
#include <wincrypt.h>
#endif

#include <SeyconServer.h>

#define LOGIN_ERROR   0
#define LOGIN_SUCCESS 1
#define LOGIN_DENIED  2
#define LOGIN_UNKNOWNUSER 3

/**
 * Interfaces
 */

class SsoDaemon;

enum DuplicateSessionAction {
	dsaCancel,
	dsaCloseOther,
	dsaWait
};

class SeyconDialog {
public:
	virtual ~SeyconDialog ();
	virtual bool askCard (const char* targeta, const char* cella, std::wstring &result) = 0;
	virtual bool askNewPassword (const char* reason, std::wstring &password) = 0;
	virtual DuplicateSessionAction askDuplicateSession (const char *details) = 0;
	virtual bool askAllowRemoteLogout () = 0;
	virtual void notify (const char *message) = 0;
	virtual void progressMessage (const char *message) = 0;
};


class SeyconSession {
public:
	SeyconSession ();
	void weakSessionStartup (const char* lpszUser, const wchar_t* lpszPass);
	void close ();
	int kerberosSessionStartup ();
	int passwordSessionStartup (const char* lpszUser, const wchar_t* lpszPass);
	int passwordSessionPrepare(const char* lpszUser, const wchar_t* lpszPass);
	void updateMazingerConfig ();
	void renewSecrets(const char * lpszNewKey);
	void setDialog (SeyconDialog *d) { m_dialog = d;}
	void notify (const char *message);
	void remoteClose (const char *host);
	void executeOfflineScript ();
	void updateConfiguration();

	ServiceIteratorResult iteratePassword (const char* hostName, size_t dwPort, bool prepareOnly);
	ServiceIteratorResult iterateKerberos (const char* hostName, size_t dwPort);

	void setUser (const char *user) { this->user.assign (user);}
	void setPassword (const wchar_t *pass) {this->password.assign(pass);}
	int changePassword (const wchar_t *newpass) ;

	const char *getErrorMessage ( ) { return errorMessage.c_str(); }

	const char *getUser() { return user.c_str();}
	const char *getSessionKey() { return sessionKey.c_str();}
	const char *getSessionId() {return sessionId.c_str();}
	const char *getSoffidUser() {return soffidUser.c_str();}
	void setSoffidUser(const char *soffidUser) {this->soffidUser = soffidUser;}
	bool isOpen() {return m_bOpen;}
	SeyconDialog * getDialog() { return m_dialog;}
	int restartSession ();

private:
	std::wstring getCardValue () ;
	ServiceIteratorResult kerberosLogin (const char* wchHostName, size_t dwPort) ;
	ServiceIteratorResult createKerberosSession (const char* wchHostName, size_t dwPort);
	ServiceIteratorResult createPasswordSession (const char* wchHostName, size_t dwPort);
	ServiceIteratorResult preauthChangePassword (const char* wchHostName, size_t dwPort) ;
	ServiceIteratorResult tryPasswordLogin (const char* wchHostName, size_t dwPort, bool prepareOnly) ;
	ServiceIteratorResult createSession (const char* servletName, const char* wchHostName, size_t dwPort);
	ServiceIteratorResult launchMazinger (const char* szHostName, int dwPort,
			const char* lpszServlet);
	ServiceIteratorResult launchPasswordBank (const char* szHostName, int dwPort);
	int isPasswordBankInstalled ();


	void downloadMazingerConfig(std::string &configFile) ;
	void startMazinger (SeyconResponse *resp, const char* configFile) ;
	void parseAndStoreSecrets(SeyconResponse *resp);

	void generateMenus ();

	void createWeakSession ();
	void propagatePassword ();

	void saveOfflineScript ();

	bool m_bOpen;
	std::string sessionId;
	std::string sessionKey;
	std::string user;
	std::wstring password;
	std::wstring newPassword;
	std::string cardNumber;
	std::string cardCell;
	std::string errorMessage;
	std::string soffidUser;
	SsoDaemon *m_daemon;
	SeyconDialog *m_dialog;
	int status;
	bool m_kerberosLogin;
	bool m_passwordLogin;
	std::vector<std::string> newKeys;
};

#endif /* SSOCLIENT_H_ */
