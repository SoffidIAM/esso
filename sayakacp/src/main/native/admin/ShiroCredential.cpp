#include "sayaka.h"

#include "ShiroCredential.h"
#include "Utils.h"
#include <security.h>
#if ! defined(WIN64) && defined(__GNUC__) && false
#include <ntsecapi7.h>
#else
#include <ntsecapi.h>
#endif
#include <SayakaGuid.h>
#include <credentialprovider.h>

#include "WaitDialog.h"
#include <string.h>
#include <stdlib.h>
#include "LocalAdminHandler.h"
#include <MZNcompat.h>
#include <ssoclient.h>

ShiroCredential::ShiroCredential () :
		m_log("ShiroCredential")
{
	m_nRefCount = 0;
	m_log.info("ShiroCredential::ShiroCredential");

	m_pCredProvCredentialEvents = NULL;

	// Copy the field descriptors for each field. This is useful if you want to vary the field
	// descriptors based on what Usage scenario the credential was created for.
	m_cpfs[SHI_TITLE] = CPFS_DISPLAY_IN_DESELECTED_TILE;
	m_cpfs[SHI_USER] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfs[SHI_PASSWORD] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfs[SHI_IMAGE] = CPFS_DISPLAY_IN_BOTH;
	m_cpfs[SHI_SUBMIT_BUTTON] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfs[SHI_MESSAGE] = CPFS_HIDDEN;

	m_cpfis[SHI_TITLE] = CPFIS_NONE;
	m_cpfis[SHI_USER] = CPFIS_FOCUSED;
	m_cpfis[SHI_PASSWORD] = CPFIS_FOCUSED;
	m_cpfis[SHI_IMAGE] = CPFIS_NONE;
	m_cpfis[SHI_SUBMIT_BUTTON] = CPFIS_NONE;
	m_cpfis[SHI_MESSAGE] = CPFIS_NONE;

	// Initialize the String value of all the fields
	Utils::duplicateString(m_rgFieldStrings[SHI_IMAGE],
			MZNC_strtowstr(Utils::LoadResourcesString(31).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SHI_TITLE],
			MZNC_strtowstr(Utils::LoadResourcesString(54).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SHI_USER], L"");
	Utils::duplicateString(m_rgFieldStrings[SHI_PASSWORD], L"");
	Utils::duplicateString(m_rgFieldStrings[SHI_SUBMIT_BUTTON],
			MZNC_strtowstr(Utils::LoadResourcesString(33).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SHI_MESSAGE],
			MZNC_strtowstr(Utils::LoadResourcesString(34).c_str()).c_str());
}

HRESULT __stdcall ShiroCredential::QueryInterface (REFIID riid, void **ppObj)
{
	if (riid == IID_IUnknown)
	{
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	else if (riid == IID_ICredentialProviderCredential)
	{
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

ULONG __stdcall ShiroCredential::AddRef ()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG __stdcall ShiroCredential::Release ()
{
	long nRefCount = 0;
	nRefCount = InterlockedDecrement(&m_nRefCount);
	if (nRefCount == 0)
		delete this;
	return nRefCount;
}

//LogonUI calls this in order to give us a callback in case we need to notify it of anything
HRESULT ShiroCredential::Advise (ICredentialProviderCredentialEvents* pcpce)
{
	m_log.info("Credential::Advise");
	if (m_pCredProvCredentialEvents != NULL)
	{
		m_pCredProvCredentialEvents->Release();
	}
	m_pCredProvCredentialEvents = pcpce;
	m_pCredProvCredentialEvents->AddRef();
	m_log.info("Credential::EndAdvise");
	return S_OK ;
}

//LogonUI calls this to tell us to release the callback
HRESULT ShiroCredential::UnAdvise ()
{
	m_log.info("Credential::UnAdvise");
	if (m_pCredProvCredentialEvents)
	{
		m_pCredProvCredentialEvents->Release();
	}
	m_pCredProvCredentialEvents = NULL;
	return S_OK ;
}

//LogonUI calls this function when our tile is selected (zoomed)
//If you simply want fields to show/hide based on the selected state,
//there's no need to do anything here - you can set that up in the
//field definitions.  But if you want to do something
//more complicated, like change the contents of a field when the tile is
//selected, you would do it here.
HRESULT ShiroCredential::SetSelected (BOOL* pbAutoLogon)
{
	m_log.info("Credential::SetSelected");
	*pbAutoLogon = false;
	return S_OK ;
}

//Similarly to SetSelected, LogonUI calls this when your tile was selected
//and now no longer is.  The most common thing to do here (which we do below)
//is to clear out the password field.
HRESULT ShiroCredential::SetDeselected ()
{
	m_log.info("Credential::SetDeselected");
	return S_OK ;
}

//
// Get info for a particular field of a tile. Called by logonUI to get information to display the tile.
//
HRESULT ShiroCredential::GetFieldState (DWORD dwFieldID,
		CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
		CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis)
{
	HRESULT hr;

	m_log.info("Credential::GetFieldState %d", dwFieldID);

	switch (dwFieldID)
	{

	}
	if (dwFieldID < SHI_NUM_FIELDS && pcpfs && pcpfis)
	{
		*pcpfs = m_cpfs[dwFieldID];
		*pcpfis = m_cpfis[dwFieldID];
		m_log.info("Returning state %d - %d", *pcpfs, *pcpfis);
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
HRESULT ShiroCredential::GetStringValue (DWORD dwFieldID, PWSTR* ppwz)
{
	HRESULT hr;

	m_log.info("Credential::GetStringValue %d", dwFieldID);

	// Check to make sure dwFieldID is a legitimate index
	if (dwFieldID < SHI_NUM_FIELDS && ppwz)
	{
		hr = Utils::duplicateString(*ppwz, m_rgFieldStrings[dwFieldID]);
		m_log.info(L"Result = %s", *ppwz);
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
HRESULT ShiroCredential::GetBitmapValue (DWORD dwFieldID, HBITMAP* phbmp)
{
	HRESULT hr;

	m_log.info("Credential::GetBitmapValue %d", dwFieldID);

	if (SHI_IMAGE == dwFieldID && phbmp)
	{
		HBITMAP hbmp;
		hbmp = LoadBitmap(hSayakaInstance, "ADMIN");

		if (hbmp != NULL)
		{
			m_log.info("Generated bitmap");
			*phbmp = hbmp;
			hr = S_OK;
		}
		else
		{
			m_log.warn("Cannot load bitmap LOGO");
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		m_log.info("No existe el bitmap %d", dwFieldID);
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
HRESULT ShiroCredential::GetSubmitButtonValue (DWORD dwFieldID,
		DWORD* pdwAdjacentTo)
{
	HRESULT hr;

	m_log.info("Credential::GetSubmitButtonValue %d", dwFieldID);

	if (SHI_SUBMIT_BUTTON == dwFieldID && pdwAdjacentTo)
	{
		// pdwAdjacentTo is a pointer to the fieldID you want the submit button to appear next to.
		*pdwAdjacentTo = SHI_PASSWORD;
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
HRESULT ShiroCredential::SetStringValue (DWORD dwFieldID, PCWSTR pwz)
{
	HRESULT hr;

	m_log.info(L"Credential::SetStringValue %d = %s", dwFieldID, pwz);
	if (dwFieldID == SHI_USER || dwFieldID == SHI_PASSWORD)
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
HRESULT ShiroCredential::GetCheckboxValue (DWORD dwFieldID, BOOL* pbChecked,
		PWSTR* ppwzLabel)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(pbChecked);
	UNREFERENCED_PARAMETER(ppwzLabel);

	return E_NOTIMPL ;
}

HRESULT ShiroCredential::GetComboBoxValueCount (DWORD dwFieldID, DWORD* pcItems,
		DWORD* pdwSelectedItem)
{
	return E_INVALIDARG ;
}

HRESULT ShiroCredential::GetComboBoxValueAt (DWORD dwFieldID, DWORD dwItem,
		PWSTR* ppwzItem)
{
	return E_INVALIDARG ;
}

HRESULT ShiroCredential::SetCheckboxValue (DWORD dwFieldID, BOOL bChecked)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(bChecked);

	return E_NOTIMPL ;
}

HRESULT ShiroCredential::SetComboBoxSelectedValue (DWORD dwFieldId,
		DWORD dwSelectedItem)
{
	return E_INVALIDARG ;
}

HRESULT ShiroCredential::CommandLinkClicked (DWORD dwFieldID)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	return E_NOTIMPL ;
}
//------ end of methods for controls we don't have in our tile ----//

//
// Collect the username and password into a serialized credential for the correct usage scenario
// (logon/unlock is what's demonstrated in this sample).  LogonUI then passes these credentials
// back to the system to log on.
//
HRESULT ShiroCredential::GetSerialization (
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	HRESULT hr;

	displayMessage();
	if (ObtainCredentials(pcpgsr, pcpcs, ppwzOptionalStatusText,
			pcpsiOptionalStatusIcon))
	{
		hr = GenerateLoginSerialization(pcpgsr, pcpcs, ppwzOptionalStatusText,
				pcpsiOptionalStatusIcon);
	}
	else
	{
		*pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
		hr = S_OK;
	}

	hideMessage();
	return hr;
}

bool ShiroCredential::ObtainCredentials (
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{

	std::wstring szUser;
	std::wstring szPassword;
	szUser.assign(m_rgFieldStrings[SHI_USER]);
	szPassword.assign(m_rgFieldStrings[SHI_PASSWORD]);

	if (handler.getAdminCredentials(szUser, szPassword))
	{
		return true;
	}
	else
	{
		*pcpgsr = CPGSR_NO_CREDENTIAL_FINISHED;
		*pcpsiOptionalStatusIcon = CPSI_ERROR;
		Utils::duplicateString(*ppwzOptionalStatusText,
				handler.szErrorMessage.c_str());
		m_log.info("Status Error = %s", handler.szErrorMessage.c_str());
		return false;
	}
}

HRESULT ShiroCredential::GenerateLoginSerialization (
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{

	UNREFERENCED_PARAMETER(ppwzOptionalStatusText);
	UNREFERENCED_PARAMETER(pcpsiOptionalStatusIcon);

	HRESULT hr;

	KERB_INTERACTIVE_UNLOCK_LOGON kiul;
	ZeroMemory(&kiul, sizeof(kiul));
	*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
	hr = Utils::generateKerberosInteractiveLogon(handler.szUser.c_str(),
			handler.szPassword.c_str(), handler.szHostName.c_str(), CPUS_LOGON,
			kiul);
	if (SUCCEEDED(hr))
	{
		hr = Utils::KerbInteractiveLogonPack(kiul, &pcpcs->rgbSerialization,
				&pcpcs->cbSerialization);

		if (SUCCEEDED(hr))
		{
			ULONG ulAuthPackage;
			hr = Utils::RetrieveNegotiateAuthPackage(&ulAuthPackage);
			if (SUCCEEDED(hr))
			{
				pcpcs->ulAuthenticationPackage = ulAuthPackage;
				pcpcs->clsidCredentialProvider = CLSID_ShiroKabuto;

				// At this point the credential has created the serialized credential used for logon
				// By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting logonUI know
				// that we have all the information we need and it should attempt to submit the
				// serialized credential.
				*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
			}
		}
	}

	return hr;
}

struct REPORT_RESULT_STATUS_INFO
{
		NTSTATUS ntsStatus;
		NTSTATUS ntsSubstatus;
		PWSTR pwzMessage;
		CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

//these are currently defined in the ddk, but not the sdk
#ifndef STATUS_LOGON_FAILURE
#define STATUS_LOGON_FAILURE             ((NTSTATUS)0xC000006DL)     // ntsubauth
#endif
#ifndef STATUS_ACCOUNT_RESTRICTION
#define STATUS_ACCOUNT_RESTRICTION       ((NTSTATUS)0xC000006EL)     // ntsubauth
#endif
#ifndef STATUS_ACCOUNT_DISABLED
#define STATUS_ACCOUNT_DISABLED          ((NTSTATUS)0xC0000072L)     // ntsubauth
#endif
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   ((NTSTATUS)0x000000000)     // ntsubauth
#endif
#ifndef STATUS_PASSWORD_MUST_CHANGE
#define STATUS_PASSWORD_MUST_CHANGE      ((NTSTATUS)0xC0000224L)    // ntsubauth
#endif

static const int s_numStatus = 3;
static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[s_numStatus] = { {
		STATUS_LOGON_FAILURE, STATUS_SUCCESS,
		(wchar_t*) L"Contrasenya incorrecta", CPSI_ERROR, }, {
		STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED,
		(wchar_t*) L"El compte est\u00e0 cancel\u00b7lat", CPSI_WARNING }, {
		STATUS_PASSWORD_MUST_CHANGE, STATUS_SUCCESS,
		(wchar_t*) L"La contrasenya ha caducat.", CPSI_WARNING }, };

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT ShiroCredential::ReportResult (NTSTATUS ntsStatus, NTSTATUS ntsSubstatus,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	*ppwzOptionalStatusText = NULL;
	*pcpsiOptionalStatusIcon = CPSI_NONE;

	DWORD dwStatusInfo = (DWORD) -1;

	m_log.info("Credential::ReportResult %d %x - %d", ntsStatus, ntsStatus,
			ntsSubstatus);

	// Look for a match on status and substatus.
	for (int i = 0; i < s_numStatus; i++)
	{
		if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus
				&& s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
		{
			dwStatusInfo = i;
			break;
		}
	}

	if ((DWORD) -1 != dwStatusInfo)
	{
		Utils::duplicateString(*ppwzOptionalStatusText,
				s_rgLogonStatusInfo[dwStatusInfo].pwzMessage);
		*pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
	}

	// Since NULL is a valid value for *ppwszOptionalStatusText and *pcpsiOptionalStatusIcon
	// this function can't fail.
	return S_OK ;
}

void ShiroCredential::displayMessage ()
{
	m_pCredProvCredentialEvents->SetFieldState(this, SHI_USER, CPFS_HIDDEN);
	m_pCredProvCredentialEvents->SetFieldState(this, SHI_PASSWORD, CPFS_HIDDEN);
	m_pCredProvCredentialEvents->SetFieldState(this, SHI_MESSAGE,
			CPFS_DISPLAY_IN_SELECTED_TILE);
}

void ShiroCredential::hideMessage ()
{
	m_pCredProvCredentialEvents->SetFieldState(this, SHI_USER,
			CPFS_DISPLAY_IN_SELECTED_TILE);
	m_pCredProvCredentialEvents->SetFieldState(this, SHI_PASSWORD,
			CPFS_DISPLAY_IN_SELECTED_TILE);
	m_pCredProvCredentialEvents->SetFieldState(this, SHI_MESSAGE, CPFS_HIDDEN);
}
