/*
 * Utils.cpp
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */

#include "sayaka.h"
#include "Utils.h"
#include <objbase.h>
#include "Log.h"
#include <credentialprovider.h>
#include <lm.h>

Utils::Utils() {}

Utils::~Utils() {}

void Utils::bstr2str ( std::string &str, BSTR bstr)
{
	if (bstr == NULL)
	{
		str.clear ();
	}
	else
	{
		DWORD dwRequiredSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) bstr,
				SysStringLen(bstr), NULL, 0,
				NULL,NULL);
		LPSTR lpszBuffer = (LPSTR) malloc(dwRequiredSize+2);
		dwRequiredSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) bstr,
				 SysStringLen(bstr), lpszBuffer, dwRequiredSize+1,
				NULL,NULL);
		lpszBuffer[dwRequiredSize] = '\0';
		str.assign(lpszBuffer);
		free(lpszBuffer);
		SysFreeString(bstr);
	}
}


BSTR Utils::str2bstr (const char*lpsz)
{
	if (lpsz == NULL)
		return NULL;
	else
	{
		DWORD dwRequiredSize = MultiByteToWideChar(CP_UTF8, 0, lpsz,
				-1, NULL, 0);
		DWORD dwBytes = sizeof (wchar_t) * dwRequiredSize;
		LPWSTR lpwszBuffer = (LPWSTR) malloc (dwBytes+1);
		MultiByteToWideChar(CP_UTF8, 0, lpsz,
				-1, lpwszBuffer, dwBytes);
		lpwszBuffer[dwRequiredSize] = L'\0';
		BSTR bstr = SysAllocString(lpwszBuffer);
		free (lpwszBuffer);
		return bstr;
	}

}

BSTR Utils::str2bstr (const wchar_t*lpsz)
{
	if (lpsz == NULL)
		return NULL;
	else
	{
		BSTR bstr = SysAllocString(lpsz);
		return bstr;
	}

}
HRESULT Utils::FieldDescriptorCoAllocCopy(
    const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR& rcpfd,
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
    )
{
    HRESULT hr;
    DWORD cbStruct = sizeof(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR);

    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR* pcpfd =
        (CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR*)CoTaskMemAlloc(cbStruct);

    if (pcpfd)
    {
        pcpfd->dwFieldID = rcpfd.dwFieldID;
        pcpfd->cpft = rcpfd.cpft;

        if (rcpfd.pszLabel)
        {
        	hr = duplicateString(pcpfd->pszLabel, rcpfd.pszLabel);
        }
        else
        {
            pcpfd->pszLabel = NULL;
            hr = S_OK;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    if (SUCCEEDED(hr))
    {
        *ppcpfd = pcpfd;
    }
    else
    {
        CoTaskMemFree(pcpfd);
        *ppcpfd = NULL;
    }


    return hr;
}

HRESULT Utils::duplicateString (wchar_t* &szResult, const char* sz) {
	if (sz == NULL)
		return E_OUTOFMEMORY;
	else
	{
		size_t s = mbstowcs (NULL, sz, 0);
		szResult = (LPWSTR) CoTaskMemAlloc (sizeof (wchar_t)* (s+1));
		mbstowcs (szResult, sz, s+1);
		return S_OK;
	}

}

HRESULT Utils::duplicateString (wchar_t* &szResult, const wchar_t* wsz) {
	int len = wcslen (wsz);
	szResult = (wchar_t*) CoTaskMemAlloc (sizeof (wchar_t) * (1+len));
	if (szResult) {
		wcscpy (szResult, wsz);
		return S_OK;
	} else {
		return E_OUTOFMEMORY;
	}
}

void Utils::freeString (wchar_t* &wsz) {
	if (wsz != NULL) {
		memset (wsz, 0, wcslen(wsz) * sizeof(wchar_t));
		CoTaskMemFree((void*)wsz);
		wsz = NULL;
	}
}

//
// The following function is intended to be used ONLY with the Kerb*Pack functions.  It does
// no bounds-checking because its callers have precise requirements and are written to respect
// its limitations.
// You can read more about the UNICODE_STRING type at:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/secauthn/security/unicode_string.asp
//
static void _UnicodeStringPackedUnicodeStringCopy(
    const UNICODE_STRING& rus,
    PWSTR pwzBuffer,
    UNICODE_STRING* pus
    )
{
    pus->Length = rus.Length;
    pus->MaximumLength = rus.Length;
    pus->Buffer = pwzBuffer;

    CopyMemory(pus->Buffer, rus.Buffer, pus->Length);
}


//
// WinLogon and LSA consume "packed" KERB_INTERACTIVE_LOGONs.  In these, the PWSTR members of each
// UNICODE_STRING are not actually pointers but byte offsets into the overall buffer represented
// by the packed KERB_INTERACTIVE_LOGON.  For example:
//
// kil.LogonDomainName.Length = 14                             -> Length is in bytes, not characters
// kil.LogonDomainName.Buffer = sizeof(KERB_INTERACTIVE_LOGON) -> LogonDomainName begins immediately
//                                                                after the KERB_... struct in the buffer
// kil.UserName.Length = 10
// kil.UserName.Buffer = sizeof(KERB_INTERACTIVE_LOGON) + 14   -> UNICODE_STRINGS are NOT null-terminated
//
// kil.Password.Length = 16
// kil.Password.Buffer = sizeof(KERB_INTERACTIVE_LOGON) + 14 + 10
//
// THere's more information on this at:
// http://msdn.microsoft.com/msdnmag/issues/05/06/SecurityBriefs/#void
//

HRESULT Utils::KerbInteractiveLogonPack(
    const KERB_INTERACTIVE_UNLOCK_LOGON& rkiul,
    BYTE** prgb,
    DWORD* pcb
    )
{
    HRESULT hr;

    // alloc space for struct plus extra for the three strings
    DWORD cb = sizeof(rkiul) +
        rkiul.Logon.LogonDomainName.Length +
        rkiul.Logon.UserName.Length +
        rkiul.Logon.Password.Length;

    KERB_INTERACTIVE_UNLOCK_LOGON* pkiul = (KERB_INTERACTIVE_UNLOCK_LOGON*)CoTaskMemAlloc(cb);
    KERB_INTERACTIVE_LOGON* pkil = &pkiul->Logon;

    if (pkiul)
    {
    	memset (&pkiul->LogonId, 0, sizeof pkiul->LogonId);
    	const KERB_INTERACTIVE_LOGON &rkil = rkiul.Logon;
        pkil->MessageType = rkil.MessageType;

        //
        // point pbBuffer at the beginning of the extra space
        //
        BYTE* pbBuffer = (BYTE*)pkil + sizeof(KERB_INTERACTIVE_UNLOCK_LOGON);

        //
        // copy each string,
        // fix up appropriate buffer pointer to be offset,
        // advance buffer pointer over copied characters in extra space
        //
        _UnicodeStringPackedUnicodeStringCopy(rkil.LogonDomainName, (PWSTR)pbBuffer, &pkil->LogonDomainName);
        pkil->LogonDomainName.Buffer = (PWSTR)(pbBuffer - (BYTE*)pkil);
        pbBuffer += pkil->LogonDomainName.Length;

        _UnicodeStringPackedUnicodeStringCopy(rkil.UserName, (PWSTR)pbBuffer, &pkil->UserName);
        pkil->UserName.Buffer = (PWSTR)(pbBuffer - (BYTE*)pkil);
        pbBuffer += pkil->UserName.Length;

        _UnicodeStringPackedUnicodeStringCopy(rkil.Password, (PWSTR)pbBuffer, &pkil->Password);
        pkil->Password.Buffer = (PWSTR)(pbBuffer - (BYTE*)pkil);

        *prgb = (BYTE*)pkil;
        *pcb = cb;

        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


//
// This function packs the string pszSourceString in pszDestinationString
// for use with LSA functions including LsaLookupAuthenticationPackage.
//
static HRESULT LsaInitString(PSTRING pszDestinationString, PCSTR pszSourceString)
{
    USHORT usLength = strlen(pszSourceString);
	pszDestinationString->Buffer = (PCHAR)pszSourceString;
	pszDestinationString->Length = usLength;
	pszDestinationString->MaximumLength = pszDestinationString->Length+1;
	return S_OK;
}

//
// Retrieves the 'negotiate' AuthPackage from the LSA. In this case, Kerberos
// For more information on auth packages see this msdn page:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/secauthn/security/msv1_0_lm20_logon.asp
//
HRESULT Utils::RetrieveNegotiateAuthPackage(ULONG * pulAuthPackage)
{
    HRESULT hr;
    HANDLE hLsa;

    NTSTATUS status = LsaConnectUntrusted(&hLsa);
    if (SUCCEEDED(HRESULT_FROM_NT(status)))
    {

        ULONG ulAuthPackage;
        LSA_STRING lsaszKerberosName;
        LsaInitString(&lsaszKerberosName, NEGOSSP_NAME);

        status = LsaLookupAuthenticationPackage(hLsa, &lsaszKerberosName, &ulAuthPackage);
        if (SUCCEEDED(HRESULT_FROM_NT(status)))
        {
            *pulAuthPackage = ulAuthPackage;
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_NT(status);
        }
        LsaDeregisterLogonProcess(hLsa);
    }
    else
    {
        hr= HRESULT_FROM_NT(status);
    }

    return hr;
}


//
// This function copies the length of pwz and the pointer pwz into the UNICODE_STRING structure
// This function is intended for serializing a credential in GetSerialization only.
// Note that this function just makes a copy of the string pointer. It DOES NOT ALLOCATE storage!
// Be very, very sure that this is what you want, because it probably isn't outside of the
// exact GetSerialization call where the sample uses it.
//
static HRESULT UnicodeStringInitWithString(
    PCWSTR pwz,
    UNICODE_STRING* pus
    )
{
    HRESULT hr;
    if (pwz)
    {
        pus->Length = wcslen(pwz) * sizeof (WCHAR);
        pus->MaximumLength = pus->Length;
        pus->Buffer = (WCHAR*) pwz;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}



HRESULT Utils::generateKerberosInteractiveLogon(
		LPCWSTR wszUser,
		LPCWSTR wszPassword,
		LPCWSTR wszDomain,
		CREDENTIAL_PROVIDER_USAGE_SCENARIO usage,
		KERB_INTERACTIVE_UNLOCK_LOGON &kiul
    )
{
    ZeroMemory(&kiul, sizeof(kiul));

    KERB_INTERACTIVE_LOGON &kil = kiul.Logon;
    HRESULT hr;

    hr = UnicodeStringInitWithString(wszDomain, &kil.LogonDomainName);

    if (SUCCEEDED(hr))
    {
        hr = UnicodeStringInitWithString(wszUser, &kil.UserName);

        if (SUCCEEDED(hr))
        {
            hr = UnicodeStringInitWithString(wszPassword, &kil.Password);

            if (SUCCEEDED(hr))
            {
                //
                // Allocate copies of, and package, the strings in a binary blob
                //
            	if (usage == CPUS_LOGON)
            		kil.MessageType = KerbInteractiveLogon;
            	else if (usage == CPUS_UNLOCK_WORKSTATION)
            		kil.MessageType = KerbWorkstationUnlockLogon;
            	else
            		kil.MessageType = (KERB_LOGON_SUBMIT_TYPE) 0;
            }
        }
    }
    return hr;
}

bool Utils::changePassword (wchar_t *wchDomain, wchar_t *wchUser, wchar_t*wchPass, wchar_t *wchNewPassword,
		PWSTR* szStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pStatusIcon)
{
	Log l ("changePassword");

	l.info(L"Changing password on %s for %s: %s -> %s", wchDomain, wchUser,
					wchPass, wchNewPassword);
	DWORD result = NetUserChangePassword(
			wchDomain,
			wchUser,
			wchPass,
			wchNewPassword);

	if (result == NERR_Success)
	{
		l.info ("OK");
		return true;
	}
	else if (result == ERROR_ACCESS_DENIED)
	{
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, "No te permís per canviar la contrasenya");
	}
	else if (result == ERROR_INVALID_PASSWORD)
	{
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, "La contrasenya introduïda no compleix els requisits mínims");
	}
	else if (result == NERR_InvalidComputer)
	{
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, "El servidor no respon");
	}
	else if (result == NERR_NotPrimary)
	{
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, "No es pot trobar el controlador del domini");
	}
	else if (result == NERR_UserNotFound)
	{
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, "El sistema no coneix el seu codi d'usuari");
	}
	else if (result == NERR_PasswordTooShort)
	{
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, "La contrasenya no compleix els requisits de seguretat, o és una contrasenya ja utilitzada");
	}
	else
	{
		char achMessage[100];
		sprintf (achMessage, "Error desconegut %x", (unsigned int) result);
		*pStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*szStatusText, achMessage);
	}
	return false;
}

// Load messages from resources
std::string Utils::LoadResourcesString (int id)
{
	char resource[4096];	// Resource load
	std::string result;			// Message of resource (or error)

	// Check the resource availability
	if ( LoadString ( hSayakaInstance, id,  (char*) &resource,
			(sizeof(resource) / sizeof(char))) > 0)
	{
		result = resource;
	}

	else
	{
		char ach [20];
		sprintf (ach, " %d ", id);
		result = "! Unknown message !";
		result += ach;
	}

	return result;
}
