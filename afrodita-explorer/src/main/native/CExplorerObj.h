#ifndef CSAYAKAOBJ_H_
#define CSAYAKAOBJ_H_

#include <ocidl.h>
#include <exdisp.h>
#include <oleauto.h>
#include "CExplorerEventHandler.h"

class ConfigReader;

extern long g_nComObjsInUse;

class CExplorerObj: public IObjectWithSite {
public:

	//IUnknown interface
	HRESULT __stdcall QueryInterface(REFIID riid, void **ppObj);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	//IObjectWithSite interface
	HRESULT __stdcall SetSite(IUnknown* pUnknown);
	HRESULT __stdcall GetSite(REFIID riid, void**pSite);

	CExplorerObj();

private:
	void connect();
	long m_nRefCount; //for managing the reference count
	IUnknown *m_pSite;
	IConnectionPointContainer *m_pCPC;
	IWebBrowser2* m_pWebBrowser;
	DWORD m_dwCookie;
	CExplorerEventHandler handler;

};

///////////////////////////////////////////////////////////

#endif
