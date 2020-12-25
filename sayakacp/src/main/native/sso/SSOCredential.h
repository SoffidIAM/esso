#ifndef SSOOCREDENTIAL_H_
#define SSOCREDENTIAL_H_

#include <credentialprovider.h>
#include "SSOProvider.h"
#include "Log.h"



class SSOCredential: public ICredentialProviderCredential {
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



	SSOCredential();

	std::wstring szUser;
	std::wstring szPassword;
	bool autoLogin;

	void updateLabel();
private:
	bool ObtainCredentials (
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon);

	HRESULT GenerateLoginSerialization(
		CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
		CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
		PWSTR* ppwzOptionalStatusText,
		CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon);

    ICredentialProviderCredentialEvents* m_pCredProvCredentialEvents;

    CREDENTIAL_PROVIDER_FIELD_STATE 	  m_cpfs [SHI_NUM_FIELDS];
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE m_cpfis[SHI_NUM_FIELDS];
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR  m_rgCredProvFieldDescriptors[SHI_NUM_FIELDS];
    wchar_t*                              m_rgFieldStrings[SHI_NUM_FIELDS];
	long m_nRefCount;
	Log m_log;
	void displayMessage();
	void hideMessage();


};


///////////////////////////////////////////////////////////

#endif
