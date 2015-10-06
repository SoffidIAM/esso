#ifndef CEVENT_LISTENER_H_
#define CEVENT_LISTENER_H_

#include <ocidl.h>
#include <exdisp.h>
#include <oleauto.h>
#include <dispex.h>

#include <WebListener.h>
#include "ExplorerElement.h"


class CEventListener :
        public IDispatch
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


	CEventListener ();
	virtual ~CEventListener ();
	void connect (ExplorerElement *pElement, const char*event, WebListener *listener);
	void connectRefresh (ExplorerWebApplication *app);

	std::string m_event;
	WebListener *m_listener;

private:
	ExplorerElement * m_pElement;
	ExplorerWebApplication *m_pApplication;

	DWORD m_dwCookie;
	LONG m_nRefCount;   //for managing the reference count

};

///////////////////////////////////////////////////////////

#endif
