#include "sayaka.h"
#include <psapi.h>

#include "cert/SayakaFactory.h"
#include "admin/ShiroFactory.h"
#include "recover/RecoverFactory.h"
#include "sso/SSOFactory.h"
#include "soffid/SoffidFactory.h"

#include <SayakaGuid.h>
#include "RenameExecutor.h"

#include "Log.h"

#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>

long g_nComObjsInUse = 0;

HINSTANCE hSayakaInstance;

IID IID_ICredentialProviderCredential = { 0x63913a93, 0x40c1, 0x481a, { 0x81, 0x8d, 0x40, 0x72, 0xff, 0x8c, 0x70, 0xcc } };

IID IID_IConnectableCredentialProviderCredential = { 0x9387928b, 0xac75, 0x4bf9, {0x8a, 0xb2, 0x2b, 0x93, 0xc4, 0xa5, 0x52, 0x90}};

IID IID_ICredentialProvider = { 0xd27c3481, 0x5a1c, 0x45b2, { 0x8a, 0xaa, 0xc2,
		0x0e, 0xbb, 0xe8, 0x22, 0x9e } };

// Generic credential provider {25CBB996-92ED-457e-B28C-4774084BD562}
IID CLSID_GenericCredentialProvider = { 0x25CBB996, 0x92ED, 0x457e, {0xB2, 0x8C, 0x47, 0x74, 0x08, 0x4B, 0xD5, 0x62}};

extern "C" {
LPSTR getMazingerDll();

HRESULT __stdcall DllGetClassObject(const CLSID & clsid, const IID & iid,
		void **ppv) {

	Log log("exports");
	wchar_t ach[128];

	StringFromGUID2(clsid, ach, sizeof ach);
	log.info ("Query Class %ls", ach);

	if (clsid == CLSID_Sayaka) {
		SayakaFactory *pAddFact = new SayakaFactory;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
			return pAddFact->QueryInterface(iid, ppv);
		}
	} else if (clsid == CLSID_ShiroKabuto) {
		ShiroFactory *pAddFact = new ShiroFactory;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
			return pAddFact->QueryInterface(iid, ppv);
		}

	} else if (clsid == CLSID_Recover) {
		RecoverFactory *pAddFact = new RecoverFactory;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
			return pAddFact->QueryInterface(iid, ppv);
		}
	} else if (clsid == CLSID_SSO ) {
		SSOFactory *pAddFact = new SSOFactory;
		pAddFact -> basic = false;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
			return pAddFact->QueryInterface(iid, ppv);
		}
	} else if (clsid == CLSID_SSO_BASIC ) {
		SSOFactory *pAddFact = new SSOFactory;
		pAddFact -> basic = true;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
			return pAddFact->QueryInterface(iid, ppv);
		}
	} else if (clsid == CLSID_Soffid) {
		SoffidFactory *pAddFact = new SoffidFactory;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
			return pAddFact->QueryInterface(iid, ppv);
		}

	}
	//
	//if control reaches here then that implies that the object
	//specified by the user is not implemented in this DLL
	//
	return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT __stdcall DllCanUnloadNow() {
	//
	//A DLL is no longer in use when it is not managing any existing objects
	// (the reference count on all of its objects is 0).
	//We will examine the value of g_nComObjsInUse
	//
	if (g_nComObjsInUse == 0) {
		return S_FALSE;
	} else {
		return S_FALSE;
	}
}

static char achPath[4096] = "XXXX";

LPSTR getMazingerDll() {
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ, &hKey)
			== ERROR_SUCCESS) {
		DWORD dwType;
		DWORD size = -150 + sizeof achPath;
		RegQueryValueEx(hKey, "ProgramFilesDir", NULL, &dwType,
				(LPBYTE) achPath, &size);
		RegCloseKey(hKey);
		strcat(achPath, "\\SoffidESSO\\MazingerHook.dll");
		return achPath;
	} else {
		return FALSE;
	}
}

BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hSayakaInstance = hinstDLL;
		Log l("DllMain");
		l.info ("*******\n\n\n");
		l.info ("Started\n\n\n");
    	RenameExecutor e;
    	e.execute ();
	}
	if (dwReason == DLL_PROCESS_DETACH) {
		Log l("DllMain");
		l.info ("Stopped\n\n\n");
		l.info ("*******");
	}
	return TRUE;
}


BOOL HelperWriteKey(HKEY roothk, const char *lpSubKey, LPCTSTR val_name,
		DWORD dwType, void *lpvData, DWORD dwDataSize) {
	//
	//Helper function for doing the registry write operations
	//
	//roothk:either of HKCR, HKLM, etc

	//lpSubKey: the key relative to 'roothk'

	//val_name:the key value name where the data will be written

	//dwType:the type of data that will be written ,REG_SZ,REG_BINARY, etc.

	//lpvData:a pointer to the data buffer

	//dwDataSize:the size of the data pointed to by lpvData
	//
	//

	HKEY hk;
	if (ERROR_SUCCESS != RegCreateKey(roothk, lpSubKey, &hk))
		return FALSE;

	if (ERROR_SUCCESS != RegSetValueEx(hk,val_name, 0,dwType, (CONST BYTE *)lpvData, dwDataSize))
		return FALSE;

	if (ERROR_SUCCESS != RegCloseKey(hk))
		return FALSE;
	return TRUE;

}

static void registerClsid ( const GUID &guid, const char *description)
{
	char achDllPath[4096];
	GetModuleFileName(hSayakaInstance, achDllPath, 4096);

	WCHAR *lpwszClsid;
	char szBuff[MAX_PATH] = "";
	char szClsid[MAX_PATH] = "", szInproc[MAX_PATH] = "";
	char szDescriptionVal[256] = "";

	StringFromCLSID(guid, (wchar_t **)&lpwszClsid);

	wsprintf(szClsid, "%S", lpwszClsid);
	wsprintf(szInproc, "%s\\%s\\%s", "clsid", szClsid, "InprocServer32");

	//
	//write the default value
	//
	wsprintf(szBuff, "%s", description);
	wsprintf(szDescriptionVal, "%s\\%s", "clsid", szClsid);

	HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, NULL,//write to the "default" value
			REG_SZ, (void*) szBuff, lstrlen(szBuff));

	//
	//write the "InprocServer32" key data
	//
	HelperWriteKey(HKEY_CLASSES_ROOT, szInproc, NULL,//write to the "default" value
			REG_SZ, (void*) achDllPath, lstrlen(achDllPath));

	strcpy (szBuff, "Apartment");
	HelperWriteKey(HKEY_CLASSES_ROOT, szInproc, "ThreadingModel",
			REG_SZ, (void*) szBuff, lstrlen(szBuff));

	wsprintf(
			szBuff,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\%s",
			szClsid);
	strcpy(szDescriptionVal, description);
	HelperWriteKey(HKEY_LOCAL_MACHINE, szBuff, NULL, REG_SZ,
			(void*) szDescriptionVal, strlen(szDescriptionVal));

}

void notifyError2(DWORD lastError)
{
	LPSTR pstr;
	char errorMsg[255];

	int fm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM, 	NULL, lastError, 0, (LPSTR) &pstr, 0, NULL);

	// Format message failure
	if (fm == 0)
	{
		sprintf(errorMsg, "Unknown error: %d\n", lastError);
	}

	else
	{
	}

	LocalFree(pstr);
}

void notifyError() {
	notifyError2(GetLastError());
}


DWORD getCurrentUserSid(PSID* pSid) {
	HANDLE hToken;
	OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) ;
	//
	// Get the size of the memory buffer needed for the SID
	//
	DWORD dwBufferSize = 0;
	PTOKEN_USER token = NULL;
	while (true) {
		if (GetTokenInformation( hToken, TokenUser, (void*)token, dwBufferSize, &dwBufferSize ))
			break;
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
			if (token != NULL)
				free (token);
			token =(PTOKEN_USER) malloc(dwBufferSize);
		} else {
			return GetLastError();
		}
	}
	*pSid = token->User.Sid;
	return ERROR_SUCCESS;
}

BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue(
            NULL,            // lookup privilege on local system
            lpszPrivilege,   // privilege to lookup
            &luid ) )        // receives LUID of privilege
    {
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
           hToken,
           FALSE,
           &tp,
           sizeof(TOKEN_PRIVILEGES),
           (PTOKEN_PRIVILEGES) NULL,
           (PDWORD) NULL) )
    {
          return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
          return FALSE;
    }

    return TRUE;
}

DWORD getRegistryAcl (LPCSTR registryKey, PACL *pAcl) {
	HKEY hKey;
	DWORD r;
	// {25CBB996-92ED-457e-B28C-4774084BD562}
	r = RegOpenKeyEx(HKEY_CLASSES_ROOT,
			registryKey, 0,
			KEY_READ|READ_CONTROL, &hKey);
	if (r != ERROR_SUCCESS) {
		notifyError();
		return r;
	}

	SECURITY_INFORMATION si;
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
	DWORD lpcbSecurityDescriptor = 0;
	while (ERROR_INSUFFICIENT_BUFFER == (r = RegGetKeySecurity(hKey, OWNER_SECURITY_INFORMATION| DACL_SECURITY_INFORMATION,
		(LPVOID) pSecurityDescriptor, &lpcbSecurityDescriptor)))
	{
		if (pSecurityDescriptor != NULL)
			free(pSecurityDescriptor);
		pSecurityDescriptor = malloc (lpcbSecurityDescriptor);

	}

	if (r != ERROR_SUCCESS) {
		return r;
	}
	if (!IsValidSecurityDescriptor(pSecurityDescriptor)) {
		return r;
	}

	PACL dacl = NULL;
	BOOL daclPresent;
	BOOL daclDefault;
	if (!GetSecurityDescriptorDacl(pSecurityDescriptor, &daclPresent, &dacl, &daclDefault))
		return GetLastError();

	RegCloseKey(hKey);
	*pAcl = dacl;
	return ERROR_SUCCESS;
}

DWORD changeOwnership (LPCSTR lpszRegistryKey) {
	BOOL bRetval = FALSE;

	HANDLE hToken = NULL;
	PSID pSIDAdmin = NULL;
	PSID pSIDEveryone = NULL;
	PACL pACL = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
			SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	ULONG cEntries;
	EXPLICIT_ACCESS *prevEa;
	EXPLICIT_ACCESS *ea;
	DWORD dwRes;

	std::string reg = "CLASSES_ROOT\\";
	reg += lpszRegistryKey;

	PACL prevAcl = NULL;
	dwRes = getRegistryAcl(lpszRegistryKey, &prevAcl);
	if (dwRes != ERROR_SUCCESS) {
		return dwRes;
	}

	if ( GetExplicitEntriesFromAcl(prevAcl, &cEntries, &prevEa) != ERROR_SUCCESS) {
		notifyError();
		goto Cleanup;
	}

	ea = (EXPLICIT_ACCESS*)malloc((1+cEntries) * sizeof (EXPLICIT_ACCESS) );
	ZeroMemory(ea, (1+cEntries) * sizeof(EXPLICIT_ACCESS));
	for (int i = 0; i < cEntries; i++) {
		ea[i] = prevEa[i];
		if (ea[i].Trustee.TrusteeForm == TRUSTEE_IS_SID) {
			LPSTR str = NULL;
			ConvertSidToStringSid(ea[i].Trustee.ptstrName, &str);
		}
	}
	// Set full control for myself.
	PSID myself;
	if (getCurrentUserSid(&myself) != ERROR_SUCCESS) {
		notifyError();
		goto Cleanup;
	}
	ea[cEntries].grfAccessPermissions = GENERIC_ALL;
	ea[cEntries].grfAccessMode = SET_ACCESS;
	ea[cEntries].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea[cEntries].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[cEntries].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[cEntries].Trustee.ptstrName = (LPTSTR) myself;

	if (ERROR_SUCCESS != SetEntriesInAcl(cEntries+1,
										 ea,
										 NULL,
										 &pACL))
	{
		goto Cleanup;
	}

	// Try to modify the object's DACL.
	dwRes = SetNamedSecurityInfo(
		(LPSTR) reg.c_str(),                 // name of the object
		SE_REGISTRY_KEY,              // type of object
		DACL_SECURITY_INFORMATION,   // change only the object's DACL
		NULL, NULL,                  // do not change owner or group
		pACL,                        // DACL specified
		NULL);                       // do not change SACL

	if (ERROR_SUCCESS == dwRes)
	{
		// No more processing needed.
		goto Cleanup;
	}
	if (dwRes != ERROR_ACCESS_DENIED)
	{
		notifyError2(dwRes);
		goto Cleanup;
	}

	// If the preceding call failed because access was denied,
	// enable the SE_TAKE_OWNERSHIP_NAME privilege, create a SID for
	// the Administrators group, take ownership of the object, and
	// disable the privilege. Then try again to set the object's DACL.

	// Open a handle to the access token for the calling process.
	if (!OpenProcessToken(GetCurrentProcess(),
						  TOKEN_ADJUST_PRIVILEGES,
						  &hToken))
	{
		dwRes = GetLastError();
		goto Cleanup;
	}

	// Enable the SE_TAKE_OWNERSHIP_NAME privilege.
	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
	{
		dwRes = GetLastError();
		goto Cleanup;
	}

	// Set the owner in the object's security descriptor.
	dwRes = SetNamedSecurityInfo(
		(LPSTR) reg.c_str(),                 // name of the object
		SE_REGISTRY_KEY,              // type of object
		OWNER_SECURITY_INFORMATION,  // change only the object's owner
		myself,                   // SID of Administrator group
		NULL,
		NULL,
		NULL);

	if (dwRes != ERROR_SUCCESS)
	{
		dwRes = GetLastError();
		goto Cleanup;
	}

	// Disable the SE_TAKE_OWNERSHIP_NAME privilege.
	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE))
	{
		dwRes = GetLastError();
		goto Cleanup;
	}

	// Try again to modify the object's DACL,
	// now that we are the owner.
	dwRes = SetNamedSecurityInfo(
		(LPSTR) reg.c_str(),                 // name of the object
		SE_REGISTRY_KEY,              // type of object
		DACL_SECURITY_INFORMATION,   // change only the object's DACL
		NULL, NULL,                  // do not change owner or group
		pACL,                        // DACL specified
		NULL);                       // do not change SACL

	if (dwRes == ERROR_SUCCESS)
	{
//		log("Successfully changed DACL\n");
	}
	else
	{
	}

Cleanup:

	if (pSIDAdmin)
		FreeSid(pSIDAdmin);

	if (pSIDEveryone)
		FreeSid(pSIDEveryone);

	if (pACL)
	   LocalFree(pACL);

	if (hToken)
	   CloseHandle(hToken);

	return dwRes;

}

void overrideBasicCredentialProvider () {
	HKEY hKey;
	DWORD r;

	r = RegOpenKeyEx(HKEY_CLASSES_ROOT,
				"Clsid\\{25CBB996-92ED-457e-B28C-4774084BD562}", 0,
				KEY_ALL_ACCESS, &hKey);
	if (r != ERROR_SUCCESS) {
		r = changeOwnership("Clsid\\{25CBB996-92ED-457e-B28C-4774084BD562}");
		if ( r != ERROR_SUCCESS)
		{
			return;
		}
		r = RegOpenKeyEx(HKEY_CLASSES_ROOT,
					"Clsid\\{25CBB996-92ED-457e-B28C-4774084BD562}", 0,
					KEY_ALL_ACCESS, &hKey);
	}

	if (r == ERROR_SUCCESS)
	{
		HKEY hKey2;
		r = RegCreateKeyExA(hKey, "TreatAs", 0, (LPSTR) "",
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
				&hKey2, NULL);
		if (r != ERROR_SUCCESS) {
			notifyError2(r);
			return;
		}
		std::string target = "{44ee45c8-529b-4542-98ed-cfeceab1d4cc}";
		RegSetValueA(hKey, "TreatAs", REG_SZ, target.c_str(), target.length());
		RegCloseKey ( hKey2);
		RegCloseKey ( hKey);
	}
}

void __stdcall Register() {

	registerClsid (CLSID_Sayaka, "Sayaka Credential Provider");
	registerClsid (CLSID_ShiroKabuto, "Shiro Kabutgo Credential Provider");
	registerClsid (CLSID_Recover, "Sayaka Recover Credential Provider");
	registerClsid (CLSID_SSO, "Sayaka SSO Credential Provider");
	registerClsid (CLSID_SSO_BASIC, "Sayaka SSO Basic Credential Provider");
	registerClsid (CLSID_Soffid, "Sayaka Soffid Credential Provider");
	overrideBasicCredentialProvider();
}


}


