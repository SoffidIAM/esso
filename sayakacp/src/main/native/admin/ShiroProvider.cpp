#include "sayaka.h"


#include "ShiroProvider.h"
#include "ShiroCredential.h"
#include "Utils.h"
#include <ssoclient.h>

#include <MZNcompat.h>

ShiroProvider* ShiroProvider::s_handler;
static CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgShiroFieldDescriptors[] =
{
    { SHI_IMAGE, CPFT_TILE_IMAGE, (wchar_t*) L"Image" },
    { SHI_TITLE, CPFT_LARGE_TEXT, (wchar_t*) L"Start as administrator" },
    { SHI_USER,  CPFT_EDIT_TEXT, (wchar_t*) L"User" },
    { SHI_PASSWORD, CPFT_PASSWORD_TEXT, (wchar_t*) L"Password" },
    { SHI_MESSAGE, CPFT_LARGE_TEXT, (wchar_t*) L"Validating credentials...."},
    { SHI_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON, (wchar_t*) L"Start" },
};

ShiroProvider::ShiroProvider (): m_log ("ShiroProvider")
{
	m_credentialProviderEvents = NULL;
	s_handler = this;
	m_log.info("Creating ShiroProvider");
	m_pCredential = new ShiroCredential ();
	m_pCredential->AddRef();
}


HRESULT __stdcall ShiroProvider::QueryInterface(REFIID riid, void **ppObj) {
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}
	else if (riid == IID_ICredentialProvider) {
		m_log.info ("Query Interface iid_icredentialprovider");
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	wchar_t *lpwszClsid;
	StringFromCLSID(riid, &lpwszClsid);
	m_log.info (L"Query Interface Unknown %s", lpwszClsid);

	*ppObj = NULL;
	return E_NOINTERFACE;
}





ULONG __stdcall ShiroProvider::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall ShiroProvider::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) {
		if (s_handler == this)
			s_handler = NULL;
		delete this;
	}
	return nRefCount;
}



HRESULT ShiroProvider::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);
    HRESULT hr;

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
    	s_rgShiroFieldDescriptors[SHI_TITLE].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(45).c_str()).c_str() );
    	s_rgShiroFieldDescriptors[SHI_USER].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(46).c_str()).c_str() );
    	s_rgShiroFieldDescriptors[SHI_PASSWORD].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(47).c_str()).c_str() );
    	s_rgShiroFieldDescriptors[SHI_MESSAGE].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(48).c_str()).c_str() );
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
STDMETHODIMP ShiroProvider::SetSerialization(
    const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs
    )
{
    m_log.info("Provider::SetSerialization");
    UNREFERENCED_PARAMETER(pcpcs);
    return E_NOTIMPL;
}

//
// Called by LogonUI to give you a callback.  Providers often use the callback if they
// some event would cause them to need to change the set of tiles that they enumerated
//
HRESULT ShiroProvider::Advise(
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
HRESULT ShiroProvider::UnAdvise()
{
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
HRESULT ShiroProvider::GetFieldDescriptorCount(
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
HRESULT ShiroProvider::GetFieldDescriptorAt(
    DWORD dwIndex,
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetFieldDescriptorAt %d", dwIndex);

    // Verify dwIndex is a valid field.
    if (dwIndex < SHI_NUM_FIELDS && ppcpfd)
    {
        hr = Utils::FieldDescriptorCoAllocCopy(s_rgShiroFieldDescriptors[dwIndex], ppcpfd);
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
HRESULT ShiroProvider::GetCredentialCount(
    DWORD* pdwCount,
    DWORD* pdwDefault,
    BOOL* pbAutoLogonWithDefault
    )
{
    m_log.info("Provider::GetCredentialCount");
    *pdwCount = 1; // This provider always has the same number of credentials.
	*pdwDefault = 0;
	*pbAutoLogonWithDefault = false;
    return S_OK;
}

//
// Returns the credential at the index specified by dwIndex. This function is called by logonUI to enumerate
// the tiles.
//
HRESULT ShiroProvider::GetCredentialAt(
    DWORD dwIndex,
    ICredentialProviderCredential** ppcpc
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetCredentialAt %d", dwIndex);

    if(dwIndex == 0 && ppcpc )
    {
        hr = m_pCredential->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


ShiroProvider::~ShiroProvider () {
	m_pCredential->Release();
}


