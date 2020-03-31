#ifndef CRecoverCREDENTIAL_H_
#define CRecoverCREDENTIAL_H_

#include <credentialprovider.h>
#include "RecoverProvider.h"
#include "CertificateHandler.h"
#include "Log.h"

class ConfigReader;

extern long g_nComObjsInUse;

class RecoverCredential: public ICredentialProviderCredential {
public:

	//IUnknown interface
	HRESULT __stdcall QueryInterface(REFIID riid, void **ppObj);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

    virtual HRESULT __stdcall Advise(
        /* [in] */ ICredentialProviderCredentialEvents *pcpce);

    virtual HRESULT __stdcall UnAdvise( void);

    virtual HRESULT __stdcall SetSelected(
        /* [out] */ BOOL *pbAutoLogon);

    virtual HRESULT __stdcall SetDeselected( void);

    virtual HRESULT __stdcall GetFieldState(
        /* [in] */ DWORD dwFieldID,
        /* [out] */ CREDENTIAL_PROVIDER_FIELD_STATE *pcpfs,
        /* [out] */ CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE *pcpfis);

    virtual HRESULT __stdcall GetStringValue(
        /* [in] */ DWORD dwFieldID,
        /* [string][out] */ LPWSTR *ppsz);

    virtual HRESULT __stdcall GetBitmapValue(
        /* [in] */ DWORD dwFieldID,
        /* [out] */ HBITMAP *phbmp);

    virtual HRESULT __stdcall GetCheckboxValue(
        /* [in] */ DWORD dwFieldID,
        /* [out] */ BOOL *pbChecked,
        /* [string][out] */ LPWSTR *ppszLabel);

    virtual HRESULT __stdcall GetSubmitButtonValue(
        /* [in] */ DWORD dwFieldID,
        /* [out] */ DWORD *pdwAdjacentTo);

    virtual HRESULT __stdcall GetComboBoxValueCount(
        /* [in] */ DWORD dwFieldID,
        /* [out] */ DWORD *pcItems,
        /* [out] */ DWORD *pdwSelectedItem);

    virtual HRESULT __stdcall GetComboBoxValueAt(
        /* [in] */ DWORD dwFieldID,
        DWORD dwItem,
        /* [string][out] */ LPWSTR *ppszItem);

    virtual HRESULT __stdcall SetStringValue(
        /* [in] */ DWORD dwFieldID,
        /* [string][in] */ LPCWSTR psz);

    virtual HRESULT __stdcall SetCheckboxValue(
        /* [in] */ DWORD dwFieldID,
        /* [in] */ BOOL bChecked);

    virtual HRESULT __stdcall SetComboBoxSelectedValue(
        /* [in] */ DWORD dwFieldID,
        /* [in] */ DWORD dwSelectedItem);

    virtual HRESULT __stdcall CommandLinkClicked(
        /* [in] */ DWORD dwFieldID);

    virtual HRESULT __stdcall GetSerialization(
        /* [out] */ CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE *pcpgsr,
        /* [out] */ CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcs,
        /* [out] */ LPWSTR *ppszOptionalStatusText,
        /* [out] */ CREDENTIAL_PROVIDER_STATUS_ICON *pcpsiOptionalStatusIcon);

    virtual HRESULT __stdcall ReportResult(
        /* [in] */ NTSTATUS ntsStatus,
        /* [in] */ NTSTATUS ntsSubstatus,
        /* [out] */ LPWSTR *ppszOptionalStatusText,
        /* [out] */ CREDENTIAL_PROVIDER_STATUS_ICON *pcpsiOptionalStatusIcon);



	RecoverCredential(const std::wstring &domain);

	virtual ~RecoverCredential ();

	void setUsage ( CREDENTIAL_PROVIDER_USAGE_SCENARIO usage) {
		m_usage = usage;
	}

private:
	HRESULT GenerateLoginSerialization(
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon);

	long m_nRefCount;
	Log m_log;
    ICredentialProviderCredentialEvents* m_pCredProvCredentialEvents;

    CREDENTIAL_PROVIDER_FIELD_STATE 	  m_cpfs [REC_NUM_FIELDS];
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE m_cpfis[REC_NUM_FIELDS];
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR  m_rgCredProvFieldDescriptors[REC_NUM_FIELDS];
    wchar_t*                              m_rgFieldStrings[REC_NUM_FIELDS];


    std::wstring requestId;
    std::wstring user;
    std::wstring domain;
    std::wstring windowsDomain;
    CREDENTIAL_PROVIDER_USAGE_SCENARIO m_usage;

    std::vector<std::wstring> m_questions;
    std::vector<std::wstring> m_answers;

    int currentQuestion;
    std::wstring desiredPassword1;
    std::wstring desiredPassword2;

    boolean performRequest ();
    boolean responseChallenge ();
    boolean resetPassword ();

    void updateInterface ();
};


///////////////////////////////////////////////////////////

#endif
