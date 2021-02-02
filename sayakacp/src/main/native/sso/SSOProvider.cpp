#include "sayaka.h"


#include "SSOProvider.h"
#include "SSOCredential.h"
#include "Utils.h"
#include <ssoclient.h>
#include <vector>
#include <MazingerInternal.h>
#include <SecretStore.h>

#include <MZNcompat.h>
#include <WebTransport.h>
#include <combaseapi.h>
#include <wincred.h>

#define CREDUIWIN_ENUMERATE_ADMINS 0x100

static CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgSSOFieldDescriptors[] =
{
    { SHI_IMAGE, CPFT_TILE_IMAGE, (wchar_t*) L"Image" },
    { SHI_TITLE, CPFT_LARGE_TEXT, (wchar_t*) L"Start as administrator" },
    { SHI_USER,  CPFT_EDIT_TEXT, (wchar_t*) L"User" },
    { SHI_PASSWORD, CPFT_PASSWORD_TEXT, (wchar_t*) L"Password" },
    { SHI_MESSAGE, CPFT_LARGE_TEXT, (wchar_t*) L"Validating credentials...."},
    { SHI_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON, (wchar_t*) L"Start" },
};

SSOProvider::SSOProvider (): m_log ("SSOProvider")
{
	m_credentialProviderEvents = NULL;
	m_log.info("Creating SSOProvider ********* 2");
	m_pCredential = new SSOCredential ();
	m_pCredential->AddRef();
	m_autoCredentials.clear();
}


HRESULT __stdcall SSOProvider::QueryInterface(REFIID riid, void **ppObj) {
	m_log.info ("sso: Query Interface ");
	wchar_t ach[128];
	StringFromGUID2(riid, ach, sizeof ach);
	m_log.info ("sso: Query Interface %ls", ach);
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}
	else if (riid == IID_ICredentialProvider) {
		m_log.info ("sso: Query Interface iid_icredentialprovider");
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	wchar_t *lpwszClsid;
	StringFromCLSID(riid, &lpwszClsid);
	m_log.info (L"sso: Query Interface Unknown %ls", lpwszClsid);

	*ppObj = NULL;
	return E_NOINTERFACE;
}





ULONG __stdcall SSOProvider::AddRef()
{
	m_log.info ("sso: addref");
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall SSOProvider::Release()
{
	m_log.info ("sso: release");
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) {
		delete this;
	}
	return nRefCount;
}



HRESULT __stdcall SSOProvider::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD dwFlags
    )
{
    HRESULT hr;

    scenarioFlags = dwFlags;

    m_log.info("Provider::SetUsageScenario %d", cpus);

    m_cpus = cpus;

    switch (cpus)
    {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:
    case CPUS_CHANGE_PASSWORD:
        hr = E_NOTIMPL;
        break;

    case CPUS_CREDUI:
    	s_rgSSOFieldDescriptors[SHI_TITLE].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(45).c_str()).c_str() );
    	s_rgSSOFieldDescriptors[SHI_USER].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(46).c_str()).c_str() );
    	s_rgSSOFieldDescriptors[SHI_PASSWORD].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(47).c_str()).c_str() );
    	s_rgSSOFieldDescriptors[SHI_MESSAGE].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(48).c_str()).c_str() );
    	hr = S_OK;
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

//
// SetSerialization takes the kind of buffer that you would normally return to LogonUI for
// an authentication attempt.  It's the opposite of ICredentialProviderCredential::GetSerialization.
// GetSerialization is implement by a credential and serializes that credential.  Instead,
// SetSerialization takes the serialization and uses it to create a tile.
//
// SetSerialization is called for two main scenarios.  The first scenario is in the credui case
// where it is prepopulating a tile with credentials that the user chose to store in the OS.
// The second situation is in a remote logon case where the remote client may wish to
// prepopulate a tile with a username, or in some cases, completely populate the tile and
// use it to logon without showing any UI.
//
// SetSerialization is currently optional, which is why it's not implemented in this sample.
// If that changes in the future, we will update the sample.
//
HRESULT __stdcall SSOProvider::SetSerialization(
    const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs
    )
{
    m_log.info("Provider::SetSerialization VVVVVVVV");
    std::wstring s = L"";
    for (int i=pcpcs->cbSerialization-2; i >=0; i-=2) {
    	wchar_t wch = * ((wchar_t*) &pcpcs->rgbSerialization[i]);
    	if (wch == L'/') break;
        std::wstring t;
        t += wch;
    	s = t + s;
    }
    m_log.info("Domain %ls", s.c_str());

    m_domain = s;

    std::vector<WebTransport*> rules = MZNWebTransportMatch();

    SecretStore ss(MZNC_getUserName());
    for (std::vector<WebTransport*>::iterator it = rules.begin(); it != rules.end(); it++) {
    	WebTransport* wt = *it;
    	std::wstring url = wt->url;
    	int i = url.find(L"://");
        m_log.info("Checking url %ls", url.c_str());
    	if ( i != url.npos) {
    		url = url.substr(i+3);
    		i = url.find(L"/");
    		if (i != url.npos) url = url.substr(0, i);
    		i = url.find(L":");
    		if (i != url.npos) url = url.substr(0, i);
            m_log.info("domain %ls", url.c_str());
    		if (url == m_domain) {
                m_log.info("Loading rules");
				std::wstring secret = L"account.";
				secret += wt->system;
				m_log.info("Check secret %ls", secret.c_str());
				std::vector<std::wstring> accounts = ss.getSecrets(secret.c_str());
				for (std::vector<std::wstring>::iterator it = accounts.begin(); it != accounts.end(); it ++) {
					secret = L"pass.";
					secret += wt->system;
					secret += L".";
					secret += *it;
					m_log.info("Check secret %ls", secret.c_str());
					wchar_t *str = ss.getSecret(secret.c_str());
					if (str != NULL) {
						SSOCredential *c = new SSOCredential;
						std::wstring userName;
						if (! wt->domain.empty()) {
							userName = wt->domain;
							userName += L"\\";
						}
						userName += *it;
						c->szUser = userName;
						c->szPassword = str;
						c->autoLogin = true;
						c->AddRef();
						c->updateLabel();
						m_autoCredentials.push_back(c);
					}
				}
    		}
    	}
    }
    UNREFERENCED_PARAMETER(pcpcs);
    return S_OK;
}

//
// Called by LogonUI to give you a callback.  Providers often use the callback if they
// some event would cause them to need to change the set of tiles that they enumerated
//
HRESULT __stdcall SSOProvider::Advise(
    ICredentialProviderEvents* pcpe,
    UINT_PTR upAdviseContext
    )
{
    m_log.info("Provider::Advise");
    m_credentialProviderEvents = pcpe;
    m_upAdviseContext = upAdviseContext;

    pcpe->AddRef ();

    return S_OK;
}

//
// Called by LogonUI when the ICredentialProviderEvents callback is no longer valid.
//
HRESULT __stdcall SSOProvider::UnAdvise()
{
    m_log.info("Provider::Unadvise");
	if (m_credentialProviderEvents == NULL)
	{
		return E_INVALIDARG;
	} else {
		m_credentialProviderEvents->Release();
		m_credentialProviderEvents = NULL;
	    return S_OK;
	}
}

//
// Called by LogonUI to determine the number of fields in your tiles.  This
// does mean that all your tiles must have the same number of fields.
// This number must include both visible and invisible fields. If you want a tile
// to have different fields from the other tiles you enumerate for a given usage
// scenario you must include them all in this count and then hide/show them as desired
// using the field descriptors.
//
HRESULT __stdcall SSOProvider::GetFieldDescriptorCount(
    DWORD* pdwCount
    )
{
    *pdwCount = SHI_NUM_FIELDS;
    m_log.info("Provider::GetFieldDescriptorCount");
    return S_OK;
}

//
// Gets the field descriptor for a particular field
//
HRESULT __stdcall SSOProvider::GetFieldDescriptorAt(
    DWORD dwIndex,
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetFieldDescriptorAt %d", dwIndex);

    // Verify dwIndex is a valid field.
    if (dwIndex < SHI_NUM_FIELDS && ppcpfd)
    {
        hr = Utils::FieldDescriptorCoAllocCopy(s_rgSSOFieldDescriptors[dwIndex], ppcpfd);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//
// Sets pdwCount to the number of tiles that we wish to show at this time.
// Sets pdwDefault to the index of the tile which should be used as the default.
// The default tile is the tile which will be shown in the zoomed view by default. If
// more than one provider specifies a default tile the behavior is undefined.
// If *pbAutoLogonWithDefault is TRUE, LogonUI will immediately call GetSerialization
// on the credential you've specified as the default and will submit that credential
// for authentication without showing any further UI.
//
HRESULT __stdcall SSOProvider::GetCredentialCount(
    DWORD* pdwCount,
    DWORD* pdwDefault,
    BOOL* pbAutoLogonWithDefault
    )
{

    m_log.info("Provider::GetCredentialCount");
	if (scenarioFlags & CREDUIWIN_ENUMERATE_ADMINS)
	    *pdwCount = 0; // This provider always has the same number of credentials.
	else {
		int count = m_autoCredentials.size();
		if (basic && count != 1) count++;
		*pdwCount = count;
	}
	*pdwDefault = 0;
	*pbAutoLogonWithDefault = m_autoCredentials.size() == 1;
    return S_OK;
}

//
// Returns the credential at the index specified by dwIndex. This function is called by logonUI to enumerate
// the tiles.
//
HRESULT __stdcall SSOProvider::GetCredentialAt(
    DWORD dwIndex,
    ICredentialProviderCredential** ppcpc
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetCredentialAt %d", dwIndex);

    if(ppcpc )
    {
    	if ( dwIndex == m_autoCredentials.size())
    		hr = m_pCredential->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
    	else
    		hr = m_autoCredentials[dwIndex]->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


SSOProvider::~SSOProvider () {
	for (std::vector<SSOCredential *>::iterator it = m_autoCredentials.begin();
			it != m_autoCredentials.end();
			it ++) {
		SSOCredential* cred = *it;
		cred->Release();
	}
	m_autoCredentials.clear();
	if (m_pCredential != NULL)
		m_pCredential->Release();
	m_pCredential = NULL;
}


