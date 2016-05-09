#include <windows.h>
#include <stdio.h>
#include <exdispid.h>

#include "CEventListener.h"
#include "ExplorerWebApplication.h"
#include <MazingerInternal.h>
#include "Utils.h"
#include <olectl.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <string.h>


CEventListener::CEventListener ()
{
	m_pElement = NULL;
	m_listener = NULL;
	m_pApplication = NULL;
	preventLoop = false;
	isAddEventListener = true;
}

CEventListener::~CEventListener() {
	if (m_listener != NULL)
	{
		m_listener->release();

		IHTMLElement2 *e = NULL;
		HRESULT hr = m_pElement->getIDispatch()->QueryInterface(IID_IHTMLElement2, reinterpret_cast<void**>(&e));
		if (!FAILED(hr))
		{
			VARIANT_BOOL b;
			BSTR bstr = Utils::str2bstr(m_event.c_str());
			hr = e->detachEvent(bstr, dynamic_cast<IDispatch*>(this));
			e->Release();
		}
	}
	if (m_pElement != NULL)
		m_pElement -> release();
	if (m_pApplication != NULL)
		m_pApplication -> release();
}

void CEventListener::connect (ExplorerElement *pElement,const char *event, WebListener *listener)
{
	HRESULT hr;
	this -> m_pElement = pElement;
	this -> m_pElement->lock();
	this -> m_event = event;
	this -> m_listener = listener;
	listener->lock();

	std::string tagName;
	pElement->getTagName(tagName);
	// Try add event listener
	BSTR bstr = Utils::str2bstr("addEventListener");
	DISPID dispId = 0;
	hr = pElement->getIDispatch()->GetIDsOfNames(IID_NULL, &bstr, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
	if ( !FAILED(hr))
	{
		DISPPARAMS dp;
		VARIANTARG va[3];
		dp.cArgs = 3;
		dp.cNamedArgs = 0;
		dp.rgvarg = va;
		va[2].vt = VT_BSTR;
		va[2].bstrVal = Utils::str2bstr(event);
		va[1].vt = VT_DISPATCH;
		va[1].pdispVal = reinterpret_cast<IDispatch*> (this);
		va[0].vt = VT_BOOL;
		va[0].bVal = FALSE;
		VARIANT result;
		VariantInit(&result);
		UINT error;
		EXCEPINFO ei;
		ZeroMemory (&ei, sizeof ei);
//		MZNSendDebugMessageA("Invoking %s", "addEventListener");
		hr = pElement->getIDispatch()->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dp, &result, &ei, &error);
		if (FAILED(hr))
		{
			MZNSendDebugMessage("Error en %lX en attributes %d", (long) hr,  (int) error);
			MZNSendDebugMessage("Error code %d ", (int) ei.wCode);
			MZNSendDebugMessage("Source %ls", ei.bstrSource);
			MZNSendDebugMessage("Description %ls", ei.bstrDescription);
		}
		SysFreeString(va[2].bstrVal);
	} else {
//		MZNSendDebugMessage("Cannot get addEventListener: %lx", hr);
		isAddEventListener = false;
		BSTR bstr = Utils::str2bstr("attachEvent");
		DISPID dispId = 0;
		hr = pElement->getIDispatch()->GetIDsOfNames(IID_NULL, &bstr, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
//		MZNSendDebugMessageA("Looking for property %s on %s", "attachEvent", pElement->toString().c_str());
		if ( !FAILED(hr))
		{
			DISPPARAMS dp;
			VARIANTARG va[3];
			dp.cArgs = 2;
			dp.cNamedArgs = 0;
			dp.rgvarg = va;
			std::string event2 = std::string("on")+event;
			if (event2 == "oninput") event2 = "onchange";
			va[1].vt = VT_BSTR;
			va[1].bstrVal = Utils::str2bstr(event2.c_str());
			va[0].vt = VT_DISPATCH;
			va[0].pdispVal = reinterpret_cast<IDispatch*> (this);
			VARIANT result;
			VariantInit(&result);
			UINT error;
			EXCEPINFO ei;
			ZeroMemory (&ei, sizeof ei);
//			MZNSendDebugMessageA("Invoking %s", "attachEvent");
			hr = pElement->getIDispatch()->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dp, &result, &ei, &error);
			if (FAILED(hr))
			{
//				MZNSendDebugMessage("Error en %lX en attributes %d", (long) hr,  (int) error);
//				MZNSendDebugMessage("Error code %d ", (int) ei.wCode);
//				MZNSendDebugMessage("Source %ls", ei.bstrSource);
//				MZNSendDebugMessage("Description %ls", ei.bstrDescription);
			}
			SysFreeString(va[1].bstrVal);
		} else {
			MZNSendDebugMessage("Cannot get attachEvent: %lx", hr);
		}
	}
	SysFreeString(bstr);
}



// MIDL_INTERFACE("A6EF9860-C720-11d0-9337-00A0C90DCAA9")

static CLSID IID_SOFFID_IDispatchEx = {0xa6ef9860, 0xc720, 0x11d0, {0x93, 0x37, 0x0, 0xa0, 0xc9, 0xd, 0xca, 0xa9}};

HRESULT __stdcall CEventListener::QueryInterface(REFIID riid, void **ppObj) {

	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	} else if (riid == IID_IDispatch) {
		IDispatch *pDispatch = reinterpret_cast<IDispatch*> (this);
		*ppObj = static_cast<void*> (pDispatch);
		AddRef();
		return S_OK;
	} else if (riid == IID_SOFFID_IDispatchEx) {
		IDispatchEx *pDispatchEx = reinterpret_cast<IDispatchEx*> (this);
		*ppObj = static_cast<void*> (pDispatchEx);
		AddRef();
		return S_OK;
	} else if (riid == DIID_DWebBrowserEvents2) {
//		MZNSendDebugMessage("Query DIID_DWebBrowserEvents2 interface");
		DWebBrowserEvents2 *pDispatch = reinterpret_cast<DWebBrowserEvents2*> (this);
		*ppObj = static_cast<void*> (pDispatch);
		AddRef();
		return S_OK;
	} else {
		BSTR bstr = NULL;
		StringFromCLSID(riid, &bstr);
	}

	//
	//if control reaches here then , let the client know that
	//we do not satisfy the required interface
	//
	*ppObj = NULL;
	return E_NOINTERFACE;
}



ULONG __stdcall CEventListener::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall CEventListener::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) delete this;
	return nRefCount;
}


// Implementacion de IDispatch
HRESULT __stdcall CEventListener::GetTypeInfoCount (UINT*n) {
	MZNSendDebugMessage("GetTypeInfoCount");
	*n = 0;
	return S_OK ;
}

HRESULT __stdcall CEventListener::GetTypeInfo(UINT num, LCID id,LPTYPEINFO* info)
{
	MZNSendDebugMessage("GetTypeInfo %d", (int) num);
	return E_NOTIMPL;
}

HRESULT __stdcall CEventListener::GetIDsOfNames(REFIID ID,LPOLESTR* bstr,UINT num ,LCID id ,DISPID* pDispId) {
	return E_NOTIMPL;
}

HRESULT __stdcall CEventListener::Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr) {

	if(!IsEqualIID(riid,IID_NULL)) return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL

	execute (pDispParams);
	return S_OK;
}

HRESULT CEventListener::InvokeEx(DISPID id, LCID lcid, WORD wFlags,
		DISPPARAMS* pDispParams, VARIANT* pvarRes, EXCEPINFO* pei,
		IServiceProvider* pspCaller) {
	MZNSendDebugMessage("Invoked CEventListener InvokeEx");

	execute (pDispParams);

	return S_OK;
}

HRESULT CEventListener::DeleteMemberByName(BSTR bstrName, DWORD grfdex) {
	return E_NOTIMPL;
}

HRESULT CEventListener::DeleteMemberByDispID(DISPID id) {
	return E_NOTIMPL;
}

HRESULT CEventListener::GetMemberProperties(DISPID id, DWORD grfdexFetch,
		DWORD* pgrfdex) {
	return E_NOTIMPL;
}

HRESULT CEventListener::GetMemberName(DISPID id, BSTR* pbstrName) {
	return E_NOTIMPL;
}

HRESULT CEventListener::GetNextDispID(DWORD grfdex, DISPID id, DISPID* pid) {
	return E_NOTIMPL;
}

HRESULT CEventListener::GetNameSpaceParent(IUnknown** ppunk) {
	return E_NOTIMPL;
}

void CEventListener::execute(DISPPARAMS* pDispParams) {
	if (m_pElement != NULL && m_listener != NULL)
	{
		AbstractWebElement *pElement = m_pElement;
		bool releaseElement = false;

//		MZNSendDebugMessage("isAddEventListener = %d  pDispParams->cArgs = %d pDispParams->cNamedArgs = %d",
//				(int) isAddEventListener, (int) pDispParams->cArgs,
//				pDispParams->cNamedArgs);
		if (isAddEventListener && pDispParams != NULL && pDispParams->cArgs == 2)
		{
			VARIANTARG &arg2 = pDispParams->rgvarg[1];
			if (arg2.vt == VT_DISPATCH)
			{
//				MZNSendDebugMessage("Got pDispatch");

				BSTR bstr = Utils::str2bstr("target");
				DISPID dispId = 0;
				HRESULT hr = arg2.pdispVal->GetIDsOfNames(IID_NULL, &bstr, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
				if ( !FAILED(hr))
				{
//					MZNSendDebugMessageA("Got id for target");
					DISPPARAMS dp;
					VARIANTARG va[0];
					dp.cArgs = 0;
					dp.cNamedArgs = 0;
					VARIANT result;
					VariantInit(&result);
					UINT error;
					EXCEPINFO ei;
					ZeroMemory (&ei, sizeof ei);
		//			MZNSendDebugMessageA("Invoking %s", "attachEvent");
					hr = arg2.pdispVal->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, INVOKE_PROPERTYGET, &dp, &result, &ei, &error);
					if (!FAILED(hr))
					{
//						MZNSendDebugMessage("Got target attribute %p", result.pdispVal);
						IDispatch* target = result.pdispVal;
						pElement = new ExplorerElement (target, dynamic_cast<ExplorerWebApplication*>(m_pElement->getApplication()));
						std::string tag;
						pElement->getTagName(tag);
//						MZNSendDebugMessage("Created new element %s", tag.c_str());
						releaseElement = true;
						target->Release();
					}
				} else {
//					MZNSendDebugMessage("Cannot get DOMEvent from arg2.pdisval");
				}
			}
		}
		pElement->sanityCheck();

//		MZNSendDebugMessage("Processing listener");
		m_listener->onEvent(m_event.c_str(), m_pElement->getApplication(), pElement);
		if (releaseElement)
			pElement->release();
//		MZNSendDebugMessage("Processed listener");
	}
	else if (m_pApplication != NULL && false)
	{
//		MZNSendDebugMessageA("m_pApplication != null");
		if (!preventLoop)
		{
			preventLoop = true;
			std::string url;
			m_pApplication->getUrl(url);
//			MZNSendDebugMessage("Invoked refresh event %p; %s", m_pApplication, url.c_str());
			MZNWebMatchRefresh(m_pApplication);
			preventLoop = false;
		}
	}
//	MZNSendDebugMessage("End listener::execute");
}

HRESULT CEventListener::GetDispID(BSTR bstrName, DWORD grfdex, DISPID* pid) {
	return E_NOTIMPL;
}

void CEventListener::connectRefresh (ExplorerWebApplication *app)
{
	m_pApplication = app;
	m_pApplication -> lock();
	BSTR bstr = Utils::str2bstr("soffidRefresh");
	VARIANT va;
	va.vt = VT_DISPATCH;
	va.pdispVal  = this;

//	MZNSendDebugMessageA("Installing listener");

	IHTMLWindow2 *w = NULL;
	IHTMLDocument2* pDoc = app->getHTMLDocument2();
	if (pDoc != NULL && S_OK == pDoc->get_parentWindow(&w) && w != NULL)
	{
		DISPID dispId;
		IDispatchEx *pDispatchEx = NULL;
		IID IDispatchEx_CLSID = { 0xa6ef9860, 0xc720, 0x11d0, {0x93, 0x37, 0x00,0xa0,0xc9,0x0d,0xca,0xa9}};

		if (S_OK == w->QueryInterface(IDispatchEx_CLSID, reinterpret_cast<void**>(&pDispatchEx)))
		{
			if (FAILED (pDispatchEx->GetDispID(bstr, fdexNameEnsure, &dispId)))
			{
				MZNSendDebugMessage("Unable to create property %ls", bstr);
			}
			else
			{
				DISPPARAMS dp;
				dp.cArgs = 1;
				dp.cNamedArgs = 0;
				dp.rgvarg = &va;
				VARIANT result ;
				result.vt = VT_NULL;
				UINT error;
				HRESULT hr = pDispatchEx->InvokeEx(dispId, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUTREF, &dp, &result, NULL, NULL);

				if ( !FAILED(hr))
				{
//					MZNSendDebugMessageA("Success !!");
					BSTR b = Utils::str2bstr("window.soffidRefresh();");
					BSTR b2 = Utils::str2bstr("javascript");
					VARIANT v;
					long timer;
					v.vt = VT_BSTR;
					v.bstrVal = b2;
					w->setInterval(b, 10000, &v, &timer);
					SysFreeString(b);
					SysFreeString(b2);
				}
//				else
//					MZNSendDebugMessage("Error en InvokeEx");

			}
			pDispatchEx -> Release();
		}
		else
		{
//			MZNSendDebugMessage("Cannot get IDispatchEx");
		}
		w->Release();
	}

	SysFreeString(bstr);

}


