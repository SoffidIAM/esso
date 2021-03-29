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

#include <sspi.h>
#include "sspi-update.h"
#include <time.h>

static time_t lastLogin = 0;
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
	m_nRefCount = 0;
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
		m_log.info ("Removing");
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

static std::wstring dumpSerialization (
		Log & log,
	    const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs
	){
	SEC_WINNT_CREDUI_CONTEXT* ctx = NULL;
	ULONG headerSize = 0x34;
	SspiUnmarshalCredUIContext(&pcpcs->rgbSerialization[headerSize], pcpcs->cbSerialization - headerSize, &ctx);

	if (ctx == NULL) {
		log.info ("Cannot get target name (error %x)", GetLastError());
		for (int i = 0; i < pcpcs->cbSerialization; i+=16) {
			std::string x;
			std::string s;
			char ach[4];
			for (int j = i; j < pcpcs->cbSerialization && j < i + 16; j++)
			{
				sprintf (ach, "%02x ", pcpcs->rgbSerialization[j]);
				x += ach;
				if (pcpcs->rgbSerialization[j] > 32)
					s += (char) pcpcs->rgbSerialization[j];
				else
					s += " ";
			}
			log.info ( "%04d   %s %s", i , x.c_str(), s.c_str());
		}
		return std::wstring();
	}
	else
	{
		log.info ("Target name %ls", ctx->TargetName->Buffer);
		log.info ("Message %ls", ctx->UIInfo->pszMessageText);
		return std::wstring(ctx->TargetName->Buffer);
	}
}

bool checkAuthenticationPackage (const char *package, unsigned long id) {
	ULONG authenticationPackage;
	HANDLE lsaHandle;

	LsaConnectUntrusted(&lsaHandle);

	LSA_STRING packageName;
	packageName.Buffer = (char*) package;
	packageName.Length = strlen (package);
	packageName.MaximumLength = packageName.Length + 1;
	LsaLookupAuthenticationPackage( lsaHandle, &packageName, &authenticationPackage);

	CloseHandle(lsaHandle);

//    m_log.info("Provider::Checking package %ls %lx==%lx", package, id, authenticationPackage);

    return authenticationPackage == id;
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
	if (pcpcs == NULL)
		return S_OK;

	wchar_t achFileName[4096];
	GetModuleFileNameW(NULL, achFileName, sizeof achFileName-1);
	PSEC_WINNT_CREDUI_CONTEXT pContext = NULL;

    m_log.info("Provider::SetSerialization");

    if (! checkAuthenticationPackage(MSV1_0_PACKAGE_NAME, pcpcs->ulAuthenticationPackage) &&
    		! checkAuthenticationPackage(MICROSOFT_KERBEROS_NAME_A, pcpcs->ulAuthenticationPackage) &&
			! checkAuthenticationPackage(NEGOSSP_NAME_A, pcpcs->ulAuthenticationPackage))
    {
    	m_log.info("Ignoring unknown authentication packange %lx", pcpcs->ulAuthenticationPackage);
    	ignore = true;
    	return S_OK;
    } else {
    	ignore = false;
    }

    m_autoCredentials.clear();

    std::wstring target = dumpSerialization(m_log, pcpcs);
    m_log.info ("target = %ls", target.c_str());
    if (target.length() > 5 && target.substr(0, 5) == std::wstring(L"HTTP/")) {
		m_domain =  target.substr(5).c_str() ;
   		loadCredentials();
    }
    if (target.length() > 7 && target.substr(0, 7) == std::wstring(L"http://")) {
		m_domain =  target.substr(7).c_str() ;
		int i = m_domain.find(L"/");
		if (i != m_domain.npos) m_domain = m_domain.substr(0, i);
		i = m_domain.find(L":");
		if (i != m_domain.npos) m_domain = m_domain.substr(0, i);
   		loadCredentials();
    }
    if (target.length() > 8 && target.substr(0, 8) == std::wstring(L"https://")) {
		m_domain =  target.substr(8).c_str() ;
		int i = m_domain.find(L"/");
		if (i != m_domain.npos) m_domain = m_domain.substr(0, i);
		i = m_domain.find(L":");
		if (i != m_domain.npos) m_domain = m_domain.substr(0, i);
   		loadCredentials();
    }

   	if (m_autoCredentials.empty())
   	{
   		m_domain = achFileName;
   		loadCredentials();
   	}

    return S_OK;
}


void SSOProvider::loadCredentials() {
	MZNSendDebugMessageA("WebTransport: Searching credentials for %ls", m_domain.c_str());
	m_log.info("Searching credentials for %ls", m_domain.c_str());
	std::vector<WebTransport*> rules = MZNWebTransportMatch();

    SecretStore ss(MZNC_getUserName());
    for (std::vector<WebTransport*>::iterator it = rules.begin(); it != rules.end(); it++) {
    	WebTransport* wt = *it;
    	std::wstring url = wt->url;
    	int i = url.find(L"://");
        m_log.info("Checking url %ls", url.c_str());
        bool match = false;
    	if ( i != url.npos) {
    		url = url.substr(i+3);
    		i = url.find(L"/");
    		if (i != url.npos) url = url.substr(0, i);
    		i = url.find(L":");
    		if (i != url.npos) url = url.substr(0, i);
            m_log.info("domain %ls", url.c_str());
    		if (url == m_domain) {
    			match = true;
    		}
    	}
    	if (!match) {
    		match = wcsicmp (url.c_str(), m_domain.c_str()) == 0;
    	}

    	if (match) {
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
	time_t now;
	time (&now);

    m_log.info("Provider::GetCredentialCount");
    if (ignore) {
    	*pdwCount = 0;
    	*pbAutoLogonWithDefault = 0;
        return S_OK;

    }
    else if (scenarioFlags & CREDUIWIN_ENUMERATE_ADMINS)
	    *pdwCount = 0; // This provider always has the same number of credentials.
	else {
		int count = m_autoCredentials.size();
		if (basic && count != 1) count++;
		*pdwCount = count;
	}
	*pdwDefault = 0;
	*pbAutoLogonWithDefault = m_autoCredentials.size() == 1 && now - lastLogin > 30;
	lastLogin = now;
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


