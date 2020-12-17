#ifndef SSOPROVIDER_H_
#define SSOPROVIDER_H_

#include <credentialprovider.h>
#include "Log.h"
#include <vector>

extern long g_nComObjsInUse;

class SSOCredential;

class SSOProvider: public ICredentialProvider {
public:
	//IUnknown interface
	HRESULT __stdcall QueryInterface(REFIID riid, void **ppObj);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

    virtual HRESULT __stdcall SetUsageScenario(
        /* [in] */ CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
        /* [in] */ DWORD dwFlags) ;

    virtual HRESULT __stdcall SetSerialization(
        /* [in] */ const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcs) ;

    virtual HRESULT __stdcall Advise(
        /* [in] */ ICredentialProviderEvents *pcpe,
        /* [in] */ UINT_PTR upAdviseContext) ;

    virtual HRESULT __stdcall UnAdvise( void) ;

    virtual HRESULT __stdcall GetFieldDescriptorCount(
        /* [out] */ DWORD *pdwCount) ;

    virtual HRESULT __stdcall GetFieldDescriptorAt(
        /* [in] */ DWORD dwIndex,
        /* [out] */ CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR **ppcpfd) ;

    virtual HRESULT __stdcall GetCredentialCount(
        /* [out] */ DWORD *pdwCount,
        /* [out] */ DWORD *pdwDefault,
        /* [out] */ BOOL *pbAutoLogonWithDefault) ;

    virtual HRESULT __stdcall GetCredentialAt(
        /* [in] */ DWORD dwIndex,
        /* [out] */ ICredentialProviderCredential **ppcpc) ;


	SSOProvider();
	~SSOProvider ();

	bool basic;
private:

	DWORD scenarioFlags;

	long m_nRefCount;
	Log m_log;

	UINT_PTR m_upAdviseContext;
	CREDENTIAL_PROVIDER_USAGE_SCENARIO m_cpus;

	SSOCredential * m_pCredential;
	std::vector<SSOCredential *> m_autoCredentials;
    ICredentialProviderEvents* m_credentialProviderEvents;

    std::wstring m_domain;
};


enum SSO_FIELD_ID
{
    SHI_IMAGE           = 0,
    SHI_TITLE			= 1,
    SHI_USER            = 2,
    SHI_PASSWORD        = 3,
    SHI_SUBMIT_BUTTON   = 4,
    SHI_MESSAGE         = 5,
    SHI_NUM_FIELDS      = 6,  // Note: if new fields are added, keep NUM_FIELDS last.  This is used as a count of the number of fields
};



///////////////////////////////////////////////////////////

#endif
