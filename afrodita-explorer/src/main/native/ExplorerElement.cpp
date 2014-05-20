/*
 * ExplorerElement.cpp
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */

#include <windows.h>
#include <exdisp.h>
#include <mshtml.h>
#include "ExplorerElement.h"
#include "Utils.h"

ExplorerElement::ExplorerElement(IDispatch *pdispElement) {
	m_pElement = pdispElement;
	m_pElement->AddRef();
}

ExplorerElement::~ExplorerElement() {
	m_pElement->Release();
	m_pElement = NULL;
}



void ExplorerElement::getChildren(std::vector<AbstractWebElement*> &children)
{
	children.clear();
	IHTMLElement *e;
	long size;
	IDispatch *childrenDisp;
	IHTMLElementCollection *col;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		hr = e->get_children(&childrenDisp);
	}
	if (!FAILED(hr))
		hr = childrenDisp-> QueryInterface(IID_IHTMLElementCollection, reinterpret_cast<void**>(&col));
	if (!FAILED(hr))
		hr=col->get_length(&size);
	if (!FAILED(hr))
	{
		IDispatch *el2;
		VARIANT v1, v2;
		v1.vt = VT_NULL;
		for (int i = 0 ; !FAILED(hr) && i < size; i++)
		{
			v2.vt = VT_I4;
			v2.lVal = i;
			hr = col->item(v2, v2, &el2);
			if (!FAILED(hr))
			{
				children.push_back(new ExplorerElement(el2));
				el2->Release();
			}
		}
	}
	col->Release();
}



void ExplorerElement::click()
{
	IHTMLElement *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		hr = e->click();
		e->Release();
	}
}



void ExplorerElement::getAttribute(const char *attribute, std::string & value)
{
	IHTMLElement *e;
	value.clear();
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		BSTR bstr = Utils::str2bstr(attribute);
		VARIANT v;
		hr = e->getAttribute(bstr, 2 /* as bstr*/, &v);
		if (!FAILED(hr) && v.vt == VT_BSTR)
		{
			Utils::bstr2str(value, v.bstrVal);
			SysFreeString(v.bstrVal);
		}
		SysFreeString(bstr);
		e->Release();
	}
}



void ExplorerElement::blur()
{
	IHTMLElement2 *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement2, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		hr = e->blur();
		e->Release();
	}
}



AbstractWebElement *ExplorerElement::getParent()
{
	IHTMLElement *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	ExplorerElement *parent = NULL;
	if (!FAILED(hr))
	{
		IHTMLElement *nativeParent;
		hr = e->get_parentElement(&nativeParent);
		if (nativeParent != NULL) {
			parent = new ExplorerElement(nativeParent);
			nativeParent->Release();
		}
		e->Release();
	}
	return parent;
}



void ExplorerElement::setAttribute(const char *attribute, const char*value)
{
	IHTMLElement *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		BSTR bstrAttribute = Utils::str2bstr(attribute);
		BSTR bstrValue = Utils::str2bstr(value);
		VARIANT v;
		v.vt = VT_BSTR;
		v.bstrVal = bstrValue;
		hr = e->setAttribute(bstrAttribute, v,  1 /* respecte case*/);
		SysFreeString(bstrValue);
		SysFreeString(bstrAttribute);
		e->Release();
	}
}



void ExplorerElement::focus()
{
	IHTMLElement2 *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement2, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		hr = e->focus();
		e->Release();
	}

}



void ExplorerElement::getTagName(std::string & value)
{
	IHTMLElement *e;
	value.clear ();
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		BSTR bstrTag;
		hr = e->get_tagName(&bstrTag);
		if (!FAILED(hr) && bstrTag != NULL) {
			Utils::bstr2str(value, bstrTag);
			SysFreeString(bstrTag);
		}
		e->Release();
	}
}



AbstractWebElement *ExplorerElement::clone()
{
	return new ExplorerElement(m_pElement);
}


