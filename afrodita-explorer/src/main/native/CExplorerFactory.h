/*
 * CSayakaFactory.h
 *
 *  Created on: 11/10/2010
 *      Author: u07286
 */

#ifndef CSAYAKAFACTORY_H_
#define CSAYAKAFACTORY_H_

#include <ocidl.h>

class CExplorerFactory: public IClassFactory {
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

	private:
	long m_nRefCount;
};

#endif /* CSAYAKAFACTORY_H_ */
