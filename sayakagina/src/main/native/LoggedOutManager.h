/*
 * LoggedOutManager.h
 *
 *  Created on: 11/02/2011
 *      Author: u07286
 */

#ifndef LOGGEDOUTMANAGER_H_
#define LOGGEDOUTMANAGER_H_

#include <string>
#include "Log.h"
#include "Pkcs11Configuration.h"
#include "logindialog.h"
#include "LoginStatus.h"
#include <ntsecapi.h>

class LoggedOutManager {
public:
	LoggedOutManager();
	virtual ~LoggedOutManager();
    PLUID getAuthenticationId() const
    {
        return pAuthenticationId;
    }

    DWORD getDwSasType() const
    {
        return dwSasType;
    }

    PSID getLogonSid() const
    {
        return pLogonSid;
    }

    PWLX_MPR_NOTIFY_INFO getMprNotifyInfo() const
    {
        return pMprNotifyInfo;
    }

    PDWORD getPdwOptions() const
    {
        return pdwOptions;
    }

    PHANDLE getPhToken() const
    {
        return phToken;
    }

    PVOID *getProfile() const
    {
        return pProfile;
    }

    void setAuthenticationId(PLUID pAuthenticationId)
    {
        this->pAuthenticationId = pAuthenticationId;
    }

    void setDwSasType(DWORD dwSasType)
    {
        this->dwSasType = dwSasType;
    }

    void setLogonSid(PSID pLogonSid)
    {
        this->pLogonSid = pLogonSid;
    }

    void setMprNotifyInfo(PWLX_MPR_NOTIFY_INFO pMprNotifyInfo)
    {
        this->pMprNotifyInfo = pMprNotifyInfo;
    }

    void setPdwOptions(PDWORD pdwOptions)
    {
        this->pdwOptions = pdwOptions;
    }

    void setPhToken(PHANDLE phToken)
    {
        this->phToken = phToken;
    }

    void setProfile(PVOID *pProfile)
    {
        this->pProfile = pProfile;
    }

    int process ();
    int processCard ();

    HANDLE getLsa() const
    {
        return hLsa;
    }

    void setLsa(HANDLE hLsa)
    {
        this->hLsa = hLsa;
    }


    void onCardInsert ();
private:
    DWORD           dwSasType;
    PLUID           pAuthenticationId;
    PSID            pLogonSid;
    PDWORD          pdwOptions;
    PHANDLE         phToken;
    PWLX_MPR_NOTIFY_INFO    pMprNotifyInfo;
    PVOID           *pProfile;

    std::wstring   szUser;
    std::wstring   szPassword;
    std::wstring   szDomain;

    std::wstring   szLocalHostName;

    bool doLogin ();
    Log m_log;
    MSV1_0_INTERACTIVE_PROFILE *pInteractiveProfile;
    HANDLE hLsa;
    LoginDialog    loginDialog;

    void recordLogonTime ();

    bool tryCardLogon ();
    bool chanceToChangePassword ();
};

#endif /* LOGGEDOUTMANAGER_H_ */
