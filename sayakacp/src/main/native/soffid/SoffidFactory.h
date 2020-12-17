/*
 * CSayakaFactory.h
 *
 *  Created on: 11/10/2010
 *      Author: u07286
 */

#ifndef SOFFIDFACTORY_H_
#define SOFFIDFACTORY_H_

#include <ocidl.h>
#include "Log.h"

class SoffidFactory: public IClassFactory {
public:
	//interface IUnknown methods
	HRESULT __stdcall QueryInterface(
								REFIID riid ,
								void **ppObj);
	ULONG   __stdcall AddRef();
	ULONG   __stdcall Release();


	//interface IClassFactory methods
	HRESULT __stdcall CreateInstance(IUnknown* pUnknownOuter,
											 const IID& iid,
											 void** ppv) ;
	HRESULT __stdcall LockServer(BOOL bLock) ;


	SoffidFactory ();
private:
	long m_nRefCount;
	Log m_log;
};

#endif /* CSAYAKAFACTORY_H_ */
