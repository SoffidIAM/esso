#include "sayaka.h"


#include "SoffidProvider.h"
#include "SoffidCredential.h"
#include "Utils.h"
#include <ssoclient.h>

#include <MZNcompat.h>
#include <combaseapi.h>

SoffidProvider* SoffidProvider::s_handler;
static CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgSoffidFieldDescriptors[] =
{
    { SOF_IMAGE, CPFT_TILE_IMAGE, (wchar_t*) L"Image" },
    { SOF_TITLE, CPFT_LARGE_TEXT, (wchar_t*) L"Start as administrator" },
    { SOF_USER,  CPFT_EDIT_TEXT, (wchar_t*) L"User" },
    { SOF_PASSWORD, CPFT_PASSWORD_TEXT, (wchar_t*) L"Password" },
    { SOF_NEW_PASSWORD1, CPFT_PASSWORD_TEXT, (wchar_t*) L"New password" },
    { SOF_NEW_PASSWORD2, CPFT_PASSWORD_TEXT, (wchar_t*) L"Repeat password" },
    { SOF_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON, (wchar_t*) L"Start" },
    { SOF_MESSAGE, CPFT_LARGE_TEXT, (wchar_t*) L"Validating credentials...."},
    { SOF_WARNING, CPFT_LARGE_TEXT, (wchar_t*) L""},
};

SoffidProvider::SoffidProvider (): m_log ("SoffidProvider")
{
	m_nRefCount = 0;
	m_credentialProviderEvents = NULL;
	s_handler = this;
	m_log.info("Creating SoffidProvider ********* 2");
	m_pCredential = new SoffidCredential ();
	m_pCredential->AddRef();
}


HRESULT __stdcall SoffidProvider::QueryInterface(REFIID riid, void **ppObj) {
	m_log.info ("Soffid: Query Interface ");
	wchar_t ach[128];
	StringFromGUID2(riid, ach, sizeof ach);
	m_log.info ("Soffid: Query Interface %ls", ach);
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}
	else if (riid == IID_ICredentialProvider) {
		m_log.info ("Soffid: Query Interface iid_icredentialprovider");
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	wchar_t *lpwszClsid;
	StringFromCLSID(riid, &lpwszClsid);
	m_log.info (L"Soffid: Query Interface Unknown %ls", lpwszClsid);

	*ppObj = NULL;
	return E_NOINTERFACE;
}


ULONG __stdcall SoffidProvider::AddRef()
{
	m_log.info (" addref");
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall SoffidProvider::Release()
{
	m_log.info (" release");
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) {
		m_log.info ("Removing");
		delete this;
	}
	return nRefCount;
}


HRESULT __stdcall SoffidProvider::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD dwFlags
    )
{
    HRESULT hr;

    m_log.info("Provider::SetUsageScenario %d", cpus);

    std::string type;
    SeyconCommon::readProperty("LoginType", type);

    m_cpus = cpus;

    switch (cpus)
    {
    case CPUS_LOGON:
    	s_rgSoffidFieldDescriptors[SOF_TITLE].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(45).c_str()).c_str() );
    	s_rgSoffidFieldDescriptors[SOF_USER].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(46).c_str()).c_str() );
    	s_rgSoffidFieldDescriptors[SOF_PASSWORD].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(47).c_str()).c_str() );
    	s_rgSoffidFieldDescriptors[SOF_MESSAGE].pszLabel =  wcsdup ( MZNC_strtowstr(Utils::LoadResourcesString(48).c_str()).c_str() );
    	if (type == "soffid")
    		hr = S_OK;
    	else
            hr = E_NOTIMPL;
        break;

    case CPUS_UNLOCK_WORKSTATION:
    case CPUS_CHANGE_PASSWORD:
    case CPUS_CREDUI:
        hr = E_NOTIMPL;
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
HRESULT __stdcall SoffidProvider::SetSerialization(
    const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs
    )
{
    m_log.info("Provider::SetSerialization VVVVVVVV");
//    if (pcpcs != NULL)
//    	m_log.info("Provider::SetSerialization : %ld", pcpcs->ulAuthenticationPackage);
    UNREFERENCED_PARAMETER(pcpcs);
    return S_OK;
}

//
// Called by LogonUI to give you a callback.  Providers often use the callback if they
// some event would cause them to need to change the set of tiles that they enumerated
//
HRESULT __stdcall SoffidProvider::Advise(
    ICredentialProviderEvents* pcpe,
    UINT_PTR upAdviseContext
    )
{
    m_log.info("Provider::Advise");
    m_credentialProviderEvents = pcpe;
    m_upAdviseContext = upAdviseContext;
    pcpe->AddRef ();

    if ( m_pCredential != NULL) {
    	m_pCredential -> m_credentialProviderEvents = pcpe;
    	m_pCredential -> m_upAdviseContext = upAdviseContext;
    }

    return S_OK;
}

//
// Called by LogonUI when the ICredentialProviderEvents callback is no longer valid.
//
HRESULT __stdcall SoffidProvider::UnAdvise()
{
    m_log.info("Provider::Unadvise");
	if (m_credentialProviderEvents == NULL)
	{
		return E_INVALIDARG;
	} else {
		m_credentialProviderEvents->Release();
		m_credentialProviderEvents = NULL;
	    if ( m_pCredential != NULL) {
	    	m_pCredential -> m_credentialProviderEvents = NULL;
	    }
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
HRESULT __stdcall SoffidProvider::GetFieldDescriptorCount(
    DWORD* pdwCount
    )
{
    *pdwCount = SOF_NUM_FIELDS;
    m_log.info("Provider::GetFieldDescriptorCount");
    return S_OK;
}

//
// Gets the field descriptor for a particular field
//
HRESULT __stdcall SoffidProvider::GetFieldDescriptorAt(
    DWORD dwIndex,
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetFieldDescriptorAt %d", dwIndex);

    // Verify dwIndex is a valid field.
    if (dwIndex < SOF_NUM_FIELDS && ppcpfd)
    {
        hr = Utils::FieldDescriptorCoAllocCopy(s_rgSoffidFieldDescriptors[dwIndex], ppcpfd);
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
HRESULT __stdcall SoffidProvider::GetCredentialCount(
    DWORD* pdwCount,
    DWORD* pdwDefault,
    BOOL* pbAutoLogonWithDefault
    )
{
	std::string type;
    m_log.info("Provider::GetCredentialCount");
	SeyconCommon::readProperty("LoginType", type);
	if (type == "soffid") {
		*pdwCount = 1;
		*pdwDefault = 0;
	}
	else {
		*pdwCount = 0;
		*pdwDefault = 0;
	}
	*pbAutoLogonWithDefault = false;
    return S_OK;
}

//
// Returns the credential at the index specified by dwIndex. This function is called by logonUI to enumerateoue
// the tiles.
//
HRESULT __stdcall SoffidProvider::GetCredentialAt(
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


SoffidProvider::~SoffidProvider () {
	m_pCredential->Release();
}


