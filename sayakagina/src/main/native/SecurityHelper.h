/*
 * SecurityHelper.h
 *
 *  Created on: 11/01/2011
 *      Author: u07286
 */

#ifndef SECURITYHELPER_H_
#define SECURITYHELPER_H_


#include <string>
#include "winwlx.h"
#include <ntsecapi.h>

typedef struct _WLX_PROFILE_V2_0  WLX_PROFILE_V2_0;


class SecurityHelper {
public:
    static bool RegisterLogonProcess(const char* logonProcessName, HANDLE* phLsa);
    static bool CallLsaLogonUser(HANDLE hLsa,
                                const wchar_t* domain,
                                const wchar_t* user,
                                const wchar_t* pass,
                                SECURITY_LOGON_TYPE logonType,
                                LUID* pLogonSessionId,
                                HANDLE* phToken,
                                MSV1_0_INTERACTIVE_PROFILE** ppProfile,
                                DWORD* pWin32Error);
    static bool GetUserSid(HANDLE htok, void* psid);
    static bool GetLogonSid(HANDLE htok, void* psid);
    static bool GetLogonSessionId(HANDLE htok, LUID* pluid);
    static bool ExtractProfilePath(wchar_t** ppProfilePath, MSV1_0_INTERACTIVE_PROFILE* pProfile);
    static bool AllocWinLogonProfile(WLX_PROFILE_V2_0** ppWinLogonProfile, const wchar_t* profilePath);
    static bool CreateProcessAsUserOnDesktop(HANDLE hToken, wchar_t* programImage, wchar_t* desktop, void* env);
    static bool ImpersonateAndGetUserName(HANDLE hToken, wchar_t* name, int cch);
    static bool IsSameUser(HANDLE hToken1, HANDLE hToken2, bool* pbIsSameUser);
    static bool IsAdmin(HANDLE hToken);
    static bool getDomain(std::wstring &domain) ;
private:
    SecurityHelper() {} // not meant to be instantiated
};

#endif /* SECURITYHELPER_H_ */
