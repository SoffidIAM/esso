#include <windows.h>
#include <stdio.h>
#include <exdispid.h>

#include "CExplorerObj.h"
#include "ExplorerWebApplication.h"
#include <MazingerInternal.h>
#include "Utils.h"


CExplorerEventHandler::CExplorerEventHandler ()
{
}


void  CExplorerEventHandler::connect(IWebBrowser2 *pBrowser)
{
	m_pBrowser = pBrowser;
	m_pBrowser->AddRef();
	IConnectionPointContainer* pCPC = NULL;
	if (S_OK == m_pBrowser->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC)))
	{
		  IConnectionPoint* pCP;

		  // Receives the connection point for WebBrowser events
		  HRESULT hr = pCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, &pCP);
		  if (FAILED(hr)) {
			   return ;
		  }

		  // Pass our event handlers to the container. Each time an event occurs
		  // the container will invoke the functions of the IDispatch interface
		  // we implemented.
		  AddRef();
		  m_dwCookie = 0;
		  hr = pCP->Advise( reinterpret_cast<IDispatch*>(this), &m_dwCookie);
		  pCP -> Release ();
		  pCPC -> Release ();
	}


}





HRESULT __stdcall CExplorerEventHandler::QueryInterface(REFIID riid, void **ppObj) {
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	} else if (riid == IID_IDispatch) {
		IDispatch *pDispatch = reinterpret_cast<IDispatch*> (this);
		*ppObj = static_cast<void*> (pDispatch);
		AddRef();
		return S_OK;
	} else if (riid == DIID_DWebBrowserEvents2) {
		DWebBrowserEvents2 *pDispatch = reinterpret_cast<DWebBrowserEvents2*> (this);
		*ppObj = static_cast<void*> (pDispatch);
		AddRef();
		return S_OK;
	} else {
	}

	//
	//if control reaches here then , let the client know that
	//we do not satisfy the required interface
	//
	*ppObj = NULL;
	return E_NOINTERFACE;
}



ULONG __stdcall CExplorerEventHandler::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall CExplorerEventHandler::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) delete this;
	return nRefCount;
}


// Implementacion de IDispatch
HRESULT __stdcall CExplorerEventHandler::GetTypeInfoCount (UINT*n) {
	return E_NOTIMPL ;
}

HRESULT __stdcall CExplorerEventHandler::GetTypeInfo(UINT num, LCID id,LPTYPEINFO* info)
{
	return E_NOTIMPL;
}
HRESULT __stdcall CExplorerEventHandler::GetIDsOfNames(REFIID ID,LPOLESTR* bstr,UINT num ,LCID id ,DISPID* pDispId) {
	return E_NOTIMPL;
}

void CExplorerEventHandler::onLoad(IWebBrowser2 *pBrowser, const char *url)
{
	ExplorerWebApplication app (pBrowser == NULL ? m_pBrowser: pBrowser, NULL, url);
	MZNWebMatch(&app);
}

HRESULT __stdcall CExplorerEventHandler::Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr) {
	if(!IsEqualIID(riid,IID_NULL)) return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL



	if(dispIdMember==DISPID_DOCUMENTCOMPLETE) { // Handle the BeforeNavigate2 event
		IWebBrowser2 *pBrowser = NULL;
		std::string url;

		if (pDispParams != NULL && pDispParams->cArgs > 1
				&& (pDispParams->rgvarg[1].vt & VT_TYPEMASK)== VT_DISPATCH)
		{
			IDispatch *doc = pDispParams->rgvarg[1].pdispVal;

		    // Is this the DocumentComplete event for the top frame window?
		    // Check COM identity: compare IUnknown interface pointers.
		    if (S_OK != doc->QueryInterface(IID_IWebBrowser2, (void**)&pBrowser))
		    	pBrowser = NULL;
		}
		if (pDispParams != NULL && pDispParams->cArgs > 0)
		{
			VARIANTARG dest;
			VariantInit (&dest);
			if ( S_OK == VariantChangeType (&dest, &pDispParams->rgvarg[0], 0, VT_BSTR))
			{
				//sprintf (ach, "args[1] = %d", pDispParams->rgvarg[1].vt & VT_TYPEMASK);
				//MessageBox(NULL, ach, "AFRODITA E3", MB_OK);
				Utils::bstr2str (url, dest.bstrVal);
				//MessageBox(NULL, v.c_str(), "AFRODITA E", MB_OK);
				VariantClear (&dest);
			}
		}
		onLoad(pBrowser, url.c_str());
	}
	return S_OK;
}


