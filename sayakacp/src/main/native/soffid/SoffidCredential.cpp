#include "sayaka.h"

#include "SoffidProvider.h"
#include "SoffidCredential.h"
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
#include "SoffidDirHandler.h"
#include <string.h>
#include <stdlib.h>
#include <MZNcompat.h>
#include <ssoclient.h>

SoffidCredential::SoffidCredential () :
		m_log("SoffidCredential")
{
	m_nRefCount = 0;
	m_log.info("SoffidCredential::SoffidCredential");

	m_pCredProvCredentialEvents = NULL;

	// Copy the field descriptors for each field. This is useful if you want to vary the field
	// descriptors based on what Usage scenario the credential was created for.
	m_cpfs[SOF_TITLE] = CPFS_DISPLAY_IN_DESELECTED_TILE;
	m_cpfs[SOF_USER] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfs[SOF_PASSWORD] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfs[SOF_IMAGE] = CPFS_DISPLAY_IN_BOTH;
	m_cpfs[SOF_SUBMIT_BUTTON] = CPFS_DISPLAY_IN_SELECTED_TILE;
	m_cpfs[SOF_MESSAGE] = CPFS_HIDDEN;
	m_cpfs[SOF_WARNING] = CPFS_HIDDEN;
	m_cpfs[SOF_NEW_PASSWORD1] = CPFS_HIDDEN;
	m_cpfs[SOF_NEW_PASSWORD2] = CPFS_HIDDEN;

	m_cpfis[SOF_TITLE] = CPFIS_NONE;
	m_cpfis[SOF_USER] = CPFIS_FOCUSED;
	m_cpfis[SOF_PASSWORD] = CPFIS_NONE;
	m_cpfis[SOF_IMAGE] = CPFIS_NONE;
	m_cpfis[SOF_SUBMIT_BUTTON] = CPFIS_NONE;
	m_cpfis[SOF_MESSAGE] = CPFIS_NONE;
	m_cpfis[SOF_WARNING] = CPFIS_NONE;
	m_cpfis[SOF_NEW_PASSWORD1] = CPFIS_FOCUSED;
	m_cpfis[SOF_NEW_PASSWORD2] = CPFIS_FOCUSED;

	// Initialize the String value of all the fields
	Utils::duplicateString(m_rgFieldStrings[SOF_IMAGE],
			MZNC_strtowstr(Utils::LoadResourcesString(31).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SOF_TITLE],
			MZNC_strtowstr(Utils::LoadResourcesString(49).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SOF_USER], L"");
	Utils::duplicateString(m_rgFieldStrings[SOF_PASSWORD], L"");
	Utils::duplicateString(m_rgFieldStrings[SOF_NEW_PASSWORD1], L"");
	Utils::duplicateString(m_rgFieldStrings[SOF_NEW_PASSWORD2], L"");
	Utils::duplicateString(m_rgFieldStrings[SOF_SUBMIT_BUTTON],
			MZNC_strtowstr(Utils::LoadResourcesString(33).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SOF_MESSAGE],
			MZNC_strtowstr(Utils::LoadResourcesString(34).c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SOF_WARNING], L"");
}

HRESULT __stdcall SoffidCredential::QueryInterface (REFIID riid, void **ppObj)
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
	else if (riid == IID_IConnectableCredentialProviderCredential)
	{
		m_log.info (L"Query Interface connectable credential provider");
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	wchar_t *lpwszClsid;
	StringFromCLSID(riid, &lpwszClsid);
	m_log.info (L"Query Interface Unknown %ls", lpwszClsid);
	//
	//if control reaches here then , let the client know that
	//we do not satisfy the required interface
	//
		*ppObj = NULL;
		return E_NOINTERFACE;
	}

ULONG __stdcall SoffidCredential::AddRef ()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG __stdcall SoffidCredential::Release ()
{
	long nRefCount = 0;
	nRefCount = InterlockedDecrement(&m_nRefCount);
	if (nRefCount == 0)
		delete this;
	return nRefCount;
}

//LogonUI calls this in order to give us a callback in case we need to notify it of anything
HRESULT SoffidCredential::Advise (ICredentialProviderCredentialEvents* pcpce)
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
HRESULT SoffidCredential::UnAdvise ()
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
HRESULT SoffidCredential::SetSelected (BOOL* pbAutoLogon)
{
	std::string lastUser;

	SeyconCommon::readProperty("lastSoffidUser", lastUser);

	m_log.info("Credential::SetSelected");
	*pbAutoLogon = false;
	Utils::duplicateString(m_rgFieldStrings[SOF_USER], MZNC_utf8tostr(lastUser.c_str()).c_str());
	Utils::duplicateString(m_rgFieldStrings[SOF_PASSWORD], L"");
	Utils::duplicateString(m_rgFieldStrings[SOF_NEW_PASSWORD1], L"");
	Utils::duplicateString(m_rgFieldStrings[SOF_NEW_PASSWORD2], L"");
	m_pCredProvCredentialEvents->SetFieldString(this, SOF_USER, m_rgFieldStrings[SOF_USER]);
	m_pCredProvCredentialEvents->SetFieldString(this, SOF_PASSWORD, L"");
	m_pCredProvCredentialEvents->SetFieldString(this, SOF_NEW_PASSWORD1, L"");
	m_pCredProvCredentialEvents->SetFieldString(this, SOF_NEW_PASSWORD2, L"");

	m_cpfis[SOF_USER] = lastUser.empty()? CPFIS_FOCUSED: CPFIS_NONE;
	m_cpfis[SOF_PASSWORD] = lastUser.empty()? CPFIS_NONE: CPFIS_FOCUSED;
	m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SOF_USER, m_cpfis[SOF_USER]);
	m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SOF_PASSWORD, m_cpfis[SOF_PASSWORD]);
	return S_OK ;
}

//Similarly to SetSelected, LogonUI calls this when your tile was selected
//and now no longer is.  The most common thing to do here (which we do below)
//is to clear out the password field.
HRESULT SoffidCredential::SetDeselected ()
{
	m_log.info("Credential::SetDeselected");
	return S_OK ;
}

//
// Get info for a particular field of a tile. Called by logonUI to get information to display the tile.
//
HRESULT SoffidCredential::GetFieldState (DWORD dwFieldID,
		CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
		CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis)
{
	HRESULT hr;

	m_log.info("Credential::GetFieldState %d", dwFieldID);

	switch (dwFieldID)
	{

	}
	if (dwFieldID < SOF_NUM_FIELDS && pcpfs && pcpfis)
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
HRESULT SoffidCredential::GetStringValue (DWORD dwFieldID, PWSTR* ppwz)
{
	HRESULT hr;

	m_log.info("Credential::GetStringValue %d", dwFieldID);

	// Check to make sure dwFieldID is a legitimate index
	if (dwFieldID < SOF_NUM_FIELDS && ppwz)
	{
		hr = Utils::duplicateString(*ppwz, m_rgFieldStrings[dwFieldID]);
		m_log.info(L"Result = %ls", *ppwz);
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
HRESULT SoffidCredential::GetBitmapValue (DWORD dwFieldID, HBITMAP* phbmp)
{
	HRESULT hr;

	m_log.info("Credential::GetBitmapValue %d", dwFieldID);

	if (SOF_IMAGE == dwFieldID && phbmp)
	{
		HBITMAP hbmp;
		hbmp = LoadBitmap(hSayakaInstance, "LOGO");

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
HRESULT SoffidCredential::GetSubmitButtonValue (DWORD dwFieldID,
		DWORD* pdwAdjacentTo)
{
	HRESULT hr;

	m_log.info("Credential::GetSubmitButtonValue %d", dwFieldID);

	if (SOF_SUBMIT_BUTTON == dwFieldID && pdwAdjacentTo)
	{
		// pdwAdjacentTo is a pointer to the fieldID you want the submit button to appear next to.
		if (m_cpfs[SOF_NEW_PASSWORD2] == CPFS_DISPLAY_IN_SELECTED_TILE)
			*pdwAdjacentTo = SOF_NEW_PASSWORD2;
		else
			*pdwAdjacentTo = SOF_PASSWORD;
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
HRESULT SoffidCredential::SetStringValue (DWORD dwFieldID, PCWSTR pwz)
{
	HRESULT hr;

	m_log.info(L"Credential::SetStringValue %d = %l"
			"s", dwFieldID, pwz);
	if (dwFieldID == SOF_USER || dwFieldID == SOF_PASSWORD ||
			dwFieldID == SOF_NEW_PASSWORD1 || dwFieldID == SOF_NEW_PASSWORD2)
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
HRESULT SoffidCredential::GetCheckboxValue (DWORD dwFieldID, BOOL* pbChecked,
		PWSTR* ppwzLabel)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(pbChecked);
	UNREFERENCED_PARAMETER(ppwzLabel);

	return E_NOTIMPL ;
}

HRESULT SoffidCredential::GetComboBoxValueCount (DWORD dwFieldID, DWORD* pcItems,
		DWORD* pdwSelectedItem)
{
	return E_INVALIDARG ;
}

HRESULT SoffidCredential::GetComboBoxValueAt (DWORD dwFieldID, DWORD dwItem,
		PWSTR* ppwzItem)
{
	return E_INVALIDARG ;
}

HRESULT SoffidCredential::SetCheckboxValue (DWORD dwFieldID, BOOL bChecked)
{
	UNREFERENCED_PARAMETER(dwFieldID);
	UNREFERENCED_PARAMETER(bChecked);

	return E_NOTIMPL ;
}

HRESULT SoffidCredential::SetComboBoxSelectedValue (DWORD dwFieldId,
		DWORD dwSelectedItem)
{
	return E_INVALIDARG ;
}

HRESULT SoffidCredential::CommandLinkClicked (DWORD dwFieldID)
{
	m_log.info("CommanLinkClicked");
	UNREFERENCED_PARAMETER(dwFieldID);
	return E_NOTIMPL ;
}
//------ end of methods for controls we don't have in our tile ----//

//
// Collect the username and password into a serialized credential for the correct usage scenario
// (logon/unlock is what's demonstrated in this sample).  LogonUI then passes these credentials
// back to the system to log on.
//
HRESULT SoffidCredential::GetSerialization (
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

bool SoffidCredential::ObtainCredentials (
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
	SoffidDirHandler h;

	std::wstring szUser = m_rgFieldStrings[SOF_USER];
	std::wstring szPass = m_rgFieldStrings[SOF_PASSWORD];
	std::wstring szPass1 = m_rgFieldStrings[SOF_NEW_PASSWORD1];
	std::wstring szPass2 = m_rgFieldStrings[SOF_NEW_PASSWORD2];

	wchar_t wchComputerName [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW (wchComputerName, &dwSize);

	szHost.assign (wchComputerName);

	if (m_cpfs[SOF_NEW_PASSWORD1] == CPFS_DISPLAY_IN_SELECTED_TILE) {
		if (szPass1 == szPass2) {
			h.validate(szUser, szPass, szPass1);
			if (h.valid) {
				this->szUser = szUser;
				this->szPassword = szPass1;
				return true;
			}
			else
			{
				std::wstring msg = MZNC_strtowstr(Utils::LoadResourcesString(53).c_str()); // Password not acceptable
				Utils::duplicateString(m_rgFieldStrings[SOF_WARNING],msg.c_str());
				m_pCredProvCredentialEvents->SetFieldString(this, SOF_WARNING, msg.c_str());
				m_cpfs[SOF_WARNING] = CPFS_DISPLAY_IN_SELECTED_TILE;
				m_pCredProvCredentialEvents->SetFieldState(this, SOF_WARNING, m_cpfs[SOF_WARNING]);
				return false;
			}
		} else {
			std::wstring msg = MZNC_strtowstr(Utils::LoadResourcesString(51).c_str()); // Password do not match
			Utils::duplicateString(m_rgFieldStrings[SOF_WARNING],msg.c_str());
			m_pCredProvCredentialEvents->SetFieldString(this, SOF_WARNING, msg.c_str());
			m_cpfs[SOF_WARNING] = CPFS_DISPLAY_IN_SELECTED_TILE;
			m_pCredProvCredentialEvents->SetFieldState(this, SOF_WARNING, m_cpfs[SOF_WARNING]);
			return false;
		}
	}
	else
	{
		h.validate(szUser, szPass);
		if (h.error) { // Cannot connect. Cross fingers and go on
			this -> szUser = szUser;
			this -> szPassword = szPass;
			return true;
		}
		else if (h.mustChange) {
			std::wstring msg = MZNC_strtowstr(Utils::LoadResourcesString(50).c_str());
			Utils::duplicateString(m_rgFieldStrings[SOF_WARNING],msg.c_str());
			m_pCredProvCredentialEvents->SetFieldString(this, SOF_WARNING, msg.c_str());
			m_cpfs[SOF_WARNING] = CPFS_DISPLAY_IN_SELECTED_TILE;
			m_pCredProvCredentialEvents->SetFieldState(this, SOF_WARNING, m_cpfs[SOF_WARNING]);

			m_cpfs[SOF_NEW_PASSWORD1] = CPFS_DISPLAY_IN_SELECTED_TILE;
			m_pCredProvCredentialEvents->SetFieldState(this, SOF_NEW_PASSWORD1, m_cpfs[SOF_NEW_PASSWORD1]);
			m_pCredProvCredentialEvents->SetFieldString(this, SOF_NEW_PASSWORD1, L"");
			m_cpfis[SOF_NEW_PASSWORD1] = CPFIS_FOCUSED;
			m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SOF_NEW_PASSWORD1, m_cpfis[SOF_NEW_PASSWORD1]);

			m_cpfs[SOF_NEW_PASSWORD2] = CPFS_DISPLAY_IN_SELECTED_TILE;
			m_pCredProvCredentialEvents->SetFieldState(this, SOF_NEW_PASSWORD2, m_cpfs[SOF_NEW_PASSWORD2]);
			m_pCredProvCredentialEvents->SetFieldString(this, SOF_NEW_PASSWORD2, L"");

			m_pCredProvCredentialEvents->SetFieldSubmitButton(this, SOF_SUBMIT_BUTTON, SOF_NEW_PASSWORD2);

			m_cpfis[SOF_USER] = CPFIS_DISABLED;
			m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SOF_USER, m_cpfis[SOF_USER]);
//			m_pCredProvCredentialEvents->SetFieldState(this, SOF_USER, CPFS_HIDDEN);

			m_cpfis[SOF_PASSWORD] = CPFIS_DISABLED;
			m_pCredProvCredentialEvents->SetFieldInteractiveState(this, SOF_PASSWORD, m_cpfis[SOF_PASSWORD]);
			m_pCredProvCredentialEvents->SetFieldState(this, SOF_PASSWORD, CPFS_DISPLAY_IN_SELECTED_TILE);

			return false;
		}
		else if (h.valid) {
			this -> szUser = szUser;
			this -> szPassword = szPass;
			return true;
		}
		else {
			std::wstring msg = MZNC_strtowstr(Utils::LoadResourcesString(52).c_str()); //Access denied
			m_pCredProvCredentialEvents->SetFieldString(this, SOF_WARNING, msg.c_str());
			m_cpfs[SOF_WARNING] = CPFS_DISPLAY_IN_SELECTED_TILE;
			m_pCredProvCredentialEvents->SetFieldState(this, SOF_WARNING, m_cpfs[SOF_WARNING]);
			return false;
		}
	}
	return true;
}

HRESULT SoffidCredential::GenerateLoginSerialization (
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
	hr = Utils::generateKerberosInteractiveLogon(szUser.c_str(),
			szPassword.c_str(), szHost.c_str(), CPUS_LOGON,
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
				SeyconCommon::writeProperty("lastSoffidUser", MZNC_wstrtoutf8(szUser.c_str()).c_str());
				pcpcs->ulAuthenticationPackage = ulAuthPackage;
				pcpcs->clsidCredentialProvider = CLSID_Soffid;

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
		int message;
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
		19, CPSI_ERROR, }, {
		STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED,
		20, CPSI_WARNING }, {
		STATUS_PASSWORD_MUST_CHANGE, STATUS_SUCCESS,
		21, CPSI_WARNING }, };

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT SoffidCredential::ReportResult (NTSTATUS ntsStatus, NTSTATUS ntsSubstatus,
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
				Utils::LoadResourcesString(s_rgLogonStatusInfo[dwStatusInfo].message).c_str());
		*pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
	}

	// Since NULL is a valid value for *ppwszOptionalStatusText and *pcpsiOptionalStatusIcon
	// this function can't fail.
	return S_OK ;
}

void SoffidCredential::displayMessage ()
{
	if (m_pQueryContinueWithStatus != NULL) {
		m_pQueryContinueWithStatus->QueryContinue();
		m_pQueryContinueWithStatus->SetStatusMessage(L"Connecting....");
	}
}

HRESULT SoffidCredential::Connect(/* [in] */IQueryContinueWithStatus *pqcws) {
	m_log.info("Connecting to query continue");
	this->m_pQueryContinueWithStatus = pqcws;
	m_pQueryContinueWithStatus->AddRef();
	return S_OK;
}

HRESULT SoffidCredential::Disconnect(void) {
	if (m_pQueryContinueWithStatus != NULL)
		m_pQueryContinueWithStatus->Release();
	this->m_pQueryContinueWithStatus = NULL;
	return S_OK;
}

void SoffidCredential::hideMessage ()
{
}
