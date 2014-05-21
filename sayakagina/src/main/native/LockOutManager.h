/*
 * LoggedOutManager.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef LOCKOUTMANAGER_H_
#define LOCKOUTMANAGER_H_


#include <string>
#include "Log.h"
#include "Pkcs11Configuration.h"
#include "logindialog.h"
class LoginStatus;


class LockOutManager {
public:
	LockOutManager();
	virtual ~LockOutManager();

	int process ();
    int processCard ();

    void onCardInsert ();

    void setLoginStatus (LoginStatus *pLoginStatus) {
    	this->pLoginStatus = pLoginStatus;
    }

    LoginStatus *getLoginStatus () {
    	return pLoginStatus;
    }


private:

    Log m_log;
    LoginDialog loginDialog;
    std::wstring   szUser;
    std::wstring   szPassword;
    std::wstring   szDomain;
    std::wstring   szLocalHostName;
    LoginStatus *  pLoginStatus;
    bool doLogin ();
    bool m_bSameUser;
};

#endif /* LOCKOUTMANAGER_H_ */
