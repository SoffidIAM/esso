#ifndef SHIROPROVIDER_H_
#define SHIROPROVIDER_H_

#include <credentialprovider.h>
#include "Log.h"
#include <vector>

extern long g_nComObjsInUse;

class ShiroCredential;

class ShiroProvider: public ICredentialProvider {
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


	ShiroProvider();
	~ShiroProvider ();
	void notifySlotEvent ();
	void waitForSlot ();

private:

	static ShiroProvider* s_handler;
	HRESULT enumerateTokens ();


	static HWND getRootHwnd();
	long m_nRefCount;
	Log m_log;

	UINT_PTR m_upAdviseContext;
	CREDENTIAL_PROVIDER_USAGE_SCENARIO m_cpus;

	ShiroCredential * m_pCredential;
    ICredentialProviderEvents* m_credentialProviderEvents;

};


enum SHIRO_FIELD_ID
{
    SHI_IMAGE           = 0,
    SHI_TITLE			= 1,
    SHI_USER            = 2,
    SHI_PASSWORD        = 3,
    SHI_SUBMIT_BUTTON   = 4,
    SHI_MESSAGE         = 5,
    SHI_NUM_FIELDS      = 6,  // Note: if new fields are added, keep NUM_FIELDS last.  This is used as a count of the number of fields
};

static const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgShiroFieldDescriptors[] =
{
    { SHI_IMAGE, CPFT_TILE_IMAGE, (wchar_t*) L"Image" },
    { SHI_TITLE, CPFT_LARGE_TEXT, (wchar_t*) L"Iniciar com a Administrador" },
    { SHI_USER,  CPFT_EDIT_TEXT, (wchar_t*) L"Usuari" },
    { SHI_PASSWORD, CPFT_PASSWORD_TEXT, (wchar_t*) L"Contrasenya" },
    { SHI_MESSAGE, CPFT_LARGE_TEXT, (wchar_t*) L"Validant credencials...."},
    { SHI_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON, (wchar_t*) L"Iniciar" },
};


///////////////////////////////////////////////////////////

#endif
