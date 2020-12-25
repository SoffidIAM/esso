#ifndef CSAYAKAPROVIDER_H_
#define CSAYAKAPROVIDER_H_

#include <credentialprovider.h>
#include "Log.h"
#include <vector>
#include "Pkcs11Configuration.h"

class Pkcs11Configuration;
class CertificateHandler;

extern long g_nComObjsInUse;

class SayakaCredential;

class SayakaProvider: public ICredentialProvider {
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


	SayakaProvider();
	~SayakaProvider ();
	void notifySlotEvent ();
	void waitForSlot ();

private:

	static SayakaProvider* s_handler;
	HRESULT enumerateTokens ();


	static HWND getRootHwnd();
	long m_nRefCount;
	Log m_log;

	Pkcs11Configuration m_pkcsConfig;
	std::vector<SayakaCredential *>m_creds;
	ICredentialProviderEvents *m_credentialProviderEvents;
	UINT_PTR m_upAdviseContext;
	bool m_bRefresh;
	HANDLE 	m_ghSvcStopEvent ;
	CREDENTIAL_PROVIDER_USAGE_SCENARIO m_cpus;
};

enum SAYAKA_FIELD_ID
{
    SAY_IMAGE           			= 0,
    SAY_TITLE						= 1,
    SAY_PIN             				= 2,
    SAY_CERT            			= 3,
    SAY_SUBMIT_BUTTON	= 4,
    SAY_CHANGE_MSG      	= 5,
    SAY_CHANGE_MSG2     	= 6,
    SAY_NEW_PASSWORD    = 7,
    SAY_NEW_PASSWORD2  = 8,
    SAY_NUM_FIELDS      = 9,
    // Note: if new fields are added, keep NUM_FIELDS last.  This is used as a count of the number of fields
};

///////////////////////////////////////////////////////////

#endif
