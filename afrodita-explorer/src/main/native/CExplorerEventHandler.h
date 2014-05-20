#ifndef CSAYAKAEVENTHANDLER_H_
#define CSAYAKAEVENTHANDLER_H_

#include <ocidl.h>
#include <exdisp.h>
#include <oleauto.h>

class ConfigReader;
extern long g_nComObjsInUse;

class CExplorerEventHandler :
        public DWebBrowserEvents2
    {
    public:

    //IUnknown interface
    HRESULT __stdcall QueryInterface(
                                REFIID riid ,
                                void **ppObj);
    ULONG   __stdcall AddRef();
    ULONG   __stdcall Release();

	// IDispatch interface
	STDMETHOD(GetTypeInfoCount)(THIS_ UINT*) ;
	STDMETHOD(GetTypeInfo)(THIS_ UINT,LCID,LPTYPEINFO*);
	STDMETHOD(GetIDsOfNames)(THIS_ REFIID,LPOLESTR*,UINT,LCID,DISPID*);
	STDMETHOD(Invoke)(THIS_ DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);

	CExplorerEventHandler ();
	void connect (IWebBrowser2 *pBrowser);

private:
	void onLoad(IWebBrowser2 *pBrowser, const char *url);

	IWebBrowser2* m_pBrowser;
	DWORD m_dwCookie;
	long m_nRefCount;   //for managing the reference count

};

///////////////////////////////////////////////////////////

#endif
