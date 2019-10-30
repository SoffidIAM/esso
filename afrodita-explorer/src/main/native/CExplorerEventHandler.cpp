#include <windows.h>
#include <stdio.h>
#include <exdispid.h>

#include "CExplorerObj.h"
#include "ExplorerWebApplication.h"
#include <MazingerInternal.h>
#include "Utils.h"
#include <mshtml.h>

CExplorerEventHandler::CExplorerEventHandler ()
{
	m_app = NULL;
	m_nRefCount = 0;
}


void  CExplorerEventHandler::connect(IWebBrowser2 *pBrowser)
{
	if (m_app != NULL)
		m_app->release();
	m_app = NULL;
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
	if (m_app != NULL)
		m_app->release();
	m_app = new ExplorerWebApplication(pBrowser == NULL ? m_pBrowser: pBrowser, NULL, url);
	MZNWebMatch(m_app);
}


static DWORD WINAPI documentThreadProc(
  LPVOID arg
)
{
	IDispatch *pDispatch = (IDispatch*) arg;
	MZNSendDebugMessageA("pDispatch = %p", pDispatch);
	ExplorerWebApplication *app = new ExplorerWebApplication(NULL, pDispatch, NULL);
	std::string url;
	std::string url2;
	MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
	app->getUrl(url);
	MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
	do
	{
		MZNSendDebugMessage("URL = %s", url.c_str());
		MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
		MZNWebMatch(app);
		MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
		Sleep(3000);
		MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
		app->getUrl(url2);
		MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
	} while (url == url2);
	MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
	app->release();
	MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
	pDispatch->Release();
	MZNSendDebugMessage ("%s:%d", __FILE__, __LINE__);
	return 0;
}

HRESULT __stdcall CExplorerEventHandler::Invoke(DISPID dispIdMember,REFIID riid,LCID lcid,WORD wFlags,DISPPARAMS *pDispParams,VARIANT *pVarResult,EXCEPINFO *pExcepInfo,UINT *puArgErr) {
	if(!IsEqualIID(riid,IID_NULL)) return DISP_E_UNKNOWNINTERFACE; // riid should always be IID_NULL

	if(dispIdMember==DISPID_DOCUMENTCOMPLETE) { // Handle the BeforeNavigate2 event
//		MZNSendDebugMessageA("OnDocumentComplete");
		IDispatch *pDispatch = NULL;

		std::string url;

		if (pDispParams != NULL && pDispParams->cArgs > 1
				&& (pDispParams->rgvarg[1].vt & VT_TYPEMASK)== VT_DISPATCH)
		{
			IWebBrowser2 *pBrowser = NULL;
			IDispatch *doc = pDispParams->rgvarg[1].pdispVal;

		    // Is this the DocumentComplete event for the top frame window?
		    // Check COM identity: compare IUnknown interface pointers.
		    if (S_OK == doc->QueryInterface(IID_IWebBrowser2, (void**)&pBrowser))
		    {
    		    pBrowser->get_Document(&pDispatch);
    		    pBrowser->Release();
		    }
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
		if (pDispatch != NULL)
		{
			ExplorerWebApplication *app = new ExplorerWebApplication(NULL, pDispatch, NULL);
			bool found = MZNWebMatch(app, false);
			IHTMLWindow2 *w;
			if ( ! found )
			{
				MZNSendDebugMessageA("web match failed. Retry in 2 seconds");
				app->installIntervalListener();
			}
		}
	} else if (dispIdMember == DISPID_PROGRESSCHANGE){

//		MZNSendDebugMessageA("Progress Change %ld / %ld" , pDispParams->rgvarg[0].lVal,
//				pDispParams->rgvarg[1].lVal) ;
	} else if (dispIdMember == DISPID_NAVIGATECOMPLETE2){
	} else if (dispIdMember == DISPID_NAVIGATECOMPLETE){
	} else {
//		MZNSendDebugMessageA("Ignoreing dispatcher %d", (int) dispIdMember) ;
	}
	return S_OK;
}


