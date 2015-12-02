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
#include "ExplorerWebApplication.h"
#include "Utils.h"
#include "CEventListener.h"
#include <map>
#include <MazingerInternal.h>
#include <string.h>
#include <stdio.h>

ExplorerElement::ExplorerElement(IDispatch *pdispElement, ExplorerWebApplication *app) {
	m_pElement = pdispElement;
	m_pElement->AddRef();
	m_pApp = app;
	app->lock();
	getProperty("__Soffid_Afr_id", m_internalId);
	if (m_internalId.empty())
	{
		char ach[20];
		sprintf (ach, "%ld", (long) m_pApp->getNextCounter());
		m_internalId = ach;
		setProperty("__Soffid_Afr_id", ach);
	}
}

ExplorerElement::~ExplorerElement() {
	m_pElement->Release();
	m_pElement = NULL;
	m_pApp->release();
	m_pApp = NULL;
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
				children.push_back(new ExplorerElement(el2, m_pApp));
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



static HRESULT getDispatchProperty (IDispatch *object, const char *property, VARIANT &value)
{
	BSTR bstr = Utils::str2bstr(property);
	DISPID dispId;
	IDispatchEx *pDispatchEx;
	static IID IDispatchEx_CLSID = { 0xa6ef9860, 0xc720, 0x11d0, {0x93, 0x37, 0x00,0xa0,0xc9,0x0d,0xca,0xa9}};
	HRESULT hr = object->QueryInterface(IDispatchEx_CLSID, reinterpret_cast<void**>(&pDispatchEx)) ;
	if (! FAILED (hr))
	{
		hr = pDispatchEx->GetDispID(bstr, fdexNameEnsure, &dispId);
		if (FAILED (hr))
		{
			MZNSendDebugMessage("Unable to get property %ls", bstr);
		}
		else
		{
			DISPPARAMS dp;
			dp.cArgs = 0;
			dp.cNamedArgs = 0;
			dp.rgvarg = NULL;
			VARIANT result ;
			result.vt = VT_NULL;
			UINT error;
			EXCEPINFO ei;
			ZeroMemory (&ei, sizeof ei);
			hr = pDispatchEx->InvokeEx(dispId, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, &value, &ei, NULL);
			if ( FAILED(hr))
			{
#if 0
				MZNSendDebugMessage("Cannot get property %s", property);
				MZNSendDebugMessage("Error en InvokeEx");
				MZNSendDebugMessage("Error en %lX en attributes %d", (long) hr,  (int) error);
				MZNSendDebugMessage("Error code %d ", (int) ei.wCode);
				MZNSendDebugMessage("Source %ls", ei.bstrSource);
				MZNSendDebugMessage("Description %ls", ei.bstrDescription);
#endif
			}

		}
		pDispatchEx -> Release();
	}
	else
	{
		MZNSendDebugMessage("Cannot get IDispatchEx");
	}
	SysFreeString(bstr);
	return hr;
}

static HRESULT setDispatchProperty (IDispatch *object, const char *property, VARIANT &value)
{
	BSTR bstr = Utils::str2bstr(property);
	DISPID dispId;
	IDispatchEx *pDispatchEx;
	static IID IDispatchEx_CLSID = { 0xa6ef9860, 0xc720, 0x11d0, {0x93, 0x37, 0x00,0xa0,0xc9,0x0d,0xca,0xa9}};
	HRESULT hr = object->QueryInterface(IDispatchEx_CLSID, reinterpret_cast<void**>(&pDispatchEx)) ;
	if (! FAILED (hr))
	{
		hr = pDispatchEx->GetDispID(bstr, fdexNameEnsure, &dispId);
		if (FAILED (hr))
		{
			MZNSendDebugMessage("Unable to create property %ls", bstr);
		}
		else
		{
			DISPPARAMS dp;
			dp.cArgs = 1;
			dp.cNamedArgs = 0;
			dp.rgvarg = &value;
			VARIANT result ;
			result.vt = VT_NULL;
			UINT error;
			hr = pDispatchEx->InvokeEx(dispId, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &dp, &result, NULL, NULL);

			if ( !FAILED(hr))
			{
			}
			else
				MZNSendDebugMessage("Error en InvokeEx");

		}
		pDispatchEx -> Release();
	}
	else
	{
		MZNSendDebugMessage("Cannot get IDispatchEx");
	}
	SysFreeString(bstr);
	return hr;
}

static std::string trim (std::string src)
{
	while (src.length() > 0 && src[0] == ' ')
		src = src.substr(1);

	while (src.length() > 0 && src[src.length()-1] == ' ')
		src = src.substr(0, src.length()-1);
	return src;
}

static void setStyleAttribute (IDispatch *object, const char *szStyle)
{
	VARIANT v;
	VariantInit(&v);
	HRESULT hr = getDispatchProperty(object, "style", v);
	if ( FAILED (hr))
		return;

	if (! v.vt == VT_DISPATCH)
	{
		VariantClear(&v);
		return;
	}

	VARIANT v2;
	v2.vt = VT_BSTR;
	v2.bstrVal = Utils::str2bstr(szStyle);
	setDispatchProperty(v.pdispVal, "cssText", v2);
	VariantClear (&v2);

	v.pdispVal->Release();

	VariantClear(&v);
}

static void getStyleAttribute (IDispatch *object, std::string &style)
{
	VARIANT v;
	VariantInit(&v);
	HRESULT hr = getDispatchProperty(object, "style", v);
	if ( FAILED (hr))
		return;

	if (! v.vt == VT_DISPATCH)
	{
		VariantClear(&v);
		return;
	}

	VARIANT v2;
	VariantInit (&v2);
	hr = getDispatchProperty(v.pdispVal, "cssText", v2);
	if ( !FAILED(hr))
	{
		VARIANT result2;
		VariantInit(&result2);
		if ( S_OK == VariantChangeType (&result2, &v2, 0, VT_BSTR))
		{
			Utils::bstr2str (style, result2.bstrVal);
			VariantClear (&result2);
		} else {
		}
		VariantClear (&v2);
	}


	v.pdispVal->Release();
	VariantClear(&v);
}


void ExplorerElement::getProperty(const char *property, std::string & value)
{
	VARIANT result;
	VariantInit(&result);
	HRESULT hr = getDispatchProperty( m_pElement, property, result);
	if ( !FAILED(hr))
	{
		VARIANT result2;
		VariantInit(&result2);
		if ( S_OK == VariantChangeType (&result2, &result, 0, VT_BSTR))
		{
			Utils::bstr2str (value, result2.bstrVal);
			VariantClear (&result2);
		} else {
		}
		VariantClear (&result);
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
			parent = new ExplorerElement(nativeParent, m_pApp);
			nativeParent->Release();
		}
		e->Release();
	}
	return parent;
}

AbstractWebElement *ExplorerElement::getOffsetParent()
{
	IHTMLElement *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	ExplorerElement *parent = NULL;
	if (!FAILED(hr))
	{
		IHTMLElement *nativeParent;
		hr = e->get_offsetParent(&nativeParent);
		if (nativeParent != NULL) {
			parent = new ExplorerElement(nativeParent, m_pApp);
			nativeParent->Release();
		}
		e->Release();
	}
	return parent;
}


void ExplorerElement::getAttribute(const char *attribute, std::string & value)
{
	if (strcmp (attribute,"style") == 0)
		getStyleAttribute (m_pElement, value);
	else
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
}


void ExplorerElement::setAttribute(const char *attribute, const char*value)
{
	if (strcmp (attribute,"style") == 0)
		setStyleAttribute (m_pElement, value);
	else
	{
		if (strcmp (attribute, "class") == 0)
			attribute = "className";
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
	return new ExplorerElement(m_pElement, m_pApp );
}


static std::map<WebListener *, CEventListener*> activeListeners;

void ExplorerElement::subscribe(const char* eventName, WebListener* listener) {
	CEventListener *el = new CEventListener();
	el->connect(this, eventName, listener);
	activeListeners[listener] = el;
}

void ExplorerElement::unSubscribe(const char* eventName, WebListener* listener) {
	if (activeListeners.find(listener) != activeListeners.end())
	{
//		MZNSendDebugMessageA("Unsubscribing %s on %s", eventName, toString().c_str());
		CEventListener *el = activeListeners[listener];
		activeListeners.erase(listener);
		el->Release();
	}
}


AbstractWebElement* ExplorerElement::getPreviousSibling() {
	AbstractWebElement *result = NULL;
	IHTMLDOMNode *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		IHTMLDOMNode *next;
		hr = e->get_previousSibling(&next);
		if (!FAILED(hr) && next != NULL) {
			result = new ExplorerElement(next, m_pApp);
		}
		e->Release();
	}
	return result;
}


AbstractWebElement* ExplorerElement::getNextSibling() {
	AbstractWebElement *result = NULL;
	IHTMLDOMNode *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		IHTMLDOMNode *next;
		hr = e->get_nextSibling(&next);
		if (!FAILED(hr) && next != NULL) {
			result = new ExplorerElement(next, m_pApp);
		}
		e->Release();
	}
	return result;
}

void ExplorerElement::appendChild(AbstractWebElement* element) {
	ExplorerElement* child = dynamic_cast<ExplorerElement*> (element);
	if (child == NULL)
		return;
	IHTMLDOMNode *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		IHTMLDOMNode *e2;
		IHTMLDOMNode *e3 = NULL;
		HRESULT hr = child->m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e2));
		if (!FAILED(hr))
		{
			hr = e->appendChild(e2, &e3);
			if (e3 != NULL) {
#if 0
				IDispatch *old = child->m_pElement;
				HRESULT hr = e3->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&child->m_pElement));
				if (!FAILED(hr))
				{
					old->Release();
				}
				else
				{
					child->m_pElement = old;
				}
#endif
				e3->Release();
			}
			e2->Release();
		}
		e->Release();
	}
}

void ExplorerElement::insertBefore(AbstractWebElement* element,
		AbstractWebElement* before) {
	ExplorerElement* child = dynamic_cast<ExplorerElement*> (element);
	ExplorerElement* sibling = dynamic_cast<ExplorerElement*> (before);
	if (child == NULL)
		return;
	IHTMLDOMNode *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		IHTMLDOMNode *e2;
		HRESULT hr = child->m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e2));
		if (!FAILED(hr))
		{
			VARIANT v;
			v.pdispVal = NULL;
			if (sibling == NULL)
			{
				v.vt = VT_NULL;
			}
			else
			{
				// hr = sibling->m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&v.pdispVal));
				v.vt = VT_DISPATCH;
				v.pdispVal = sibling->m_pElement;
			}
			if (!FAILED(hr))
			{
				IHTMLDOMNode *e4 = NULL;
				hr = e->insertBefore(e2, v, &e4);
				if (e4 != NULL) {
					IDispatch *old = child->m_pElement;
					HRESULT hr = e4->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&child->m_pElement));
					if (!FAILED(hr))
					{
						old->Release();
					}
					else
					{
						child->m_pElement = old;
					}
					e4->Release();
				}
			}
			e2->Release();
		}
		e->Release();
	}
}

void ExplorerElement::setTextContent(const char* text) {
	IHTMLElement *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		BSTR bstrValue = Utils::str2bstr(text);
		hr = e->put_innerText(bstrValue);
		SysFreeString(bstrValue);
		e->Release();
	}
}

AbstractWebApplication* ExplorerElement::getApplication() {
	m_pApp->lock();
	return m_pApp;
}

bool ExplorerElement::equals(AbstractWebElement* other) {
	ExplorerElement *otherElement = dynamic_cast<ExplorerElement*> (other);
	if (otherElement == NULL)
		return false;
	else
	{
		std::string otherString = otherElement->m_internalId;
		std::string thisString = m_internalId;
		return otherString == thisString;
	}
}

std::string ExplorerElement::toString() {

	char ach[200];
	sprintf (ach, " %p->%p ( %s )", this, m_pElement, m_internalId.c_str());
	std::string s = "ExplorerElement ";

	std::string tag;
	getTagName(tag);
	s += tag;
	s += ach;
	return s;
}

void ExplorerElement::removeAttribute(const char* attribute) {
	IHTMLElement *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLElement, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		BSTR bstrValue = Utils::str2bstr(attribute);
		VARIANT_BOOL v;
		hr = e->removeAttribute(bstrValue, 0, &v);
		SysFreeString(bstrValue);
		e->Release();
	}
}

void ExplorerElement::removeChild(AbstractWebElement* element) {
	ExplorerElement* child = dynamic_cast<ExplorerElement*> (element);
	if (child == NULL)
		return;
	IHTMLDOMNode *e;
	HRESULT hr = m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e));
	if (!FAILED(hr))
	{
		IHTMLDOMNode *e2;
		IHTMLDOMNode *e3 = NULL;
		HRESULT hr = child->m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&e2));
		if (!FAILED(hr))
		{
			hr = e->removeChild(e2, &e3);
			if (e3 != NULL) {
				e3->Release();
			}
			e2->Release();
		}
		e->Release();
	}
}

void ExplorerElement::setProperty(const char* property, const char* value) {
	BSTR bstr = Utils::str2bstr(property);
	VARIANT va;
	va.vt = VT_BSTR;
	va.bstrVal = Utils::str2bstr(value);
	setDispatchProperty(m_pElement, property, va);
	SysFreeString(bstr);
	if (strcmp (property, "value") == 0)
	{
		BSTR eventName = Utils::str2bstr("onchange");
		IHTMLElement3 *pElement3 = NULL;
		static IID local_IIDHtmlElement3 = {0x3050f673,0x98b5,0x11cf,{0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}};
		HRESULT hr = m_pElement->QueryInterface(local_IIDHtmlElement3, reinterpret_cast<void**>(&pElement3));
		if (!FAILED(hr))
		{
			VARIANT_BOOL cancelled = 0;
			IHTMLEventObj *pEventObj;
			hr = m_pApp->getHTMLDocument4()->createEventObject(NULL, &pEventObj);
			if (! FAILED(hr))
			{
				VARIANT_BOOL cancelled = 0;
				VARIANT v;
				v.vt = VT_DISPATCH;
				v.pdispVal = pEventObj;
				hr = pElement3->fireEvent(eventName, &v, &cancelled);
				if (!FAILED(hr))
					pEventObj->Release();
			}
			pElement3->Release();
		}
		SysFreeString(eventName);
	}
}


// MIDL_INTERFACE("305104b7-98b5-11cf-bb82-00aa00bdce0b")
static IID local_IID_IHTMLWindow7 = { 0x305104b7, 0x98b5, 0x11cf, {0xbb, 0x82, 0x00, 0xaa ,0x00, 0xbd, 0xce, 0x0b}};

std::string ExplorerElement::getComputedStyle(const char* style)
{
	std::string value;


	// pDoc = element.document
	IHTMLDocument2 *pDoc = m_pApp->getHTMLDocument2();
	if (pDoc != NULL)
	{
		IHTMLWindow2 *w = NULL;
		// w = pDoc.parentWindow
		HRESULT hr = pDoc->get_parentWindow(&w);
		if (! FAILED (hr))
		{
			IHTMLWindow7 *w7 = NULL;
			w->QueryInterface(local_IID_IHTMLWindow7, reinterpret_cast<void**>(&w7));
			if (FAILED(hr))
			{
//				MZNSendDebugMessageA("Cannot get w7");
			}
			else
			{
				IHTMLDOMNode *node;
				HRESULT hr = m_pElement->QueryInterface(IID_IHTMLDOMNode, reinterpret_cast<void**>(&node));
				if (FAILED(hr))
				{
//					MZNSendDebugMessageA("Cannot cast to htmldomnode");
				}
				else {
					IHTMLCSSStyleDeclaration *pComputedStyle;
					hr = w7->getComputedStyle(node, NULL, &pComputedStyle);
					if ( FAILED(hr))
					{
//						MZNSendDebugMessage("Cannot get computedStyle");
					} else {
						BSTR bstr = Utils::str2bstr(style);
						BSTR bstr2 = NULL;
						hr = pComputedStyle->getPropertyValue(bstr, &bstr2);
						if (FAILED(hr))
						{
//							MZNSendDebugMessage("Cannot get property on computedStyle %s", style);
						}
						else
						{
							Utils::bstr2str(value, bstr2);
							if (bstr2 != NULL)
								SysFreeString(bstr2);
						}
						SysFreeString(bstr);
						pComputedStyle -> Release();
					}
					node->Release();
				}
				w7->Release();
			}
			w->Release();
		}
	}

	return value;
}
