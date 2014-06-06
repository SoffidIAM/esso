#include "sayaka.h"

#include "RecoverProvider.h"
#include "RecoverCredential.h"
#include "Utils.h"
#include <security.h>
#include <lm.h>
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
#include <ssoclient.h>

# include <MZNcompat.h>

RecoverCredential::RecoverCredential (const std::wstring& domain):
	m_log ("RecoverCredential")
{
    m_log.info("RecoverCredential::RecoverCredential");

    this->windowsDomain = domain;

    m_pCredProvCredentialEvents = NULL;
    // Copy the field descriptors for each field. This is useful if you want to vary the field
    // descriptors based on what Usage scenario the credential was created for.
    m_cpfs[REC_TITLE] = CPFS_DISPLAY_IN_BOTH;
    m_cpfs[REC_IMAGE] = CPFS_DISPLAY_IN_BOTH;
    m_cpfs[REC_USER] = CPFS_DISPLAY_IN_SELECTED_TILE;
    m_cpfs[REC_SUBMIT_BUTTON] = CPFS_DISPLAY_IN_SELECTED_TILE;
    m_cpfs[REC_CHANGE_MSG] = CPFS_HIDDEN;
    m_cpfs[REC_NEW_PASSWORD] = CPFS_HIDDEN;
    m_cpfs[REC_NEW_PASSWORD2] = CPFS_HIDDEN;
    m_cpfs[REC_QUESTION] = CPFS_HIDDEN;
    m_cpfs[REC_ANSWER] = CPFS_HIDDEN;

    m_cpfis[REC_TITLE] = CPFIS_NONE;
    m_cpfis[REC_USER] = CPFIS_FOCUSED;
    m_cpfis[REC_IMAGE] = CPFIS_NONE;
    m_cpfis[REC_SUBMIT_BUTTON] = CPFIS_NONE;
    m_cpfis[REC_CHANGE_MSG] = CPFIS_NONE;
    m_cpfis[REC_NEW_PASSWORD] = CPFIS_NONE;
    m_cpfis[REC_NEW_PASSWORD2] = CPFIS_NONE;
    m_cpfis[REC_QUESTION] = CPFIS_NONE;
    m_cpfis[REC_ANSWER] = CPFIS_NONE;
    // Initialize the String value of all the fields
   	Utils::duplicateString(m_rgFieldStrings[REC_IMAGE], L"");
   	Utils::duplicateString(m_rgFieldStrings[REC_USER], L"");
    Utils::duplicateString(m_rgFieldStrings[REC_TITLE],
    		MZNC_strtowstr(Utils::LoadResourcesString(35).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[REC_SUBMIT_BUTTON],
    		MZNC_strtowstr(Utils::LoadResourcesString(37).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[REC_CHANGE_MSG], L"");
    Utils::duplicateString(m_rgFieldStrings[REC_NEW_PASSWORD], L"");
    Utils::duplicateString(m_rgFieldStrings[REC_NEW_PASSWORD2], L"");

    currentQuestion = -1;
}

HRESULT __stdcall RecoverCredential::QueryInterface(REFIID riid, void **ppObj) {
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

ULONG __stdcall RecoverCredential::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall RecoverCredential::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) delete this;
	return nRefCount;
}

//LogonUI calls this in order to give us a callback in case we need to notify it of anything
HRESULT RecoverCredential::Advise(
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
HRESULT RecoverCredential::UnAdvise()
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
HRESULT RecoverCredential::SetSelected(BOOL* pbAutoLogon)
{
	m_log.info("Credential::SetSelected");
//    *pbAutoLogon = true;
	*pbAutoLogon = false;

    m_cpfs[REC_TITLE] = CPFS_DISPLAY_IN_BOTH;
    m_cpfs[REC_IMAGE] = CPFS_DISPLAY_IN_BOTH;
    m_cpfs[REC_USER] = CPFS_DISPLAY_IN_SELECTED_TILE;
    m_cpfs[REC_SUBMIT_BUTTON] = CPFS_DISPLAY_IN_SELECTED_TILE;
    m_cpfs[REC_CHANGE_MSG] = CPFS_HIDDEN;
    m_cpfs[REC_NEW_PASSWORD] = CPFS_HIDDEN;
    m_cpfs[REC_NEW_PASSWORD2] = CPFS_HIDDEN;
    m_cpfs[REC_QUESTION] = CPFS_HIDDEN;
    m_cpfs[REC_ANSWER] = CPFS_HIDDEN;

    m_cpfis[REC_TITLE] = CPFIS_NONE;
    m_cpfis[REC_USER] = CPFIS_FOCUSED;
    m_cpfis[REC_IMAGE] = CPFIS_NONE;
    m_cpfis[REC_SUBMIT_BUTTON] = CPFIS_NONE;
    m_cpfis[REC_CHANGE_MSG] = CPFIS_NONE;
    m_cpfis[REC_NEW_PASSWORD] = CPFIS_NONE;
    m_cpfis[REC_NEW_PASSWORD2] = CPFIS_NONE;
    m_cpfis[REC_QUESTION] = CPFIS_NONE;
    m_cpfis[REC_ANSWER] = CPFIS_NONE;
    // Initialize the String value of all the fields
   	Utils::duplicateString(m_rgFieldStrings[REC_IMAGE], L"");
   	Utils::duplicateString(m_rgFieldStrings[REC_USER], L"");
    Utils::duplicateString(m_rgFieldStrings[REC_TITLE],
    		MZNC_strtowstr(Utils::LoadResourcesString(35).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[REC_SUBMIT_BUTTON],
    		MZNC_strtowstr(Utils::LoadResourcesString(37).c_str()).c_str());
    Utils::duplicateString(m_rgFieldStrings[REC_CHANGE_MSG], L"");
    Utils::duplicateString(m_rgFieldStrings[REC_NEW_PASSWORD], L"");
    Utils::duplicateString(m_rgFieldStrings[REC_NEW_PASSWORD2], L"");

    if (m_pCredProvCredentialEvents != NULL)
    {
    	for (int i = 0; i < REC_NUM_FIELDS; i++)
    	{
    		m_pCredProvCredentialEvents->SetFieldState(this, i, m_cpfs[i]);
    		m_pCredProvCredentialEvents->SetFieldInteractiveState(this, i, m_cpfis[i]);
    	}
		m_pCredProvCredentialEvents->SetFieldString(this, REC_NEW_PASSWORD, L"");
		m_pCredProvCredentialEvents->SetFieldString(this, REC_NEW_PASSWORD2, L"");
		m_pCredProvCredentialEvents->SetFieldString(this, REC_ANSWER, L"");
		m_pCredProvCredentialEvents->SetFieldString(this, REC_USER, L"");
    }


    return S_OK;
}

//Similarly to SetSelected, LogonUI calls this when your tile was selected
//and now no longer is.  The most common thing to do here (which we do below)
//is to clear out the password field.
HRESULT RecoverCredential::SetDeselected()
{
	m_log.info("Credential::SetDeselected");
	user = L"";
	requestId = L"";
	currentQuestion = -1;
	m_questions.clear();
	m_answers.clear();


	return S_OK;
}

//
// Get info for a particular field of a tile. Called by logonUI to get information to display the tile.
//
HRESULT RecoverCredential::GetFieldState(
    DWORD dwFieldID,
    CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis
    )
{
    HRESULT hr;

    m_log.info("Credential::GetFieldState %d", dwFieldID);

    switch (dwFieldID) {

    }
    if (dwFieldID < REC_NUM_FIELDS && pcpfs && pcpfis)
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
HRESULT RecoverCredential::GetStringValue(
    DWORD dwFieldID,
    PWSTR* ppwz
    )
{
    HRESULT hr;

    m_log.info("Credential::GetStringValue %d", dwFieldID);

    // Check to make sure dwFieldID is a legitimate index
    if (ppwz && dwFieldID == REC_ANSWER && currentQuestion >= 0 && currentQuestion < m_answers.size())
    {
    	m_log.info("Duplicating strnig for answer %d: %ls", currentQuestion, m_answers[currentQuestion].c_str());
    	hr = Utils::duplicateString(*ppwz, m_answers[currentQuestion].c_str());
    }
    else if (ppwz && dwFieldID == REC_QUESTION && currentQuestion >= 0 && currentQuestion < m_questions.size())
    {
    	hr = Utils::duplicateString(*ppwz, m_questions[currentQuestion].c_str());
    }
    else if (ppwz && dwFieldID == REC_TITLE)
    {
    	hr = Utils::duplicateString(*ppwz, m_rgFieldStrings[REC_TITLE]);
    }
    else if (ppwz && dwFieldID == REC_USER)
    {
    	hr = Utils::duplicateString(*ppwz, user.c_str());
    }
    else if (ppwz && dwFieldID < REC_NUM_FIELDS)
    {
    	hr = Utils::duplicateString(*ppwz, "");
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
HRESULT RecoverCredential::GetBitmapValue(
    DWORD dwFieldID,
    HBITMAP* phbmp
    )
{
    HRESULT hr;

    m_log.info("Credential::GetBitmapValue %d", dwFieldID);

    if (REC_IMAGE == dwFieldID && phbmp)
    {
    	HBITMAP hbmp;
   		hbmp = LoadBitmap(hSayakaInstance, "LIFERING");

        if (hbmp != NULL)
        {
        	m_log.info ("Generated bitmap");
            *phbmp = hbmp;
            hr = S_OK;
        } else {
        	m_log.warn ("Cannot load bitmap LIFERING");
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
HRESULT RecoverCredential::GetSubmitButtonValue(
    DWORD dwFieldID,
    DWORD* pdwAdjacentTo
    )
{
    HRESULT hr;

    m_log.info("Credential::GetSubmitButtonValue %d", dwFieldID);

    if (REC_SUBMIT_BUTTON == dwFieldID && pdwAdjacentTo)
    {
        // pdwAdjacentTo is a pointer to the fieldID you want the submit button to appear next to.
        *pdwAdjacentTo = REC_USER;
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
HRESULT RecoverCredential::SetStringValue(
    DWORD dwFieldID,
    PCWSTR pwz
    )
{
    HRESULT hr;

    m_log.info(L"Credential::SetStringValue %d = %ls", dwFieldID, pwz);
    if (dwFieldID == REC_USER)
    {
    	user = pwz;
    }
    else if (dwFieldID == REC_ANSWER  && currentQuestion >= 0)
    {
    	while (m_answers.size() <= currentQuestion)
    	{
    		m_answers.insert(m_answers.end(), std::wstring());
    	}
    	m_answers [currentQuestion] = pwz;
    }
    else if (dwFieldID ==  REC_NEW_PASSWORD)
    	desiredPassword1 = pwz;
    else if (dwFieldID == REC_NEW_PASSWORD2)
    	desiredPassword2 = pwz;

    hr = S_OK;

    return hr;
}

//-------------
// The following methods are for logonUI to get the values of various UI elements and then communicate
// to the credential about what the user did in that field.  However, these methods are not implemented
// because our tile doesn't contain these types of UI elements
HRESULT RecoverCredential::GetCheckboxValue(
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

HRESULT RecoverCredential::GetComboBoxValueCount(
    DWORD dwFieldID,
    DWORD* pcItems,
    DWORD* pdwSelectedItem
    )
{
	return E_INVALIDARG;
}

HRESULT RecoverCredential::GetComboBoxValueAt(
    DWORD dwFieldID,
    DWORD dwItem,
    PWSTR* ppwzItem
    )
{
	return E_INVALIDARG;
}

HRESULT RecoverCredential::SetCheckboxValue(
    DWORD dwFieldID,
    BOOL bChecked
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(bChecked);

    return E_NOTIMPL;
}

HRESULT RecoverCredential::SetComboBoxSelectedValue(
    DWORD dwFieldId,
    DWORD dwSelectedItem
    )
{
	return E_INVALIDARG;
}

HRESULT RecoverCredential::CommandLinkClicked(DWORD dwFieldID)
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
HRESULT RecoverCredential::GetSerialization(
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
	if (currentQuestion < 0)
	{
		if (performRequest ())
			currentQuestion = 0;
		*pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
		*pcpsiOptionalStatusIcon = CPSI_ERROR;
		hr = S_OK;
		updateInterface();
	}
	else if ( currentQuestion < m_questions.size())
	{
		currentQuestion ++;
		if (currentQuestion == m_questions.size())
		{
			if (! responseChallenge())
			{
				currentQuestion = 0;
			}
		}
		updateInterface ();
	} else {
		if (desiredPassword1 != desiredPassword2) {
			Utils::duplicateString (*ppwzOptionalStatusText,
					Utils::LoadResourcesString(16).c_str());
			*pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
			*pcpsiOptionalStatusIcon = CPSI_ERROR;
			hr = S_OK;
			m_log.info("Passwords do not match");
		} else {
			if (resetPassword())
			{
				m_log.info("Trying change password");
				hr = GenerateLoginSerialization(pcpgsr, pcpcs, ppwzOptionalStatusText, pcpsiOptionalStatusIcon);
			}
		}
	}
	return hr;
}



HRESULT RecoverCredential::GenerateLoginSerialization(
	CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
	CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
	PWSTR* ppwzOptionalStatusText,
	CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
	) {

	UNREFERENCED_PARAMETER(ppwzOptionalStatusText);
    UNREFERENCED_PARAMETER(pcpsiOptionalStatusIcon);

    HRESULT hr;

	KERB_INTERACTIVE_UNLOCK_LOGON kiul;
	ZeroMemory(&kiul, sizeof(kiul));
	*pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;


	hr = Utils::generateKerberosInteractiveLogon(
			user.c_str(),
			desiredPassword1.c_str(),
			windowsDomain.c_str(),
			CPUS_LOGON,
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
				pcpcs->clsidCredentialProvider = CLSID_Recover;

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


struct REPORT_RESULT_STATUS_INFO
{
    NTSTATUS ntsStatus;
    NTSTATUS ntsSubstatus;
    PWSTR     pwzMessage;
    CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

//these are currently defined in the ddk, but not the sdk
#ifndef STATUS_LOGON_FAILURE
#define STATUS_LOGON_FAILURE	(0xC000006DL)     // ntsubauth
#endif

#ifndef STATUS_ACCOUNT_RESTRICTION
#define STATUS_ACCOUNT_RESTRICTION	(0xC000006EL)     // ntsubauth
#endif

#ifndef STATUS_ACCOUNT_DISABLED
#define STATUS_ACCOUNT_DISABLED		(0xC0000072L)     // ntsubauth
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS	(0x000000000)     // ntsubauth
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
HRESULT RecoverCredential::ReportResult(
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

    return S_OK;
}

static int hexparse (wchar_t ch)
{
	if (ch >= L'0' && ch <= L'9')
		return ch - L'0';
	else if (ch >= L'a' && ch <= L'f')
		return ch - L'a' + 10;
	else if (ch >= L'A' && ch <= L'F')
		return ch - L'A' + 10;
	else
		return -1;
}

static std::wstring unscape (std::wstring s)
{
	std::string result;
	int i = 0;
	while ( i < s.length())
	{
		wchar_t wch = s[i++];
		if (wch == L'+')
			result += ' ';
		else if (wch == L'%' && i+1 < s.length())
		{
			int x = hexparse (s[i++]) * 16 + hexparse (s[i++]);
			result += (char) x;
		}
		else
			result += (char) wch;
	}
	return MZNC_utf8towstr(result.c_str());
}


boolean RecoverCredential::performRequest() {
	SeyconService service;

	m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_HIDDEN);

	std::string d;
	SeyconCommon::updateConfig("SSOSoffidAgent");
    SeyconCommon::readProperty("SSOSoffidAgent", d);
	SeyconResponse* response;
    response = service.sendUrlMessage(L"/rememberPasswordServlet?action=requestChallenge&user=%ls&domain=%ls",
    		service.escapeString(user.c_str()).c_str(),
    		service.escapeString(d.c_str()).c_str());

    if (response != NULL)
    {
    	std::string status = response->getToken(0);
    	if (status == "OK")
    	{
    		domain = MZNC_strtowstr(d.c_str());
    		m_answers.clear();
    		m_questions.clear();
    		response->getToken(1, requestId);
    		int i = 2;
			std::wstring q;
    		while (response->getToken(i++, q))
			{
				m_questions.insert(m_questions.end(), unscape(q));
			}
        	delete response;
    		return true;
    	} else {
    		std::wstring msg1;
    		std::wstring msg2;
    		response->getToken(0, msg1);
    		response->getToken(1, msg2);
    		msg1 += L":" ;
    		msg1 += msg2;
    		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
    				msg2.c_str());
    		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
        	delete response;
    		return false;
    	}
    } else {
		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
				MZNC_strtowstr(Utils::LoadResourcesString(38).c_str()).c_str());
		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
		return false;
    }
}

boolean RecoverCredential::responseChallenge() {
	SeyconService service;

	m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_HIDDEN);

	std::wstring msg = std::wstring(L"/rememberPasswordServlet?action=responseChallenge&user=")+
			service.escapeString(user.c_str()) + L"&domain" +
			service.escapeString(domain.c_str()) + L"&id=" +
			service.escapeString(requestId.c_str());

	for (int i = 0; i < m_questions.size(); i++)
	{
		msg += L"&";
		msg += service.escapeString(m_questions[i].c_str());
		msg += L"=";
		msg += service.escapeString(m_answers[i].c_str());
	}
	SeyconResponse* response;
    response = service.sendUrlMessage(msg.c_str());

    if (response != NULL)
    {
    	std::string status = response->getToken(0);
    	if (status == "OK")
    	{
        	delete response;
    		return true;
    	} else {
    		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
    				MZNC_strtowstr(Utils::LoadResourcesString(39).c_str()).c_str());
    		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
        	delete response;
    		return false;
    	}
    } else {
		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
				MZNC_strtowstr(Utils::LoadResourcesString(38).c_str()).c_str());
		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
		return false;
    }
}

boolean RecoverCredential::resetPassword() {
	SeyconService service;

	m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_HIDDEN);

	SeyconResponse* response;
    response = service.sendUrlMessage(L"/rememberPasswordServlet?action=resetPassword&user=%ls&domain=%s&password=%ls&id=%ls",
    		service.escapeString(user.c_str()).c_str(),
    		domain.c_str(),
    		service.escapeString(desiredPassword1.c_str()).c_str(),
    		requestId.c_str());

    if (response != NULL)
    {
    	std::string status = response->getToken(0);
    	if (status == "OK")
    	{
    		m_answers.clear();
    		m_questions.clear();
        	delete response;
    		return true;
    	} else if (status == "BADPASSWORD")
       	{
    		std::wstring msg2;
    		response->getToken(1, msg2);

    		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
    				msg2.c_str());
    		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
        	delete response;
    		return false;
    	} else {
    		std::wstring msg1;
    		std::wstring msg2;
    		response->getToken(0, msg1);
    		response->getToken(1, msg2);
    		msg1 += L":" ;
    		msg1 += msg2;
    		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
    				msg2.c_str());
    		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
        	delete response;
    		return false;
    	}
    } else {
		m_pCredProvCredentialEvents->SetFieldString(this, REC_CHANGE_MSG,
				MZNC_strtowstr(Utils::LoadResourcesString(38).c_str()).c_str());
		m_pCredProvCredentialEvents->SetFieldState(this, REC_CHANGE_MSG, CPFS_DISPLAY_IN_SELECTED_TILE);
		return false;
    }
}

RecoverCredential::~RecoverCredential() {
}

void RecoverCredential::updateInterface()
{
	if (currentQuestion < 0)
	{
		m_pCredProvCredentialEvents->SetFieldInteractiveState(this, REC_USER, CPFIS_FOCUSED);

		m_pCredProvCredentialEvents->SetFieldSubmitButton(this, REC_SUBMIT_BUTTON, REC_USER);

	} else if (currentQuestion < m_questions.size()) {
		m_pCredProvCredentialEvents->SetFieldInteractiveState(this, REC_USER, CPFIS_READONLY);

		m_pCredProvCredentialEvents->SetFieldState(this, REC_QUESTION, CPFS_DISPLAY_IN_SELECTED_TILE);
		m_pCredProvCredentialEvents->SetFieldString(this, REC_QUESTION, m_questions[currentQuestion].c_str());

		m_pCredProvCredentialEvents->SetFieldState(this, REC_ANSWER, CPFS_DISPLAY_IN_SELECTED_TILE);
		m_pCredProvCredentialEvents->SetFieldInteractiveState(this, REC_ANSWER, CPFIS_FOCUSED);
		m_pCredProvCredentialEvents->SetFieldString(this, REC_ANSWER, L"");

		m_pCredProvCredentialEvents->SetFieldSubmitButton(this, REC_SUBMIT_BUTTON, REC_ANSWER);
		m_pCredProvCredentialEvents->SetFieldInteractiveState(this, REC_USER, CPFIS_READONLY);

	} else {
		m_pCredProvCredentialEvents->SetFieldState(this, REC_QUESTION, CPFS_HIDDEN);
		m_pCredProvCredentialEvents->SetFieldState(this, REC_ANSWER, CPFS_HIDDEN);

		m_pCredProvCredentialEvents->SetFieldState(this, REC_NEW_PASSWORD, CPFS_DISPLAY_IN_SELECTED_TILE);
		m_pCredProvCredentialEvents->SetFieldState(this, REC_NEW_PASSWORD2, CPFS_DISPLAY_IN_SELECTED_TILE);
		m_pCredProvCredentialEvents->SetFieldInteractiveState(this, REC_NEW_PASSWORD, CPFIS_FOCUSED);

		m_pCredProvCredentialEvents->SetFieldString(this, REC_NEW_PASSWORD, L"");
		m_pCredProvCredentialEvents->SetFieldString(this, REC_NEW_PASSWORD2, L"");

		m_pCredProvCredentialEvents->SetFieldSubmitButton(this, REC_SUBMIT_BUTTON, REC_NEW_PASSWORD2);
	}
}

