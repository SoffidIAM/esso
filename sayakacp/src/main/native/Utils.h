/*
 * Utils.h
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */

#ifndef UTILS_H_
#define UTILS_H_

#define SECURITY_WIN32

#include <string>
#include <credentialprovider.h>
#include <security.h>
// #include <ntsecapi7.h>


#include <ntsecapi.h>

class Utils {
public:
	Utils();
	virtual ~Utils();

	static void bstr2str (std::string &str, BSTR bstr);
	static BSTR str2bstr (const char* str);
	static BSTR str2bstr (const wchar_t* str);
	static HRESULT duplicateString (wchar_t* &szTarget, const wchar_t* wsz);
	static HRESULT duplicateString (wchar_t* &szTarget, const char* sz);
	static void freeString (wchar_t* &wsz);

	static HRESULT generateKerberosInteractiveLogon(
			LPCWSTR wszUser,
			LPCWSTR wszPassword,
			LPCWSTR wszDomain,
			CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
			KERB_INTERACTIVE_UNLOCK_LOGON &kil);
	static HRESULT RetrieveNegotiateAuthPackage(ULONG * pulAuthPackage);
	static HRESULT KerbInteractiveLogonPack(
	    const KERB_INTERACTIVE_UNLOCK_LOGON& rkil,
	    BYTE** prgb,
	    DWORD* pcb
	    );
	static HRESULT FieldDescriptorCoAllocCopy(
	    const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR& rcpfd,
	    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
	    );


	static bool changePassword (wchar_t *wchDomain, wchar_t *wchUser, wchar_t*wchPass, wchar_t *wchNewPassword,
			PWSTR* szStatusText,
			CREDENTIAL_PROVIDER_STATUS_ICON* pStatusIcon);

	/** Load language messages from DLL
		 *
		 * Method that loads the specified language string defined in resources of DLL.
		 * @param id Resource ID to load.
		 * @return Message loaded from DLL.
		 */
		static  std::string LoadResourcesString (int id);
};

#endif /* UTILS_H_ */
