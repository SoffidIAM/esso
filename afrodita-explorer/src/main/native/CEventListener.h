#ifndef CEVENT_LISTENER_H_
#define CEVENT_LISTENER_H_

#include <ocidl.h>
#include <exdisp.h>
#include <oleauto.h>
#include <dispex.h>

#include <WebListener.h>
#include "ExplorerElement.h"


class CEventListener :
        public IDispatchEx
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

	// IDispatchEx interface
    virtual HRESULT STDMETHODCALLTYPE GetDispID(
        /* [in] */  BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */  DISPID *pid) ;

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE InvokeEx(
          DISPID id,
          LCID lcid,
          WORD wFlags,
          DISPPARAMS *pdp,
          VARIANT *pvarRes,
          EXCEPINFO *pei,
          IServiceProvider *pspCaller);

    virtual HRESULT STDMETHODCALLTYPE DeleteMemberByName(
        /* [in] */  BSTR bstrName,
        /* [in] */ DWORD grfdex) ;

    virtual HRESULT STDMETHODCALLTYPE DeleteMemberByDispID(
        /* [in] */ DISPID id) ;

    virtual HRESULT STDMETHODCALLTYPE GetMemberProperties(
        /* [in] */ DISPID id,
        /* [in] */ DWORD grfdexFetch,
        /* [out] */  DWORD *pgrfdex) ;

    virtual HRESULT STDMETHODCALLTYPE GetMemberName(
        /* [in] */ DISPID id,
        /* [out] */  BSTR *pbstrName);

    virtual HRESULT STDMETHODCALLTYPE GetNextDispID(
        /* [in] */ DWORD grfdex,
        /* [in] */ DISPID id,
        /* [out] */  DISPID *pid);

    virtual HRESULT STDMETHODCALLTYPE GetNameSpaceParent(
        /* [out] */  IUnknown **ppunk);

    //
    void execute (DISPPARAMS *params);

	CEventListener ();
	virtual ~CEventListener ();
	void connect (ExplorerElement *pElement, const char*event, WebListener *listener);
	void connectRefresh (ExplorerWebApplication *app);

	std::string m_event;
	WebListener *m_listener;

private:
	ExplorerElement * m_pElement;
	ExplorerWebApplication *m_pApplication;
	bool isAddEventListener;

	DWORD m_dwCookie;
	LONG m_nRefCount;   //for managing the reference count

	bool preventLoop;

};

///////////////////////////////////////////////////////////

#endif
