#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <ctype.h>
#include <winuser.h>
#include <wtsapi32.h>
#include <winhttp.h>
#include <wincrypt.h>
#define SECURITY_WIN32
#include <security.h>
#include <stdio.h>

#else

#include <wchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#endif

#include <time.h>
#include <unistd.h>
#include <MZNcompat.h>
#include <stdio.h>
#include <ssoclient.h>
#include <stdlib.h>
#include "passwordsession.h"
#include "passwordbank.h"
#include "mazinger.h"
#include "ssodaemon.h"
#include "crypt.h"
#include "sessioncommon.h"
#include "httpHandler.h"
#include "appmenu.h"
#include "json/JsonAbstractObject.h"

#define DEBUG

ServiceIteratorResult SeyconSession::preauthChangePassword ( const char* host,
        size_t port )
{
    SeyconService service;
    std::wstring wUser = service.escapeString ( user.c_str() );
    std::wstring wPass = service.escapeString ( password.c_str() );
    std::string domain;
    SeyconCommon::readProperty("SSOSoffidAgent", domain);
    std::wstring wDomain = service.escapeString ( domain.c_str());
    std::wstring wNewPass;
    if ( m_dialog == NULL )
    {
        errorMessage = "Dialog method disabled";
        status = LOGIN_ERROR;
        return SIR_ERROR;
    }
    if ( !m_dialog->askNewPassword ( "Password expired", newPassword ) )
    {
        status = LOGIN_ERROR;
        errorMessage = "You must change password";
        return SIR_ERROR;
    }
    bool repeat;
    do
    {
        wNewPass = service.escapeString ( newPassword.c_str() );
        repeat = false;
        SeyconResponse *resp = service.sendUrlMessage ( host, port,
                               L"/passwordLogin?action=changePass&user=%ls&password1=%ls&password2=%ls&domain=%ls",
                               wUser.c_str(), wPass.c_str(), wNewPass.c_str(), wDomain.c_str() );
        if ( resp == NULL )
        {
            status = LOGIN_ERROR;
            errorMessage = "Network error when trying to change password";
            return SIR_ERROR;
        }
        std::string statusMsg;
        resp->getToken ( 0, statusMsg );
        if ( statusMsg != "OK" )
        {
            repeat = true;
            std::string msg;
            resp->getToken ( 1, msg );
            if ( !m_dialog->askNewPassword ( msg.c_str(), newPassword ) )
            {
                delete resp;
                status = LOGIN_ERROR;
                errorMessage = "You must change password";
                return SIR_ERROR;
            }
        }
        delete resp;
    }
    while ( repeat );
    return SIR_SUCCESS;
}

ServiceIteratorResult SeyconSession::tryPasswordLogin ( const char* host, size_t port,
        bool prepareOnly )
{
    SeyconService service;
    std::wstring wUser = service.escapeString ( user.c_str() );
    std::wstring wPass = service.escapeString ( password.c_str() );
    std::string domain;
    SeyconCommon::readProperty("SSOSoffidAgent", domain);
    std::string serial;
    SeyconCommon::readProperty("serialNumber", serial);
    std::wstring wSerial = service.escapeString(serial.c_str());
    std::wstring wDomain = service.escapeString ( domain.c_str());
    std::string clientIP;
    SeyconCommon::getCitrixClientIP ( clientIP );
    std::wstring wClientIP = service.escapeString ( clientIP.c_str() );
    bool repeat = false;
    do
    {
        repeat = false;
        const wchar_t *action = prepareOnly ? L"prestart" : L"start";
        SeyconResponse *resp =
            service.sendUrlMessage ( host, port,
                                     L"/passwordLogin?action=%ls&user=%ls&password=%ls&clientIP=%ls&cardSupport=%d&domain=%ls&serial=%ls",
                                     action, wUser.c_str(), wPass.c_str(), wClientIP.c_str(),
                                     ( int ) SeyconCommon::getCardSupport(), wDomain.c_str(),
									 wSerial.c_str());
        SeyconCommon::wipe ( wPass );
        if ( resp == NULL )
        {
            return SIR_RETRY;
        }
        std::string statusMsg;
        resp->getToken ( 0, statusMsg );
        SeyconCommon::debug ( "Status = %s", statusMsg.c_str() );
        if ( statusMsg == "OK" )
        {
            if ( !prepareOnly )
            {
                resp->getToken ( 1, sessionKey );
                resp->getToken ( 2, cardNumber );
                resp->getToken ( 3, cardCell );
                resp->getToken ( 4, soffidUser);
                if (soffidUser.empty())
                	soffidUser = user;
            }
            delete resp;
            return SIR_SUCCESS;
        }
    	std::string cause = resp->getToken(1);
        delete resp;
        if ( statusMsg == "EXPIRED" )
        {
            ServiceIteratorResult r = preauthChangePassword ( host, port );
            if ( r == SIR_SUCCESS )
            {
                wPass = service.escapeString ( newPassword.c_str() );
                repeat = true;
            }
            else
                return r;
        }
        else if ( ( statusMsg == "es.caib.seycon.UnknownUserException" )
                  || ( statusMsg == "es.caib.seycon.ng.exception.UnknownUserException" ) )
        {
            errorMessage = "Unknown user code";
            status = LOGIN_UNKNOWNUSER;
            return SIR_ERROR;
        }
        else if ( statusMsg == "es.caib.seycon.LogonDeniedException"
                  || statusMsg == "es.caib.seycon.ng.exception.LogonDeniedException" )
        {
            errorMessage = "Access denied: "+cause;
            status = LOGIN_DENIED;
            return SIR_ERROR;
        }
        else if ( statusMsg == "ERROR" )
        {
            errorMessage = "Incorrect password";
            status = LOGIN_DENIED;
            return SIR_ERROR;
        }
        else
            return SIR_RETRY;
    }
    while ( repeat );
    return SIR_ERROR;
}

ServiceIteratorResult SeyconSession::createKerberosSession ( const char* host,
        size_t port )
{
    return createSession ( "kerberosLogin", host, port );
}

ServiceIteratorResult SeyconSession::createPasswordSession ( const char* host,
        size_t port )
{
    return createSession ( "passwordLogin", host, port );
}

ServiceIteratorResult SeyconSession::createSession ( const char* servletName,
        const char* host, size_t port )
{
    SeyconService service;
    std::wstring cardValue;
    if ( cardNumber.size() > 0 )
    {
        cardValue = getCardValue();
        if ( cardValue.empty() )
        {
            errorMessage = "You must enter a value in cell";
            status = LOGIN_DENIED;
            return SIR_ERROR;
        }
    }
    m_daemon = new SsoDaemon();
    m_daemon->session = this;
    int daemonPort = m_daemon->startDaemon();
    time_t lastTry = 0;
    std::string action = "";
    do
    {
        SeyconResponse *resp = service.sendUrlMessage ( host, port,
                               L"/%hs?action=createSession&challengeId=%hs&cardValue=%ls&port=%d%hs",
                               servletName, sessionKey.c_str(), cardValue.c_str(), daemonPort,
                               action.c_str() );
        if ( resp == NULL )
        {
            errorMessage = "Error accessing to network";
            status = LOGIN_DENIED;
            return SIR_ERROR;
        }
        std::string tag;
        std::string admin;
        resp->getToken ( 0, tag );
        resp->getToken ( 2, sessionId );
        if ( tag == "OK" )
        {
#ifndef WIN32
            resp->getToken ( 3, admin );
            char sudoersFile [256] = "/etc/mazinger/sudoers.d/mzn-";
            strcat ( sudoersFile, user.c_str() );
            unlink ( sudoersFile );
            if ( admin == "true" )
            {
                int f = open ( sudoersFile, O_CREAT | O_WRONLY | O_TRUNC, 0440 );
                if ( f >= 0 )
                {
                    const char *header = "# File created by MAZINGER\n\n";
                    const char *tail = " ALL=(ALL) ALL\n\n";
                    write ( f, header, strlen ( header ) );
                    write ( f, user.c_str(), strlen ( user.c_str() ) );
                    write ( f, tail, strlen ( tail ) );
                    ::fchown ( f, 0, 0 );
                    ::close ( f );
                }
                else
                {
                    printf ( "Cannot create sudoers file\n" );
                }
            }
            else
            {
                printf ( "Denied sudoers %s\n", sudoersFile );
            }
#endif
            m_daemon->sessionId = sessionId;
            DEBUG
            generateMenus();
            m_bOpen = true;
            return SIR_SUCCESS;
        }
        else if ( (tag == "es.caib.sso.TooManySessionsException"  ||
        			tag == "es.caib.seycon.ng.exception.TooManySessionsException")
        		&& m_dialog != NULL )
        {
            SeyconCommon::debug ( "Last try = %ld", lastTry );
            if ( lastTry != 0 )
            {
                time_t now;
                time ( &now );
                SeyconCommon::debug ( "Now = %ld diff = %ld", now, ( long ) ( lastTry - now ) );
                if ( now > lastTry )
                {
                    m_dialog->notify (
                        MZNC_utf8tostr (
                            "Elapsed timeout to close the remote sessions" ).c_str() );
                    lastTry = 0;
                }
                else
                {
                    m_dialog->progressMessage (
                        MZNC_utf8tostr ( "Waiting to close remote session..." ).c_str() );
                    action = "&silent=true";
#ifdef WIN32
                    Sleep ( 20000 );
#else
                    sleep ( 20 );
#endif
                }
            }
            if ( lastTry == 0 )
            {
                SeyconCommon::debug ( "Asking for action" );
                std::string details = MZNC_utf8tostr(resp->getToken ( 1 ).c_str());
                DuplicateSessionAction response = m_dialog->askDuplicateSession (details.c_str() );
                switch ( response )
                {
                case dsaCancel:
                    errorMessage =  "Session canceled";
                    status = LOGIN_DENIED;
                    return SIR_ERROR;
                case dsaCloseOther:
                    action = "&force=true";
                    break;
                case dsaWait:
                    action = "&silent=true";
                    break;
                }
                time ( &lastTry );
                lastTry += 120L; // Concedir dos minuts d'espera
            }
        }
        else
        {
            errorMessage = MZNC_utf8tostr ( "Error on session startup: " );
            errorMessage.append ( MZNC_utf8tostr ( resp->getToken ( 1 ).c_str() ) );
            status = LOGIN_DENIED;
            return SIR_ERROR;
        }
    }
    while ( true );
}

ServiceIteratorResult SeyconSession::iteratePassword ( const char* hostName,
        size_t dwPort, bool prepareOnly )
{
    DEBUG
    ServiceIteratorResult sir = tryPasswordLogin ( hostName, dwPort, prepareOnly );
    DEBUG
    if ( !prepareOnly )
    {
        DEBUG
        if ( sir == SIR_SUCCESS )
        {
            DEBUG
            sir = createPasswordSession ( hostName, dwPort );
            DEBUG
        }
        if ( sir == SIR_SUCCESS )
        {
            DEBUG
            sir = launchMazinger ( hostName, dwPort, "passwordLogin" );
            DEBUG
        }
        DEBUG
    }
    DEBUG
    if ( sir == SIR_SUCCESS )
        status = LOGIN_SUCCESS;
    return sir;
}

//////////////////////////////////////////////////////////
int SeyconSession::passwordSessionPrepare ( const char* lpszUser, const wchar_t* lpszPass )
{
    errorMessage.assign ( "Can not contact authentication servers" );
    user.assign ( lpszUser );
    password.assign ( lpszPass );
    SeyconService service;
    PasswordIterator iterator ( this, true );
    status = LOGIN_ERROR;
    SeyconService ss;
    ss.iterateServers ( iterator );
    return status;
}

ServiceIteratorResult PasswordIterator::iterate ( const char* hostName, size_t dwPort )
{
    DEBUG
    return s->iteratePassword ( hostName, dwPort, prepare );
}

PasswordIterator::~PasswordIterator ()
{
}

void SeyconSession::updateConfiguration() {
	SeyconCommon::updateHostAddress();
    // Actualitzar la configuraci�
    SeyconCommon::updateConfig ( "soffid.hostname.format" );
    SeyconCommon::updateConfig ( "soffid.esso.protocol" );
    SeyconCommon::updateConfig ( "SSOServer" );
    SeyconCommon::updateConfig ( "QueryServer" );
    SeyconCommon::updateConfig ( "PreferedServers" );
    SeyconCommon::updateConfig ( "seycon.https.port" );
    SeyconCommon::updateConfig ( "seycon.https.alternate.port" );
	SeyconCommon::updateConfig ( "SSOSoffidAgent");
	SeyconCommon::updateConfig ( "EnableCloseSession");
	SeyconCommon::updateConfig ( "ForceStartupLogin");
	SeyconCommon::updateConfig ( "soffid.esso.session.keepalive");
	SeyconCommon::updateConfig ( "soffid.esso.idleTimeout");
	SeyconCommon::updateConfig ( "soffid.esso.sharedWorkstation");
	SeyconCommon::updateConfig ( "AutoSSOSystem");
	SeyconCommon::updateConfig ( "AutoSSOURL");
}

//////////////////////////////////////////////////////////
int SeyconSession::passwordSessionStartup ( const char* lpszUser, const wchar_t* lpszPass )
{
	m_kerberosLogin = false;
	m_passwordLogin = true;

	updateConfiguration();

	errorMessage.assign ( "Can not contact identification servers" );
    DEBUG
    user.assign ( lpszUser );
    password.assign ( lpszPass );
    status = LOGIN_ERROR;
    PasswordIterator iterator ( this, false );
    SeyconService ss;
    ss.iterateServers ( iterator );
    DEBUG
    if ( status == LOGIN_SUCCESS )
    {
        // Crear la sessió
        return LOGIN_SUCCESS;
    }
    else
        return status;
}

int SeyconSession::changePassword ( const wchar_t *newpass )
{
    SeyconService service;
    std::wstring wUser = service.escapeString ( user.c_str() );
    std::wstring wPass = service.escapeString ( password.c_str() );
    std::wstring wNewPass;
    if ( m_dialog == NULL )
    {
        printf ( "Error: Dialog method disabled\n" );
        return LOGIN_DENIED;
    }
    newPassword.assign ( newpass );
    bool repeat;
    do
    {
        wNewPass = service.escapeString ( newPassword.c_str() );
        repeat = false;
        SeyconResponse *resp = service.sendUrlMessage (
                                   L"/passwordLogin?action=changePass&user=%ls&password1=%ls&password2=%ls",
                                   wUser.c_str(), wPass.c_str(), wNewPass.c_str() );
        if ( resp == NULL )
        {
            errorMessage = "Network error when trying to change password";
            return LOGIN_ERROR;
        }
        std::string statusMsg;
        resp->getToken ( 0, statusMsg );
        if ( statusMsg != "OK" )
        {
            repeat = true;
            std::string msg;
            resp->getToken ( 1, msg );
            errorMessage = "Can not change password: ";
            errorMessage += msg;
            delete resp;
            return LOGIN_DENIED;
        }
        delete resp;
    }
    while ( repeat );
    return LOGIN_SUCCESS;
}
