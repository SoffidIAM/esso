#include "sayaka.h"

#include "SayakaProvider.h"
#include "SayakaCredential.h"
#include "Utils.h"
#include <security.h>
#if ! defined(WIN64) && defined(__GNUC__) && false
#include <ntsecapi7.h>
#else
#include <ntsecapi.h>
#endif
#include <SayakaGuid.h>
#include <credentialprovider.h>

#include "TokenHandler.h"
#include "WaitDialog.h"
#include <string.h>
#include <stdlib.h>

# include <MZNcompat.h>

SayakaCredential::SayakaCredential (TokenHandler *pToken, CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus):
	m_log ("SayakaCredential")
{
	m_nRefCount = 0;
	m_needsChangePassword = false;
	m_bAutoLogon = false;
	m_cpus = cpus;
    m_log.info("SayakaCredential::SayakaCredential");

    m_pCredProvCredentialEvents = NULL;
    m_token = pToken;
    m_certs.clear ();
    // Copy the field descriptors for each field. This is useful if you want to vary the field
    // descriptors based on what Usage scenario the credential was created for.
    m_cpfs[SAY_TITLE] = CPFS_DISPLAY_IN_BOTH;
    m_cpfs[SAY_PIN] = CPFS_DISPLAY_IN_SELECTED_TILE;
    m_cpfs[SAY_IMAGE] = CPFS_DISPLAY_IN_BOTH;
    m_cpfs[SAY_CERT] = CPFS_HIDDEN;
    m_cpfs[SAY_SUBMIT_BUTTON] = CPFS_DISPLAY_IN_SELECTED_TILE;
    m_cpfs[SAY_CHANGE_MSG] = CPFS_HIDDEN;
    m_cpfs[SAY_CHANGE_MSG2] = CPFS_HIDDEN;
    m_cpfs[SAY_NEW_PASSWORD] = CPFS_HIDDEN;
    m_cpfs[SAY_NEW_PASSWORD2] = CPFS_HIDDEN;

    m_cpfis[SAY_TITLE] = CPFIS_NONE;
    m_cpfis[SAY_PIN] = CPFIS_FOCUSED;
    m_cpfis[SAY_IMAGE] = CPFIS_NONE;
    m_cpfis[SAY_CERT] = CPFIS_NONE;
    m_cpfis[SAY_SUBMIT_BUTTON] = CPFIS_NONE;
    m_cpfis[SAY_CHANGE_MSG] = CPFIS_NONE;
    m_cpfis[SAY_CHANGE_MSG2] = CPFIS_NONE;
    m_cpfis[SAY_NEW_PASSWORD] = CPFIS_NONE;
    m_cpfis[SAY_NEW_PASSWORD2] = CPFIS_NONE;
    // Initialize the String value of all the fields
   	Utils::duplicateString(m_rgFieldStrings[SAY_IMAGE],
   			MZNC_strtowstr(Utils::LoadResourcesString(12).c_str()).c_str());
   	Utils::duplicateString(m_rgFieldStrings[SAY_CERT], L"");
   	Utils::duplicateString(m_rgFieldStrings[SAY_PIN], L"");
    Utils::duplicateString(m_rgFieldStrings[SAY_TITLE],
    		(const char*) pToken->getTokenDescription());
    Utils::duplicateString(m_rgFieldStrings[SAY_SUBMIT_BUTTON],
    		MZNC_strtowstr(Utils::LoadResourcesString(13).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[SAY_CHANGE_MSG],
    		MZNC_strtowstr(Utils::LoadResourcesString(14).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[SAY_CHANGE_MSG2],
    		MZNC_strtowstr(Utils::LoadResourcesString(15).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[SAY_NEW_PASSWORD], L"");
    Utils::duplicateString(m_rgFieldStrings[SAY_NEW_PASSWORD2], L"");
}

HRESULT __stdcall SayakaCredential::QueryInterface(REFIID riid, void **ppObj) {
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	else if (riid == IID_ICredentialProviderCredential) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	wchar_t *lpwszClsid;
	StringFromCLSID(riid, &lpwszClsid);
	m_log.info (L"Query Interface Unknown %s", lpwszClsid);
	//
	//if control reaches here then , let the client know that
	//we do not satisfy the required interface
	//
	*ppObj = NULL;
	return E_NOINTERFACE;
}

ULONG __stdcall SayakaCredential::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall SayakaCredential::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) delete this;
	return nRefCount;
}

//LogonUI calls this in order to give us a callback in case we need to notify it of anything
HRESULT SayakaCredential::Advise(
    ICredentialProviderCredentialEvents* pcpce
    )
{
	m_log.info("Credential::Advise");
    if (m_pCredProvCredentialEvents != NULL)
    {
        m_pCredProvCredentialEvents->Release();
    }
    m_pCredProvCredentialEvents = pcpce;
    m_pCredProvCredentialEvents->AddRef();
	m_log.info("Credential::EndAdvise");
    return S_OK;
}

//LogonUI calls this to tell us to release the callback
HRESULT SayakaCredential::UnAdvise()
{
    m_log.info("Credential::UnAdvise");
    if (m_pCredProvCredentialEvents)
    {
        m_pCredProvCredentialEvents->Release();
    }
    m_pCredProvCredentialEvents = NULL;
    return S_OK;
}

//LogonUI calls this function when our tile is selected (zoomed)
//If you simply want fields to show/hide based on the selected state,
//there's no need to do anything here - you can set that up in the
//field definitions.  But if you want to do something
//more complicated, like change the contents of a field when the tile is
//selected, you would do it here.
HRESULT SayakaCredential::SetSelected(BOOL* pbAutoLogon)
{
	m_log.info("Credential::SetSelected");
//    *pbAutoLogon = true;
	*pbAutoLogon = m_bAutoLogon;
    return S_OK;
}

//Similarly to SetSelected, LogonUI calls this when your tile was selected
//and now no longer is.  The most common thing to do here (which we do below)
//is to clear out the password field.
HRESULT SayakaCredential::SetDeselected()
{
	m_needsChangePassword = true;
	m_log.info("Credential::SetDeselected");
    return S_OK;
}

//
// Get info for a particular field of a tile. Called by logonUI to get information to display the tile.
//
HRESULT SayakaCredential::GetFieldState(
    DWORD dwFieldID,
    CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis
    )
{
    HRESULT hr;

    m_log.info("Credential::GetFieldState %d", dwFieldID);

    switch (dwFieldID) {

    }
    if (dwFieldID < SAY_NUM_FIELDS && pcpfs && pcpfis)
    {
        *pcpfs = m_cpfs[dwFieldID];
        *pcpfis = m_cpfis[dwFieldID];
    	m_log.info ("Returning state %d - %d", *pcpfs, *pcpfis);
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//
// Sets ppwz to the string value of the field at the index dwFieldID
//
HRESULT SayakaCredential::GetStringValue(
    DWORD dwFieldID,
    PWSTR* ppwz
    )
{
    HRESULT hr;

    m_log.info("Credential::GetStringValue %d", dwFieldID);

    // Check to make sure dwFieldID is a legitimate index
    if (dwFieldID <  SAY_NUM_FIELDS  && ppwz)
    {
    	hr = Utils::duplicateString(*ppwz, m_rgFieldStrings[dwFieldID]);
    	m_log.info (L"Result = %s", *ppwz);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//
// Get the image to show in the user tile
//
HRESULT SayakaCredential::GetBitmapValue(
    DWORD dwFieldID,
    HBITMAP* phbmp
    )
{
    HRESULT hr;

    m_log.info("Credential::GetBitmapValue %d", dwFieldID);

    if (SAY_IMAGE == dwFieldID && phbmp)
    {
    	HBITMAP hbmp;
    	if (strncmp (m_token->getTokenDescription(), "DNI", 3) == 0)
    		hbmp = LoadBitmap(hSayakaInstance,"DNIE");
    	else
    		hbmp = LoadBitmap(hSayakaInstance,"LOGO");

        if (hbmp != NULL)
        {
        	m_log.info ("Generated bitmap");
            *phbmp = hbmp;
            hr = S_OK;
        } else {
        	m_log.warn ("Cannot load bitmap SOFFID_LOGO");
        	hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
    	m_log.info ("The bittmap not exists %d", dwFieldID);
        hr = E_INVALIDARG;
    }

    return hr;
}


//
// Sets pdwAdjacentTo to the index of the field the submit button should be
// adjacent to. We recommend that the submit button is placed next to the last
// field which the user is required to enter information in. Optional fields
// should be below the submit button.
//
HRESULT SayakaCredential::GetSubmitButtonValue(
    DWORD dwFieldID,
    DWORD* pdwAdjacentTo
    )
{
    HRESULT hr;

    m_log.info("Credential::GetSubmitButtonValue %d", dwFieldID);

    if (SAY_SUBMIT_BUTTON == dwFieldID && pdwAdjacentTo)
    {
        // pdwAdjacentTo is a pointer to the fieldID you want the submit button to appear next to.
        *pdwAdjacentTo = SAY_PIN;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//
// Sets the value of a field which can accept a string as a value.
// This is called on each keystroke when a user types into an edit field
//
HRESULT SayakaCredential::SetStringValue(
    DWORD dwFieldID,
    PCWSTR pwz
    )
{
    HRESULT hr;

    m_log.info(L"Credential::SetStringValue %d = %s", dwFieldID, pwz);
    if (dwFieldID == SAY_PIN || dwFieldID == SAY_NEW_PASSWORD || dwFieldID == SAY_NEW_PASSWORD2)
    {
    	Utils::duplicateString(m_rgFieldStrings[dwFieldID], pwz);
    }

    hr = S_OK;

    return hr;
}

//-------------
// The following methods are for logonUI to get the values of various UI elements and then communicate
// to the credential about what the user did in that field.  However, these methods are not implemented
// because our tile doesn't contain these types of UI elements
HRESULT SayakaCredential::GetCheckboxValue(
    DWORD dwFieldID,
    BOOL* pbChecked,
    PWSTR* ppwzLabel
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(pbChecked);
    UNREFERENCED_PARAMETER(ppwzLabel);

    return E_NOTIMPL;
}

HRESULT SayakaCredential::GetComboBoxValueCount(
    DWORD dwFieldID,
    DWORD* pcItems,
    DWORD* pdwSelectedItem
    )
{
	if (dwFieldID == SAY_CERT) {
		*pcItems = (DWORD) m_certs.size ();
		*pdwSelectedItem = (DWORD) m_iSelectedCert;
		return S_OK;
	} else
		return E_INVALIDARG;
}

HRESULT SayakaCredential::GetComboBoxValueAt(
    DWORD dwFieldID,
    DWORD dwItem,
    PWSTR* ppwzItem
    )
{
	if (dwFieldID == SAY_CERT) {
		CertificateHandler *pCert = m_certs.at(dwItem);
		if (pCert == NULL)
			Utils::duplicateString(*ppwzItem, "Unknown cert !!");
		else
			Utils::duplicateString(*ppwzItem, pCert->getName());
		return S_OK;
	} else
		return E_INVALIDARG;
}

HRESULT SayakaCredential::SetCheckboxValue(
    DWORD dwFieldID,
    BOOL bChecked
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(bChecked);

    return E_NOTIMPL;
}

HRESULT SayakaCredential::SetComboBoxSelectedValue(
    DWORD dwFieldId,
    DWORD dwSelectedItem
    )
{
	if (dwFieldId == SAY_CERT) {
		m_iSelectedCert = dwSelectedItem;
		return S_OK;
	} else
		return E_INVALIDARG;
}

HRESULT SayakaCredential::CommandLinkClicked(DWORD dwFieldID)
{
    UNREFERENCED_PARAMETER(dwFieldID);
    return E_NOTIMPL;
}
//------ end of methods for controls we don't have in our tile ----//



//
// Collect the username and password into a serialized credential for the correct usage scenario
// (logon/unlock is what's demonstrated in this sample).  LogonUI then passes these credentials
// back to the system to log on.
//
HRESULT SayakaCredential::GetSerialization(
    CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
    CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
    PWSTR* ppwzOptionalStatusText,
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
    )
{
	HWND hWnd;
	m_pCredProvCredentialEvents->OnCreatingWindow(&hWnd);
	WaitDialog waitDialog (hWnd);
	HRESULT hr;
	if (m_bAutoLogon)
	{
		m_bAutoLogon = false;
		hr = GenerateLoginSerialization (pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon);
	} else if (m_needsChangePassword) {
		if (wcscmp ( m_rgFieldStrings[SAY_NEW_PASSWORD], m_rgFieldStrings[SAY_NEW_PASSWORD2]) != 0) {
			Utils::duplicateString (*ppwzOptionalStatusText,
					Utils::LoadResourcesString(16).c_str());
			*pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
			*pcpsiOptionalStatusIcon = CPSI_ERROR;
			hr = S_OK;
			m_log.info("Passwords do not match");
		} else {
			m_log.info("Trying change password");
			hr = GenerateChangePasswordSerialization(pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon);
		}
	} else if (m_certs.size() == 0) {
		waitDialog.displayMessage(
				MZNC_strtowstr(Utils::LoadResourcesString(17).c_str()).c_str());
		hr = EnumCerts ();
		if (SUCCEEDED(hr)) {
			if (m_certs.size() == 1) {
				m_iSelectedCert = 0;
				waitDialog.displayMessage(
						MZNC_strtowstr(Utils::LoadResourcesString(18).c_str()).c_str());
				hr = processCurrentCertificate(pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon);
			} else {
				activateCertificateDialog();
				*pcpsiOptionalStatusIcon = CPSI_NONE;
				*pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
			}
		}
	} else {
		hr = processCurrentCertificate(pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon);
	}
	return hr;
}

HRESULT SayakaCredential::processCurrentCertificate(
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE * pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcs,
		PWSTR * ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON * pcpsiOptionalStatusIcon)
{
	HRESULT hr;
    if(ObtainCredentials(pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon)){
        if(m_cpus == CPUS_CHANGE_PASSWORD){
            activateChangePasswordDialog();
            *pcpsiOptionalStatusIcon = CPSI_NONE;
            *pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
            hr = S_OK;
            m_needsChangePassword = true;
        } else {
            hr = GenerateLoginSerialization(pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon);
        }
    }
    else
        hr = S_OK;

    return hr;
}

HRESULT SayakaCredential::EnumCerts () {
	if (m_pCredProvCredentialEvents != NULL) {
		HWND hWnd;
		m_pCredProvCredentialEvents->OnCreatingWindow(&hWnd);
		m_token->getHandler()->setParentWindow( hWnd );
	} else {
		m_token->getHandler()->setParentWindow( NULL );
	}
	size_t size = wcstombs (NULL, m_rgFieldStrings[SAY_PIN], 0);
	char *achPin = (char*) malloc (size + 1);
	wcstombs (achPin, m_rgFieldStrings[SAY_PIN], size+1);
	m_token->setPin(achPin);
	free (achPin);
	m_certs = m_token->getHandler()->enumCertificates(m_token);
	m_iSelectedCert = 0;
	return S_OK;
}


bool SayakaCredential::ObtainCredentials (
	CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
	CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
	PWSTR* ppwzOptionalStatusText,
	CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
	) {
	CertificateHandler *pCert = m_certs.at(m_iSelectedCert);
	if (pCert->obtainCredentials() ){
		return true;
	} else {
		*pcpgsr = CPGSR_NO_CREDENTIAL_FINISHED;
		*pcpsiOptionalStatusIcon = CPSI_ERROR;
		BSTR bstr = Utils::str2bstr(pCert->getErrorMessage());
		Utils::duplicateString(*ppwzOptionalStatusText, bstr);
		SysFreeString (bstr);
		return false;
	}
}


HRESULT SayakaCredential::GenerateLoginSerialization(
	CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
	CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
	PWSTR* ppwzOptionalStatusText,
	CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
	) {

	UNREFERENCED_PARAMETER(ppwzOptionalStatusText);
    UNREFERENCED_PARAMETER(pcpsiOptionalStatusIcon);

    HRESULT hr;

	CertificateHandler *pCert = m_certs.at(m_iSelectedCert);
	m_log.info (L"Credentials gots: %ls@%ls: %ls",
			pCert->getUser(),
			pCert->getDomain(),
			pCert->getPassword());
	KERB_INTERACTIVE_UNLOCK_LOGON kiul;
	ZeroMemory(&kiul, sizeof(kiul));
	*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
	hr = Utils::generateKerberosInteractiveLogon(
			pCert->getUser(),
			pCert->getPassword(),
			pCert->getDomain(),
			m_cpus,
			kiul);
	if (SUCCEEDED(hr))
	{
		m_log.info ("Got kerberos interactive logon");
		hr = Utils::KerbInteractiveLogonPack(kiul, &pcpcs->rgbSerialization, &pcpcs->cbSerialization);
		m_log.info ("Packed kerberos interactive logon");

		if (SUCCEEDED(hr))
		{
			ULONG ulAuthPackage;
			hr = Utils::RetrieveNegotiateAuthPackage(&ulAuthPackage);
			if (SUCCEEDED(hr))
			{
				m_log.info ("Serialization done");
				pcpcs->ulAuthenticationPackage = ulAuthPackage;
				pcpcs->clsidCredentialProvider = CLSID_Sayaka;

				// At this point the credential has created the serialized credential used for logon
				// By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting logonUI know
				// that we have all the information we need and it should attempt to submit the
				// serialized credential.
				*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
			}
		}
	}

    m_log.info ("Obtaining credentials");
    return hr;
}

static void packString (unsigned char* pointer, int &offset, UNICODE_STRING &str, wchar_t *sz) {
    str.Length = wcslen(sz) * sizeof (wchar_t);
    str.MaximumLength = str.Length;
    str.Buffer = (wchar_t*) offset;

    memcpy(&pointer[offset], sz, str.Length);
    offset += str.Length;
}

HRESULT SayakaCredential::GenerateChangePasswordSerialization(
	CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
	CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
	PWSTR* ppwzOptionalStatusText,
	CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
	) {

	HRESULT hr;

	CertificateHandler *pCert = m_certs.at(m_iSelectedCert);
	m_log.info (L"Credentials gots: %s@%s: %s -> %s",
			pCert->getUser(),
			pCert->getDomain(),
			pCert->getPassword(),
			m_rgFieldStrings[SAY_NEW_PASSWORD]);

    // alloc space for struct plus extra for the three strings
    DWORD cb = sizeof(KERB_CHANGEPASSWORD_REQUEST) +
    		wcslen(pCert->getUser()) * sizeof (wchar_t) +
    		wcslen(pCert->getDomain()) * sizeof (wchar_t) +
    		wcslen(pCert->getPassword()) * sizeof (wchar_t) +
    		wcslen(m_rgFieldStrings[SAY_NEW_PASSWORD]) * sizeof (wchar_t);

	KERB_CHANGEPASSWORD_REQUEST* kcpr = (KERB_CHANGEPASSWORD_REQUEST*) CoTaskMemAlloc(cb);
	if (kcpr == NULL)
		return E_OUTOFMEMORY;

	ZeroMemory(kcpr, sizeof(*kcpr));
	*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;

	kcpr->MessageType = KerbChangePasswordMessage;
	kcpr->Impersonating = false;
	unsigned char *pointer = (unsigned char*) kcpr;
	int offset = sizeof (*kcpr);

	packString (pointer, offset, kcpr->AccountName, pCert->getUser());
	packString (pointer, offset, kcpr->DomainName, pCert->getDomain());
	packString (pointer, offset, kcpr->OldPassword, pCert->getPassword());
	packString (pointer, offset, kcpr->NewPassword, m_rgFieldStrings[SAY_NEW_PASSWORD]);

	ULONG ulAuthPackage;
	hr = Utils::RetrieveNegotiateAuthPackage(&ulAuthPackage);
	if (SUCCEEDED(hr))
	{
		pcpcs->ulAuthenticationPackage = ulAuthPackage;
		pcpcs->clsidCredentialProvider = CLSID_Sayaka;
		pcpcs->cbSerialization = cb;
		pcpcs->rgbSerialization = (byte*) kcpr;

		// At this point the credential has created the serialized credential used for logon
		// By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting logonUI know
		// that we have all the information we need and it should attempt to submit the
		// serialized credential.
		*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
	}
	m_log.info ("Password change packed");

	return hr;
}

struct REPORT_RESULT_STATUS_INFO
{
    NTSTATUS ntsStatus;
    NTSTATUS ntsSubstatus;
    PWSTR     pwzMessage;
    CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

//these are currently defined in the ddk, but not the sdk
#ifndef STATUS_LOGON_FAILURE
#define STATUS_LOGON_FAILURE ((NTSTATUS)0xC000006DL)     // ntsubauth
#endif

#ifndef STATUS_ACCOUNT_RESTRICTION
#define STATUS_ACCOUNT_RESTRICTION	 ((NTSTATUS) 0xC000006EL)     // ntsubauth
#endif

#ifndef STATUS_ACCOUNT_DISABLED
#define STATUS_ACCOUNT_DISABLED		  ((NTSTATUS)0xC0000072L)     // ntsubauth
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS	((NTSTATUS) 0x000000000)     // ntsubauth
#endif

#ifndef STATUS_PASSWORD_MUST_CHANGE
#define STATUS_PASSWORD_MUST_CHANGE		((NTSTATUS)0xC0000224L)    // ntsubauth
#endif

static const int s_numStatus = 3;
static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[s_numStatus] =
{
    { STATUS_LOGON_FAILURE, STATUS_SUCCESS,
    		(wchar_t*)MZNC_strtowstr(Utils::LoadResourcesString(19).c_str()).c_str(),
    		CPSI_ERROR, },
    { STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED,
    		(wchar_t*)MZNC_strtowstr(Utils::LoadResourcesString(20).c_str()).c_str(),
    		CPSI_WARNING },
    { STATUS_PASSWORD_MUST_CHANGE, STATUS_SUCCESS,
    		(wchar_t*)MZNC_strtowstr(Utils::LoadResourcesString(21).c_str()).c_str(),
    		CPSI_WARNING },
};

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT SayakaCredential::ReportResult(
    NTSTATUS ntsStatus,
    NTSTATUS ntsSubstatus,
    PWSTR* ppwzOptionalStatusText,
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
    )
{
    *ppwzOptionalStatusText = NULL;
    *pcpsiOptionalStatusIcon = CPSI_NONE;

    DWORD dwStatusInfo = (DWORD)-1;

    m_log.info("Credential::ReportResult %d %x - %d", ntsStatus, ntsStatus, ntsSubstatus);

    // Look for a match on status and substatus.
    for (int i = 0; i < s_numStatus; i++)
    {
        if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus && s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
        {
            dwStatusInfo = i;
            break;
        }
    }

    if ((DWORD)-1 != dwStatusInfo)
    {
    	Utils::duplicateString(*ppwzOptionalStatusText, s_rgLogonStatusInfo[dwStatusInfo].pwzMessage);
        *pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
    }

    if (ntsStatus == STATUS_SUCCESS && m_needsChangePassword && m_cpus != CPUS_CHANGE_PASSWORD) {
    	m_needsChangePassword = false;
    	CertificateHandler *pCert = m_certs.at(m_iSelectedCert);
    	pCert->setPassword(m_rgFieldStrings[SAY_NEW_PASSWORD]);
    	m_bAutoLogon = true;
    } else if (ntsStatus == STATUS_SUCCESS) {
    	CertificateHandler *pCert = m_certs.at(m_iSelectedCert);
    	pCert->getHandler()->saveCredentials(pCert, pCert->getUser(), pCert->getPassword());
    }
    if (!m_needsChangePassword && ntsStatus == (NTSTATUS) STATUS_PASSWORD_MUST_CHANGE) /// Password must change
    {
        activateChangePasswordDialog();

    }

    // Since NULL is a valid value for *ppwszOptionalStatusText and *pcpsiOptionalStatusIcon
    // this function can't fail.
    return S_OK;
}

void SayakaCredential::activateChangePasswordDialog()
{
    m_needsChangePassword = true;
    m_pCredProvCredentialEvents->SetFieldState(this, SAY_PIN, CPFS_HIDDEN);
    m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SAY_PIN, CPFIS_READONLY);
    m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SAY_CERT, CPFIS_DISABLED);
    m_pCredProvCredentialEvents->SetFieldState(this, SAY_NEW_PASSWORD, CPFS_DISPLAY_IN_SELECTED_TILE);
    m_pCredProvCredentialEvents->SetFieldState(this, SAY_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
    m_pCredProvCredentialEvents->SetFieldState(this, SAY_CHANGE_MSG2, CPFS_DISPLAY_IN_SELECTED_TILE);
    m_pCredProvCredentialEvents->SetFieldState(this, SAY_NEW_PASSWORD2, CPFS_DISPLAY_IN_SELECTED_TILE);
    m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SAY_NEW_PASSWORD, CPFIS_FOCUSED);
    m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SAY_NEW_PASSWORD, CPFIS_FOCUSED);
    m_pCredProvCredentialEvents->SetFieldSubmitButton(this, SAY_SUBMIT_BUTTON, SAY_NEW_PASSWORD2);
}

void SayakaCredential::activateCertificateDialog()
{
	m_cpfis[SAY_PIN] = CPFIS_DISABLED;
	m_cpfs[SAY_CERT] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfis[SAY_CERT] = CPFIS_FOCUSED;
	m_pCredProvCredentialEvents->SetFieldState (this, SAY_PIN, CPFS_HIDDEN);
	m_pCredProvCredentialEvents->SetFieldInteractiveState (this, SAY_PIN, CPFIS_DISABLED);
	m_pCredProvCredentialEvents->SetFieldState (this, SAY_CERT, CPFS_DISPLAY_IN_SELECTED_TILE);
	m_pCredProvCredentialEvents->SetFieldInteractiveState (this, SAY_CERT, CPFIS_FOCUSED);
	for (std::vector<CertificateHandler*>::iterator it = m_certs.begin ();
			it != m_certs.end();
			it ++)
	{
		wchar_t *wsz;
		Utils::duplicateString(wsz, (*it)->getName());
		m_pCredProvCredentialEvents->AppendFieldComboBoxItem(this, SAY_CERT,wsz);
		Utils::freeString(wsz);
	}
	m_pCredProvCredentialEvents->SetFieldComboBoxSelectedItem(this, SAY_CERT, 0);
	m_pCredProvCredentialEvents->SetFieldSubmitButton(this, SAY_SUBMIT_BUTTON, SAY_CERT);
}
