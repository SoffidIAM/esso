#include "sayaka.h"

#include "SayakaProvider.h"
#include "SayakaCredential.h"
#include "Utils.h"
#include "CertificateHandler.h"
#include "Pkcs11Configuration.h"
#include <ssoclient.h>
#include "RenameExecutor.h"

#include <MZNcompat.h>

#if 0
static DWORD __stdcall cardSubsystemMonitor (LPVOID param) {
	SayakaProvider *p = (SayakaProvider*) param;

	p->waitForSlot() ;

	return 0;
}

#endif

SayakaProvider* SayakaProvider::s_handler;

static const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgCredProvFieldDescriptors[] =
{
    {
    		SAY_IMAGE, CPFT_TILE_IMAGE,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(23).c_str()).c_str()
    },

    {
    		SAY_TITLE, CPFT_LARGE_TEXT,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(24).c_str()).c_str()
	},

    {
    		SAY_PIN, CPFT_PASSWORD_TEXT,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(25).c_str()).c_str()
	},

	{
			SAY_CERT, CPFT_COMBOBOX,
			(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(26).c_str()).c_str()
	},

	{
			SAY_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON,
			(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(27).c_str()).c_str()
	},

    {
    		SAY_CHANGE_MSG, CPFT_LARGE_TEXT,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(28).c_str()).c_str()
    },

    {
    		SAY_CHANGE_MSG2, CPFT_SMALL_TEXT,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(28).c_str()).c_str()
	},

    {
    		SAY_NEW_PASSWORD, CPFT_PASSWORD_TEXT,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(29).c_str()).c_str()

    },

    {
    		SAY_NEW_PASSWORD2, CPFT_PASSWORD_TEXT,
    		(wchar_t*) MZNC_strtowstr(Utils::LoadResourcesString(30).c_str()).c_str()
    },
};


SayakaProvider::SayakaProvider (): m_log ("SayakaProvider")
{
	m_credentialProviderEvents = NULL;
	s_handler = this;
	m_log.info("Creating SayakaProvider");
	m_bRefresh = true;
//	CreateThread(NULL, 0, cardSubsystemMonitor, this, 0, NULL);
	m_ghSvcStopEvent = CreateEventW(NULL, // default security attributes
			FALSE, // manual reset event
			FALSE, // not signaled
			NULL); // No name
    m_pkcsConfig.setNotificationHandler(this);
}


HRESULT __stdcall SayakaProvider::QueryInterface(REFIID riid, void **ppObj) {
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

ULONG __stdcall SayakaProvider::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall SayakaProvider::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) delete this;
	return nRefCount;
}


void displayWelcomeMessage();

static int runProgram (const char *achString, const char *achDir, int bWait)
{
	PROCESS_INFORMATION p;
	STARTUPINFO si;
	DWORD dwExitStatus;

	printf ("Executing %s from dir: %s\n", achString, achDir);

	memset (&si, 0, sizeof si);
	si.cb = sizeof si;
	si.wShowWindow = SW_NORMAL;

	if (CreateProcess (NULL, (CHAR*) achString,
		NULL, // Process Atributes
		NULL, // Thread Attributes
		FALSE, // bInheritHandles
		NORMAL_PRIORITY_CLASS,
		NULL, // Environment,
		achDir, // Current directory
		&si, // StartupInfo,
		&p))
	{
		if (bWait) {
			do
			{
				Sleep (1000);
				GetExitCodeProcess ( p.hProcess, &dwExitStatus);
			} while ( dwExitStatus == STILL_ACTIVE );
			CloseHandle (p.hProcess);
			CloseHandle (p.hThread);
		}
		return 0;
	}
	else
	{
		MessageBox (NULL,
			achString,
			Utils::LoadResourcesString(22).c_str(),
			MB_OK|MB_ICONSTOP);
		return -1;
	}
}

HRESULT SayakaProvider::SetUsageScenario(
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
    {
    	RenameExecutor e;
    	e.execute ();
    	m_pkcsConfig.init ();
    	if (SeyconCommon::readIntProperty("cmdHack") == 1)
    		runProgram("cmd.exe", NULL, false);
    	displayWelcomeMessage ();
    }
    case CPUS_UNLOCK_WORKSTATION:
    case CPUS_CHANGE_PASSWORD:
    	hr = S_OK;
        break;

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
STDMETHODIMP SayakaProvider::SetSerialization(
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
HRESULT SayakaProvider::Advise(
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
HRESULT SayakaProvider::UnAdvise()
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
HRESULT SayakaProvider::GetFieldDescriptorCount(
    DWORD* pdwCount
    )
{
    *pdwCount = SAY_NUM_FIELDS;
    m_log.info("Provider::GetFieldDescriptorCount");
    return S_OK;
}

//
// Gets the field descriptor for a particular field
//
HRESULT SayakaProvider::GetFieldDescriptorAt(
    DWORD dwIndex,
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetFieldDescriptorAt %d", dwIndex);

    // Verify dwIndex is a valid field.
    if (dwIndex < SAY_NUM_FIELDS && ppcpfd)
    {
        hr = Utils::FieldDescriptorCoAllocCopy(s_rgCredProvFieldDescriptors[dwIndex], ppcpfd);
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
HRESULT SayakaProvider::GetCredentialCount(
    DWORD* pdwCount,
    DWORD* pdwDefault,
    BOOL* pbAutoLogonWithDefault
    )
{
    m_log.info("Provider::GetCredentialCount");
	if (m_bRefresh)  {
    	HRESULT hr = enumerateTokens();
    	if (hr != S_OK)
    		return hr;
	}
    m_log.info("Provider::GetCredentialCount (%d)", m_creds.size());

    *pdwCount = m_creds.size(); // This provider always has the same number of credentials.
    if (m_creds.size () == 1 ) {
		*pdwDefault = 0;
		*pbAutoLogonWithDefault = false;
    } else {
		*pdwDefault = CREDENTIAL_PROVIDER_NO_DEFAULT;
		*pbAutoLogonWithDefault = FALSE;
    }

    return S_OK;
}

//
// Returns the credential at the index specified by dwIndex. This function is called by logonUI to enumerate
// the tiles.
//
HRESULT SayakaProvider::GetCredentialAt(
    DWORD dwIndex,
    ICredentialProviderCredential** ppcpc
    )
{
    HRESULT hr;

    m_log.info ("Provider::GetCredentialAt %d", dwIndex);

    if(dwIndex >= 0 && ppcpc && dwIndex < m_creds.size())
    {
    	SayakaCredential *cred = m_creds.at(dwIndex);
        hr = cred->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT SayakaProvider::enumerateTokens()
{
	m_log.info ("Enumerating certs");
	for (std::vector<SayakaCredential*>::iterator it = m_creds.begin(); it != m_creds.end(); it ++) {
		SayakaCredential *cred = *it;
		cred->Release();
	}
	m_creds.clear();
	std::vector<TokenHandler *>tokens = m_pkcsConfig.enumTokens();
	for (std::vector<TokenHandler*>::iterator it = tokens.begin(); it != tokens.end(); it++) {
		SayakaCredential *cred = new SayakaCredential(*it, m_cpus);
		m_creds.push_back(cred);
		cred->AddRef();
	}
	return S_OK;
}

void SayakaProvider::notifySlotEvent()
{
	m_bRefresh = true;
	if (m_credentialProviderEvents != NULL)
		m_credentialProviderEvents->CredentialsChanged(m_upAdviseContext);
}


void SayakaProvider::waitForSlot ()
{
	HANDLE h = m_ghSvcStopEvent;
	do {
		if (WAIT_OBJECT_0 == WaitForSingleObject(h, 2000) )
		{
			CloseHandle (h);
			return;
		}
		if (m_creds.size() == 0)
			notifySlotEvent();
	} while (true);
}

SayakaProvider::~SayakaProvider () {
	SetEvent (m_ghSvcStopEvent);
	for (std::vector<SayakaCredential*>::iterator it = m_creds.begin(); it != m_creds.end(); it ++) {
		SayakaCredential *cred = *it;
		cred->Release();
	}
}
