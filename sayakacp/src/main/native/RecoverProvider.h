#ifndef CRecoverPROVIDER_H_
#define CRecoverPROVIDER_H_

#include <credentialprovider.h>
#include "Log.h"
#include <vector>
#include "Pkcs11Configuration.h"

class Pkcs11Configuration;
class CertificateHandler;

extern long g_nComObjsInUse;

class RecoverCredential;

class RecoverProvider: public ICredentialProvider {
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


	RecoverProvider();
	virtual ~RecoverProvider ();

private:

	static RecoverProvider* s_handler;

	static HWND getRootHwnd();
	long m_nRefCount;
	Log m_log;

	RecoverCredential *cred;
	ICredentialProviderEvents *m_credentialProviderEvents;
	UINT_PTR m_upAdviseContext;
	bool m_bRefresh;
	CREDENTIAL_PROVIDER_USAGE_SCENARIO m_cpus;
};

enum Recover_FIELD_ID
{
    REC_IMAGE           			= 0,
    REC_TITLE						= 1,
    REC_USER						= 2,
    REC_QUESTION             		= 3,
    REC_ANSWER          			= 4,
    REC_SUBMIT_BUTTON				= 5,
    REC_CHANGE_MSG      			= 6,
    REC_NEW_PASSWORD    			= 7,
    REC_NEW_PASSWORD2    			= 8,
    REC_NUM_FIELDS      			= 9,
    // Note: if new fields are added, keep NUM_FIELDS last.  This is used as a count of the number of fields
};

///////////////////////////////////////////////////////////

#endif
