#include <windows.h>
#include <stdio.h>

#include "CExplorerObj.h"


CExplorerObj::CExplorerObj ()
{
}


HRESULT __stdcall CExplorerObj::SetSite(IUnknown *pUnknown)
{
	m_pSite = pUnknown;
	if (m_pSite == NULL)
	   return E_INVALIDARG;


	m_pSite->QueryInterface(IID_IWebBrowser2, reinterpret_cast<void**>(&m_pWebBrowser));
	handler.connect (m_pWebBrowser);

	return S_OK;
}

HRESULT __stdcall CExplorerObj::QueryInterface(REFIID riid, void **ppObj) {
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	else if (riid == IID_IObjectWithSite) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	} else if (riid == IID_IDispatch) {
	} else if (riid == DIID_DWebBrowserEvents2) {
	} else {
	}

	//
	//if control reaches here then , let the client know that
	//we do not satisfy the required interface
	//
	*ppObj = NULL;
	return E_NOINTERFACE;
}



HRESULT __stdcall CExplorerObj::GetSite(REFIID riid, void **pSite)
{
	*pSite = static_cast<void*>(m_pSite);
	return S_OK;
}


ULONG __stdcall CExplorerObj::AddRef()
{
	return InterlockedIncrement(&m_nRefCount) ;
}


ULONG __stdcall CExplorerObj::Release()
{
	long nRefCount=0;
	nRefCount=InterlockedDecrement(&m_nRefCount) ;
	if (nRefCount == 0) delete this;
	return nRefCount;
}


