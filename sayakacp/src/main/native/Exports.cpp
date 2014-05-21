#include "sayaka.h"
#include <psapi.h>

#include "SayakaFactory.h"
#include "ShiroFactory.h"

#include <SayakaGuid.h>

long g_nComObjsInUse = 0;

HINSTANCE hSayakaInstance;

IID IID_ICredentialProviderCredential = { 0x63913a93, 0x40c1, 0x481a, { 0x81,
		0x8d, 0x40, 0x72, 0xff, 0x8c, 0x70, 0xcc } };

IID IID_ICredentialProvider = { 0xd27c3481, 0x5a1c, 0x45b2, { 0x8a, 0xaa, 0xc2,
		0x0e, 0xbb, 0xe8, 0x22, 0x9e } };

extern "C" {
LPSTR getMazingerDll();

HRESULT __stdcall DllGetClassObject(const CLSID & clsid, const IID & iid,
		void **ppv) {
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

void __stdcall Register() {
	char achDllPath[4096];
	GetModuleFileName(hSayakaInstance, achDllPath, 4096);

	//As per COM guidelines, every self installable COM inprocess component
	//should export the function DllRegisterServer for printing the
	//specified information to the registry
	//
	//

	WCHAR *lpwszClsid;
	char szBuff[MAX_PATH] = "";
	char szClsid[MAX_PATH] = "", szInproc[MAX_PATH] = "";
	char szDescriptionVal[256] = "";

	StringFromCLSID(CLSID_Sayaka, &lpwszClsid);

	wsprintf(szClsid, "%S", lpwszClsid);
	wsprintf(szInproc, "%s\\%s\\%s", "clsid", szClsid, "InprocServer32");

	//
	//write the default value
	//
	wsprintf(szBuff, "%s", "Sayaka Credential Provider");
	wsprintf(szDescriptionVal, "%s\\%s", "clsid", szClsid);

	HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, NULL,//write to the "default" value
			REG_SZ, (void*) szBuff, lstrlen(szBuff));
	strcpy (szBuff, "Apartment");
	HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, "ThreadingModel",
			REG_SZ, (void*) szBuff, lstrlen(szBuff));

	//
	//write the "InprocServer32" key data
	//
	HelperWriteKey(HKEY_CLASSES_ROOT, szInproc, NULL,//write to the "default" value
			REG_SZ, (void*) achDllPath, lstrlen(achDllPath));


	wsprintf(
			szBuff,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\%s",
			szClsid);
	strcpy(szDescriptionVal, "Sayaka Credential Provider");
	HelperWriteKey(HKEY_LOCAL_MACHINE, szBuff, NULL, REG_SZ,
			(void*) szDescriptionVal, strlen(szDescriptionVal));

}

}
