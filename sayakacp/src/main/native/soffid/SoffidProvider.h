#ifndef SOFFIDPROVIDER_H_
#define SOFFIDPROVIDER_H_

#include <credentialprovider.h>
#include "Log.h"
#include <vector>

extern long g_nComObjsInUse;

class SoffidCredential;

class SoffidProvider: public ICredentialProvider {
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


	SoffidProvider();
	~SoffidProvider ();
	void notifySlotEvent ();
	void waitForSlot ();

private:

	static SoffidProvider* s_handler;
	HRESULT enumerateTokens ();


	static HWND getRootHwnd();
	long m_nRefCount;
	Log m_log;

	UINT_PTR m_upAdviseContext;
	CREDENTIAL_PROVIDER_USAGE_SCENARIO m_cpus;

	SoffidCredential * m_pCredential;
    ICredentialProviderEvents* m_credentialProviderEvents;

};


enum SOFFID_FIELD_ID
{
    SOF_IMAGE           = 0,
    SOF_TITLE			= 1,
    SOF_USER            = 2,
    SOF_PASSWORD        = 3,
    SOF_NEW_PASSWORD1   = 4,
    SOF_NEW_PASSWORD2   = 5,
    SOF_SUBMIT_BUTTON   = 6,
    SOF_MESSAGE         = 7,
	SOF_WARNING			= 8,
    SOF_NUM_FIELDS      = 9,  // Note: if new fields are added, keep NUM_FIELDS last.  This is used as a count of the number of fields
};



///////////////////////////////////////////////////////////

#endif
