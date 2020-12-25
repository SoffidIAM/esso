/*
 * CSoffidFactory.cpp
 *
 *  Created on: 11/10/2010
 *      Author: u07286
 */

#include "sayaka.h"
#include "SoffidFactory.h"
#include "SoffidProvider.h"
#include "credentialprovider.h"

SoffidFactory::SoffidFactory() :
	m_log("SoffidFactory") {
	m_log.info("Creating SoffidFactory 2");
}

HRESULT __stdcall SoffidFactory::CreateInstance(IUnknown* pUnknownOuter,
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
	m_log.info("Creating SoffidProvider");
	SoffidProvider* pObject = new SoffidProvider;
	if (pObject == NULL) {
		return E_OUTOFMEMORY;
	}

	//
	// Get the requested interface.
	//
	return pObject->QueryInterface(iid, ppv);
}


HRESULT __stdcall SoffidFactory::LockServer(BOOL bLock) {
	return E_NOTIMPL;
}

HRESULT __stdcall SoffidFactory::QueryInterface(REFIID riid, void **ppObj) {
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

ULONG __stdcall SoffidFactory::AddRef() {
	return InterlockedIncrement(&m_nRefCount);
}

ULONG __stdcall SoffidFactory::Release() {
	long nRefCount = 0;
	nRefCount = InterlockedDecrement(&m_nRefCount);
	if (nRefCount == 0)
		delete this;
	return nRefCount;
}
