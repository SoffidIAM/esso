/*
 * CSayakaFactory.cpp
 *
 *  Created on: 11/10/2010
 *      Author: u07286
 */

#include "sayaka.h"
#include "SayakaFactory.h"
#include "SayakaProvider.h"
#include "credentialprovider.h"

SayakaFactory::SayakaFactory() :
	m_log("SayakaFactory") {
	m_nRefCount = 0;
	m_log.info("Creating SayakaFactory");
}

HRESULT __stdcall SayakaFactory::CreateInstance(IUnknown* pUnknownOuter,
		const IID& iid, void** ppv) {
	//
	//This method lets the client manufacture components en masse
	//The class factory provides a mechanism to control the way
	//the component is created. Within the class factory the
	//author of the component may decide to selectivey enable
	//or disable creation as per license agreements
	//
	//

	// Cannot aggregate.
	if (pUnknownOuter != NULL) {
		return CLASS_E_NOAGGREGATION;
	}

	//
	// Create an instance of the component.
	//
	m_log.info("Creating SayakaProvider");
	SayakaProvider* pObject = new SayakaProvider;
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	//
	// Get the requested interface.
	//
	return pObject->QueryInterface(iid, ppv);
}


HRESULT __stdcall SayakaFactory::LockServer(BOOL bLock) {
	return E_NOTIMPL;
}

HRESULT __stdcall SayakaFactory::QueryInterface(REFIID riid, void **ppObj) {
	if (riid == IID_IUnknown) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	if (riid == IID_IClassFactory) {
		*ppObj = static_cast<void*> (this);
		AddRef();
		return S_OK;
	}

	//
	//if control reaches here then , let the client know that
	//we do not satisfy the required interface
	//

	*ppObj = NULL;
	return E_NOINTERFACE;
}

ULONG __stdcall SayakaFactory::AddRef() {
	return InterlockedIncrement(&m_nRefCount);
}

ULONG __stdcall SayakaFactory::Release() {
	long nRefCount = 0;
	nRefCount = InterlockedDecrement(&m_nRefCount);
	if (nRefCount == 0)
		delete this;
	return nRefCount;
}
