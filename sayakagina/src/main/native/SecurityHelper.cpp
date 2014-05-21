// SecurityHelper.h
//
// Routines that interface with Win32 and LSA security APIs
//

#include "sayaka.h"

#include <windows.h>
#include <lm.h>
#include <security.h>
#include <ntsecapi.h>
#include <stdio.h>
#include "EnvironmentHandler.h"
#include <string>
#include "Log.h"

#include "winwlx.h"
#include "SecurityHelper.h"

#if false
typedef enum _NETSETUP_JOIN_STATUS {

    NetSetupUnknownStatus = 0,
    NetSetupUnjoined,
    NetSetupWorkgroupName,
    NetSetupDomainName
} NETSETUP_JOIN_STATUS, *PNETSETUP_JOIN_STATUS;
#endif

#ifndef STATUS_PASSWORD_EXPIRED
#define STATUS_PASSWORD_EXPIRED 0xC0000071L
#endif

extern "C" DWORD __stdcall NetGetJoinInformation(
    LPCWSTR                lpServer,
    LPWSTR                *lpNameBuffer,
    PNETSETUP_JOIN_STATUS  BufferType);



// excerpt from a DDK header file that we can't easily include here
#define LCF1 wprintf
#define LDB1 wprintf
#define LDB2 wprintf
#define LCF wprintf
#define LCF2 wprintf
#ifndef USHRT_MAX
#define USHRT_MAX 32000
#endif
#define LOOM {}

static int _stringLenInBytes(const wchar_t* s) {
    if (!s) return 0;
    return (wcslen(s)+1) * sizeof *s;
}

static void _initUnicodeString(UNICODE_STRING* target, wchar_t* source, USHORT cbMax) {
    target->Length        = cbMax - sizeof *source;
    target->MaximumLength = cbMax;
    target->Buffer        = source;
}

static MSV1_0_INTERACTIVE_LOGON* _allocLogonRequest(
    const wchar_t* domain,
    const wchar_t* user,
    const wchar_t* pass,
    DWORD* pcbRequest) {

    const DWORD cbHeader = sizeof(MSV1_0_INTERACTIVE_LOGON);
    const DWORD cbDom    = _stringLenInBytes(domain);
    const DWORD cbUser   = _stringLenInBytes(user);
    const DWORD cbPass   = _stringLenInBytes(pass);

    // sanity check string lengths
    if (cbDom > USHRT_MAX || cbUser > USHRT_MAX || cbPass > USHRT_MAX) {
        LCF(L"Input string was too long");
        return 0;
    }

    *pcbRequest = cbHeader + cbDom + cbUser + cbPass;

    MSV1_0_INTERACTIVE_LOGON* pRequest = (MSV1_0_INTERACTIVE_LOGON*)new char[*pcbRequest];
    if (!pRequest) {
        return 0;
    }

    pRequest->MessageType = MsV1_0InteractiveLogon;

    char* p = (char*)(pRequest + 1); // point past MSV1_0_INTERACTIVE_LOGON header

    wchar_t* pDom  = (wchar_t*)(p);
    wchar_t* pUser = (wchar_t*)(p + cbDom);
    wchar_t* pPass = (wchar_t*)(p + cbDom + cbUser);

    CopyMemory(pDom,  domain, cbDom);
    CopyMemory(pUser, user,   cbUser);
    CopyMemory(pPass, pass,   cbPass);

    _initUnicodeString(&pRequest->LogonDomainName, pDom,  (USHORT)cbDom);
    _initUnicodeString(&pRequest->UserName,        pUser, (USHORT)cbUser);
    _initUnicodeString(&pRequest->Password,        pPass, (USHORT)cbPass);

    return pRequest;
}

static bool _newLsaString(LSA_STRING* target, const char* source) {
    if (0 == source) return false;

    const int cch = lstrlenA(source);
    const int cchWithNullTerminator = cch + 1;

    // UNICODE_STRINGs have a size limit
    if (cchWithNullTerminator * sizeof(*source) > USHRT_MAX) return FALSE;

    char* newStr = new char[cchWithNullTerminator];
    if (!newStr) {
        LOOM;
        return false;
    }

    CopyMemory(newStr, source, cchWithNullTerminator * sizeof *newStr);

    target->Length        = (USHORT)cch                   * sizeof *newStr;
    target->MaximumLength = (USHORT)cchWithNullTerminator * sizeof *newStr;
    target->Buffer        = newStr;

    return true;
}

static void _deleteLsaString(LSA_STRING* target) {
    delete target->Buffer;
    target->Buffer = 0;
}

bool SecurityHelper::RegisterLogonProcess(const char* logonProcessName, HANDLE* phLsa) {
    *phLsa = 0;

    LSA_STRING name;
    if (!_newLsaString(&name, logonProcessName)) return false;

    LSA_OPERATIONAL_MODE unused;
    NTSTATUS status = LsaRegisterLogonProcess(&name, phLsa, &unused);

    _deleteLsaString(&name);

    if (status) {
        *phLsa = 0;
        LCF1(L"LsaRegisterLogonProcess failed: %d", LsaNtStatusToWinError(status));
        return false;
    }
    return true;
}

bool SecurityHelper::CallLsaLogonUser(
    HANDLE hLsa,
    const wchar_t* domain,
    const wchar_t* user,
    const wchar_t* pass,
    SECURITY_LOGON_TYPE logonType,
    LUID* pLogonSessionId,
    HANDLE* phToken,
    MSV1_0_INTERACTIVE_PROFILE** ppProfile,
    DWORD* pWin32Error) {

    bool result      = false;
    DWORD win32Error = 0;
    *phToken         = 0;

    LUID ignoredLogonSessionId;

    // optional arguments
    if (ppProfile)        *ppProfile   = 0;
    if (pWin32Error)      *pWin32Error = 0;
    if (!pLogonSessionId) pLogonSessionId = &ignoredLogonSessionId;

    LSA_STRING logonProcessName            = { 0 };
    TOKEN_SOURCE sourceContext             = {{ 's', 'a', 'y', 'a', 'k', 'a' }};
    MSV1_0_INTERACTIVE_PROFILE* pProfile = 0;
    ULONG cbProfile = 0;
    QUOTA_LIMITS quotaLimits;
    NTSTATUS substatus;
    DWORD cbLogonRequest;
    NTSTATUS status;

    MSV1_0_INTERACTIVE_LOGON* pLogonRequest =
        _allocLogonRequest(domain, user, pass, &cbLogonRequest);
    if (!pLogonRequest) {
        win32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (!_newLsaString(&logonProcessName, "Winlogon")) {
        win32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // LsaLogonUser - the function from hell
    status = LsaLogonUser(
        hLsa,
        &logonProcessName,  // we use our logon process name for the "origin name"
        logonType,
        LOGON32_PROVIDER_DEFAULT, // we use MSV1_0 or Kerb, whichever is supported
        pLogonRequest,
        cbLogonRequest,
        0,                  // we do not add any group SIDs
        &sourceContext,
        (void**)&pProfile,  // caller must free this via LsaFreeReturnBuffer
        &cbProfile,
        pLogonSessionId,
        phToken,
        &quotaLimits,       // we ignore this, but must pass in anyway
        &substatus);

    if (status) {
        win32Error = LsaNtStatusToWinError(status);

        if (ERROR_ACCOUNT_RESTRICTION == win32Error && STATUS_PASSWORD_EXPIRED == (DWORD) substatus) {
            win32Error = ERROR_PASSWORD_EXPIRED;
        }

        *phToken = 0;
        pProfile = 0;

        goto cleanup;
    }

    if (ppProfile) {
        *ppProfile = (MSV1_0_INTERACTIVE_PROFILE*)pProfile;
        pProfile = 0;
    }
    result = true;

cleanup:
    // if caller cares about the details, pass them on
    if (pWin32Error) *pWin32Error = win32Error;

    if (pLogonRequest) {
        memset(pLogonRequest, 0, cbLogonRequest);
        delete pLogonRequest;
    }
    if (pProfile) LsaFreeReturnBuffer(pProfile);
    _deleteLsaString(&logonProcessName);

    return result;
}


bool SecurityHelper::GetUserSid(HANDLE htok, void*psid) {
    DWORD cb;
    GetTokenInformation(htok, TokenUser, 0, 0, &cb);
    TOKEN_USER* ptg = (TOKEN_USER*)LocalAlloc(LMEM_FIXED, cb);
    if (!ptg) {
        LOOM;
        return false;
    }

    bool success = false;
    if (GetTokenInformation(htok, TokenUser, ptg, cb, &cb)) {

        // search for the logon SID
    	LCF1 (L"Got token\n");
        const DWORD cb = GetLengthSid(ptg->User.Sid);
        if (CopySid(cb, psid, ptg->User.Sid)) {
            success = true;
        } else {
            LCF1(L"CopySid failed: %d\n", GetLastError());
        }
    }
    else LCF1(L"GetTokenInformation(TokenGroups) failed: %d", GetLastError());

    LocalFree(ptg);

    return success;
}

bool SecurityHelper::GetLogonSid(HANDLE htok, void* psid) {
    DWORD cb;
    GetTokenInformation(htok, TokenGroups, 0, 0, &cb);
    TOKEN_GROUPS* ptg = (TOKEN_GROUPS*)LocalAlloc(LMEM_FIXED, cb);
    if (!ptg) {
        LOOM;
        return false;
    }

    bool success = false;
    if (GetTokenInformation(htok, TokenGroups, ptg, cb, &cb)) {
    	DWORD i;

        // search for the logon SID
    	LCF1 (L"Got %d attributes\n", ptg->GroupCount);
        for (i = 0; i < ptg->GroupCount; ++i) {
        	LCF1 (L"Got attribute %d type %x\n", i, ptg->Groups[i].Attributes);
            if (ptg->Groups[i].Attributes & SE_GROUP_LOGON_ID) {
                void* logonSid = ptg->Groups[i].Sid;
                const DWORD cb = GetLengthSid(logonSid);
//                if (cbMax < cb) return false; // sanity check caller's buffer size
                if (!CopySid(cb, psid, logonSid)) {
                    LCF1(L"CopySid failed: %d\n", GetLastError());
                    break;
                }
                success = true;
                break;
            }
        }
        if (i == ptg->GroupCount) {
            LCF(L"Failed to find a logon SID in the user's access token!\n");
            success = GetUserSid (htok, psid);
        }
    }
    else LCF1(L"GetTokenInformation(TokenGroups) failed: %d", GetLastError());

    LocalFree(ptg);

    return success;
}

bool SecurityHelper::GetLogonSessionId(HANDLE htok, LUID* pluid) {
    TOKEN_STATISTICS stats;
    DWORD cb = sizeof stats;
    if (GetTokenInformation(htok, TokenStatistics, &stats, cb, &cb)) {
        *pluid = stats.AuthenticationId;
        return true;
    }
    else {
        LCF1(L"GetTokenInformation(TokenStatistics) failed: %d", GetLastError());
        return false;
    }
}

// caller must free *ppProfilePath using LocalFree
bool SecurityHelper::ExtractProfilePath(wchar_t** ppProfilePath, MSV1_0_INTERACTIVE_PROFILE* pProfile) {
    *ppProfilePath = 0;
    if (0 == pProfile->ProfilePath.Length) {
        // no profile path was specified, so return a null pointer to WinLogon
        // to indicate that *it* should figure out the appropriate path
        return true;
    }
    bool result = false;

    const int cch = pProfile->ProfilePath.Length / sizeof(wchar_t);
    wchar_t* profilePath = (wchar_t*)LocalAlloc(LMEM_FIXED, sizeof(wchar_t) * (cch + 1)); // I never assume a UNICODE_STRING is null terminated
    if (profilePath) {
        // copy the string data and manually null terminate it
        CopyMemory(profilePath, pProfile->ProfilePath.Buffer, pProfile->ProfilePath.Length);
        profilePath[cch] = L'\0';
        *ppProfilePath = profilePath;
        result = true;
    }
    else LOOM;

    return result;
}

bool SecurityHelper::AllocWinLogonProfile(WLX_PROFILE_V2_0** ppWinLogonProfile, const wchar_t* profilePath) {

    *ppWinLogonProfile = 0;
    bool result = false;

    // must use LocalAlloc for this - WinLogon will use LocalFree
    WLX_PROFILE_V2_0* profile = (WLX_PROFILE_V2_0*)LocalAlloc(LMEM_FIXED, sizeof(WLX_PROFILE_V2_0));
    if (profile) {
        profile->dwType = WLX_PROFILE_TYPE_V2_0;
        profile->pszEnvironment = NULL;
        profile->pszNetworkDefaultUserProfile = NULL;
        profile->pszPolicy = NULL;
        profile->pszProfile = NULL;
        profile->pszServerName = NULL;
        if (profilePath == NULL) {
			*ppWinLogonProfile = profile;
			result = true;
        } else {
			const int cch = wcslen(profilePath) + 1;

			wchar_t* newProfilePath = (wchar_t*)LocalAlloc(LMEM_FIXED, cch * sizeof *newProfilePath);
			if (newProfilePath) {
				// copy the string data and manually null terminate it
				CopyMemory(newProfilePath, profilePath, cch * sizeof *newProfilePath);

				profile->pszProfile = newProfilePath;
				*ppWinLogonProfile = profile;

				result = true;
			}
        }
    }

    return result;
}

bool SecurityHelper::CreateProcessAsUserOnDesktop(HANDLE hToken, wchar_t* programImage, wchar_t* desktop, void* env) {
    // impersonate the user to ensure that they are allowed
    // to execute the program in the first place
    if (!ImpersonateLoggedOnUser(hToken)) {
        LCF1(L"ImpersonateLoggedOnUser failed: %d", GetLastError());
        return false;
    }

    STARTUPINFOW si = { sizeof si, 0, desktop };
    PROCESS_INFORMATION pi;
    if (!CreateProcessAsUserW(hToken, programImage, programImage, 0, 0, FALSE,
                             CREATE_UNICODE_ENVIRONMENT, env, 0, &si, &pi)) {
        RevertToSelf();
        LCF2(L"CreateProcessAsUser failed for image %s with error code %d", programImage, GetLastError());
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    RevertToSelf();

    LDB1(L"Successfully launched %s", programImage);
    return true;
}

bool SecurityHelper::ImpersonateAndGetUserName(HANDLE hToken, wchar_t* name, int cch) {
    bool result = false;
    if (ImpersonateLoggedOnUser(hToken)) {
        DWORD cchName = cch;
        if (GetUserNameW(name, &cchName))
             result = true;
        else LCF1(L"GetUserName failed: %d", GetLastError());
        RevertToSelf();
    }
    else LCF1(L"ImpersonateLoggedOnUser failed: %d", GetLastError());

    return result;
}

// checks user SID in both tokens for equality
bool SecurityHelper::IsSameUser(HANDLE hToken1, HANDLE hToken2, bool* pbIsSameUser) {
    *pbIsSameUser = false;
    bool result = false;

    const DWORD bufSize = sizeof(TOKEN_USER) + 68; // SECURITY_MAX_SID_SIZE;
    char buf1[bufSize];
    char buf2[bufSize];

    DWORD cb;
    if (GetTokenInformation(hToken1, TokenUser, buf1, bufSize, &cb) &&
        GetTokenInformation(hToken2, TokenUser, buf2, bufSize, &cb)) {
        *pbIsSameUser = EqualSid(((TOKEN_USER*)buf1)->User.Sid, ((TOKEN_USER*)buf2)->User.Sid) ? true : false;
        result = true;
    }
    else LCF1(L"GetTokenInformation failed: %d", GetLastError());

    return result;
}

static void* _administratorsAlias() {
    const int subAuthCount = 2;
    static char sid[sizeof(SID) + subAuthCount * sizeof(DWORD)];

    SID* psid = (SID*)sid;
    if (0 == psid->Revision) {
        // first time called, initialize the sid
        psid->IdentifierAuthority.Value[5] = 5; // NT Authority
        psid->SubAuthorityCount = subAuthCount;
        psid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
        psid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
        psid->Revision = 1;
    }
    return sid;
}

// checks for presence of local admin group (must have TOKEN_DUPLICATE perms on hToken)
bool SecurityHelper::IsAdmin(HANDLE hToken) {
    // we can pretty much assume all tokens will be primary tokens in this application
    // and CheckTokenMembership requires an impersonation token (which is really annoying)
    // so we'll just duplicate any token we get into an impersonation token before continuing...
    bool isAdmin = false;
    HANDLE hImpToken;
    if (DuplicateTokenEx(hToken, TOKEN_QUERY, 0, SecurityIdentification, TokenImpersonation, &hImpToken)) {
        BOOL isMember;
        if (CheckTokenMembership(hImpToken, _administratorsAlias(), &isMember) && isMember) {
            isAdmin = true;
        }
        else LCF1(L"CheckTokenMembership failed: %d", GetLastError());

        CloseHandle(hImpToken);
    }
    else LCF1(L"DuplicateTokenEx failed: %d", GetLastError());

    return isAdmin;
}

bool SecurityHelper::getDomain(std::wstring &domain) {
	bool ok;
	LPWSTR buffer = NULL;
	NETSETUP_JOIN_STATUS dwJoinStatus = NetSetupUnknownStatus;
	NetGetJoinInformation (NULL, &buffer, &dwJoinStatus);
	if (dwJoinStatus == NetSetupDomainName)
	{
		domain.assign(buffer);
		ok = true;
	}
	else {
		domain.clear ();
		ok = false;
	}
	if (buffer != NULL)
		NetApiBufferFree (buffer);

	return ok;
}
