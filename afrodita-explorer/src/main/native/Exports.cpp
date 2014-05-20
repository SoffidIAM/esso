#include <windows.h>
#include <stdio.h>
#include <psapi.h>

#include "CExplorerFactory.h"

#include <MazingerGuid.h>
#include <MazingerInternal.h>
#include "CExplorerFactory.h"

long g_nComObjsInUse = 0;

extern "C" {
LPSTR getMazingerDll();




HRESULT __stdcall DllGetClassObject(const CLSID & clsid, const IID & iid, void **ppv)
{
    //
    //Check if the requested COM object is implemented in this DLL
    //There can be more than 1 COM object implemented in a DLL
    //
//    MessageBox (NULL, "dllgetclassobject", "AFRODITA", MB_OK);
    if (clsid == CLSID_Mazinger) {
		//
		//iid specifies the requested interface for the factory object
		//The client can request for IUnknown, IClassFactory,
		//IClassFactory2
		//
		CExplorerFactory *pAddFact = new CExplorerFactory;
		if (pAddFact == NULL)
			return E_OUTOFMEMORY;
		else {
//			MessageBox (NULL, "INSTANCIADO", "AFRODITA", MB_OK);
			return pAddFact->QueryInterface(iid, ppv);
		}
	} else {

	}
    //
    //if control reaches here then that implies that the object
    //specified by the user is not implemented in this DLL
    //
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT __stdcall DllCanUnloadNow()
{
    //
    //A DLL is no longer in use when it is not managing any existing objects
    // (the reference count on all of its objects is 0).
    //We will examine the value of g_nComObjsInUse
    //
    if(g_nComObjsInUse == 0){
        return S_FALSE;
    }else{
        return S_FALSE;
    }
}

static char achPath[4096] = "XXXX";

LPSTR getMazingerDll()
{
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

BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason,
		LPVOID lpvReserved) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		hMazingerInstance = hinstDLL;
	}
	return TRUE;
}

}
