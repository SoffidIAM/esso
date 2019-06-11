/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include <windows.h>
#include <exdisp.h>
#include <mshtml.h>
#include "ExplorerWebApplication.h"
#include "ExplorerElement.h"
#include "Utils.h"
#include <stdio.h>
#include <MazingerInternal.h>
#include "CEventListener.h"

ExplorerWebApplication::~ExplorerWebApplication() {
//	MZNSendDebugMessage("Removing application %p", this);
	if (m_pBrowser != NULL)
		m_pBrowser->Release();
	if (m_pDispatch != NULL)
		m_pDispatch->Release();
	if (m_pHtmlDoc2 != NULL)
		m_pHtmlDoc2->Release();
	if (m_pHtmlDoc3 != NULL)
		m_pHtmlDoc3->Release();
	if (m_pHtmlDoc4 != NULL)
		m_pHtmlDoc4->Release();
	if (m_pDocumentEvent != NULL)
		m_pDocumentEvent->Release();
	if (smartWebPage != NULL)
	{
		MZNSendDebugMessage("** Releasing smart web page");
		smartWebPage->release();
		MZNSendDebugMessage("** Released smart web page");
	}
	if (pIntervalListener != NULL)
		pIntervalListener->Release();
}

ExplorerWebApplication::ExplorerWebApplication(IWebBrowser2 *pBrowser, IDispatch *pDispatch, const char *url)
{
//	MZNSendDebugMessage("Creating application %p", this);
	m_counter = 1;
	if (url != NULL)
		m_url = url;
	m_pBrowser = pBrowser;
	if (m_pBrowser != NULL)
		m_pBrowser->AddRef ();
	if (pDispatch != NULL)
	{
		m_pDispatch = pDispatch;
		m_pDispatch->AddRef();
	}
	else
		m_pDispatch = NULL;
	m_pHtmlDoc2 = NULL;
	m_pHtmlDoc3 = NULL;
	m_pHtmlDoc4 = NULL;
	m_pDocumentEvent = NULL;
	pIntervalListener = NULL;
	smartWebPage = new SmartWebPage();
}


void ExplorerWebApplication::getUrl(std::string & value)
{
	if (m_url.empty())
	{
		value.clear ();
		BSTR url;

		IHTMLDocument2 *pDoc = getHTMLDocument2();
		if (pDoc != NULL)
		{
			HRESULT hr = pDoc->get_URL(&url);
			if (! FAILED(hr) )
			{
				Utils::bstr2str (value, url);
				SysFreeString(url);
			}
		}
	} else {
		value = m_url;
	}
}



void ExplorerWebApplication::getTitle(std::string & value)
{
	value.clear ();
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		BSTR url;

		HRESULT hr = pDoc->get_title(&url);
		if (! FAILED(hr) )
		{
			Utils::bstr2str (value, url);
			SysFreeString(url);
		}
	}
}



void ExplorerWebApplication::getContent(std::string & value)
{
	value.clear ();
	IHTMLDocument3 *pDoc = getHTMLDocument3();
	if (pDoc != NULL)
	{
		BSTR bstr;
		IHTMLElement *docElement;
		HRESULT hr = pDoc->get_documentElement(&docElement);
		if (! FAILED(hr))
		{
			hr = docElement->get_outerHTML(&bstr);
			if (!FAILED(hr)) {
				Utils::bstr2str (value, bstr);
				SysFreeString(bstr);
			}
			docElement->Release();
		}
	}
}

IDispatch * ExplorerWebApplication::getIDispatch ()
{
	if (m_pDispatch == NULL)
	{
		m_pBrowser->get_Document(&m_pDispatch);
	}
	if (m_pDispatch == NULL)
		MZNSendDebugMessageA("Unable to get ExplorerWebApplications's IDispatch");
	return m_pDispatch;
}


IHTMLDocument2 * ExplorerWebApplication::getHTMLDocument2 ()
{
	IDispatch *pDispatch = getIDispatch();
	if (m_pHtmlDoc2 == NULL && pDispatch != NULL)
	{
		pDispatch->QueryInterface(IID_IHTMLDocument2,reinterpret_cast<void**>(&m_pHtmlDoc2));
	}
	if (m_pHtmlDoc2 == NULL)
		MZNSendDebugMessageA("Unable to get ExplorerWebApplications's IHTMLDocument2");
	return m_pHtmlDoc2;
}

IHTMLDocument3 * ExplorerWebApplication::getHTMLDocument3 ()
{
	IDispatch *pDispatch = getIDispatch();
	if (m_pHtmlDoc3 == NULL && pDispatch != NULL)
	{
		pDispatch->QueryInterface(IID_IHTMLDocument3,reinterpret_cast<void**>(&m_pHtmlDoc3));
	}
	if (m_pHtmlDoc3 == NULL)
		MZNSendDebugMessageA("Unable to get ExplorerWebApplications's IHTMLDocument3");
	return m_pHtmlDoc3;
}

IHTMLDocument4 * ExplorerWebApplication::getHTMLDocument4 ()
{
	static IID local_IID_IHTMLDocument4 = {0x3050f69a, 0x98b5, 0x11cf, {0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}};

	IDispatch *pDispatch = getIDispatch();
	if (m_pHtmlDoc4 == NULL && pDispatch != NULL)
	{
		pDispatch->QueryInterface(local_IID_IHTMLDocument4,reinterpret_cast<void**>(&m_pHtmlDoc4));
	}
	if (m_pHtmlDoc4 == NULL)
	{
		MZNSendDebugMessageA("Unable to get ExplorerWebApplications's IHTMLDocument4");
	}
	return m_pHtmlDoc4;
}

IDocumentEvent * ExplorerWebApplication::getDocumentEvent ()
{
	static IID local_IID_IDocumentEvent = {0x305104bc, 0x98b5, 0x11cf, {0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}};

	IDispatch *pDispatch = getIDispatch();
	if (m_pDocumentEvent == NULL && pDispatch != NULL)
	{
		pDispatch->QueryInterface(local_IID_IDocumentEvent,reinterpret_cast<void**>(&m_pDocumentEvent));
	}
	if (m_pDocumentEvent == NULL)
	{
		MZNSendDebugMessageA("Unable to get ExplorerWebApplications's IHTMLDocument4");
	}
	return m_pDocumentEvent;
}

AbstractWebElement *ExplorerWebApplication::getDocumentElement()
{
	IHTMLDocument3 *pDoc = getHTMLDocument3();
	if (pDoc == NULL)
		return NULL;
	IHTMLElement *p;
	HRESULT hr = pDoc->get_documentElement(&p);
	if (FAILED(hr))
	{
		return NULL;
	}
	return new ExplorerElement(p, this);
}



void ExplorerWebApplication::populateVector(IHTMLElementCollection *pCol, std::vector<AbstractWebElement*> & elements)
{
	HRESULT hr;

    long length = 0;
    hr = pCol->get_length(&length);
    for (int i = 0; i < length; i++)
		{
			VARIANT v;
			VariantInit(&v);
			v.vt=VT_I4;
			v.uintVal = i;
			IDispatch *pElement;
			hr = pCol->item (v, v, &pElement);
			if (!FAILED(hr))
			{
				elements.push_back(new ExplorerElement(pElement, this));
			}
		}
}

void ExplorerWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{

	IHTMLDocument3 *pDoc = getHTMLDocument3();
	elements.clear();

//	MZNSendDebugMessage ("Ask to look for tag %s", tag);
	if (pDoc != NULL)
	{
//		MZNSendDebugMessage ("Looking for tag %s", tag);
		BSTR bstr = Utils::str2bstr(tag);
		IHTMLElementCollection *pCol;
		HRESULT hr = pDoc-> getElementsByTagName(bstr, &pCol);
		if (FAILED(hr)) return;
	    populateVector(pCol, elements);
	    pCol->Release();
	}
}



void ExplorerWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_images(&pCol);
		if (! FAILED (hr))
		{
		    populateVector(pCol, elements);
		    pCol->Release();
		}
	}
}



void ExplorerWebApplication::writeln(const char *str)
{
	SAFEARRAY *psa;
	BSTR bstrVal = Utils::str2bstr(str);
	long index = 0;
	psa = SafeArrayCreateVector(VT_BSTR, 0, 1);
	SafeArrayPutElement(psa, &index,  bstrVal);
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		pDoc->write(psa);
	}
	SysFreeString(bstrVal);
	SafeArrayDestroy(psa);
}



void ExplorerWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_links(&pCol);
		if (! FAILED (hr))
		{
			populateVector(pCol, elements);
			pCol->Release();
		}
	}
}



void ExplorerWebApplication::getDomain(std::string & value)
{
	value.clear ();
	BSTR url;

	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_domain(&url);
		if (! FAILED(hr) )
		{
			Utils::bstr2str (value, url);
			SysFreeString(url);
		}
	}
}



void ExplorerWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_anchors(&pCol);
		if (! FAILED (hr))
		{
			populateVector(pCol, elements);
			pCol->Release();
		}
	}
}



void ExplorerWebApplication::getCookie(std::string & value)
{
	value.clear ();
	BSTR url;

	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_cookie(&url);
		if (! FAILED(hr) )
		{
			Utils::bstr2str (value, url);
			SysFreeString(url);
		}
	}
}



AbstractWebElement *ExplorerWebApplication::getElementById(const char *id)
{
	IHTMLDocument3 *pDoc = getHTMLDocument3();

	if (pDoc != NULL)
	{
		BSTR bstr = Utils::str2bstr(id);
		IHTMLElement *pElement = NULL;
		HRESULT hr = pDoc-> getElementById(bstr, &pElement);
		SysFreeString (bstr);

		if (FAILED(hr) || pElement == NULL)
			return NULL;

		IDispatch *pDispatch;
		hr = pElement->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&pDispatch));
		pElement->Release();

		if (FAILED(hr))
		{
			return NULL;
		}

		return new ExplorerElement (pDispatch, this);
	}
	else
	{
		return NULL;
	}
}



void ExplorerWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_forms(&pCol);
		if (! FAILED (hr))
		{
			populateVector(pCol, elements);
			pCol->Release();
		}
	}
}



void ExplorerWebApplication::write(const char *str)
{
	SAFEARRAY *psa;
	BSTR bstrVal = Utils::str2bstr(str);
	long index = 0;
	psa = SafeArrayCreateVector(VT_BSTR, 0, 1);
	SafeArrayPutElement(psa, &index,  bstrVal);
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		pDoc->write(psa);
	}
	SysFreeString(bstrVal);
	SafeArrayDestroy(psa);
}


AbstractWebElement* ExplorerWebApplication::createElement(const char* tagName) {
	BSTR bstr = Utils::str2bstr(tagName);
	IHTMLElement *element;
	AbstractWebElement *result = NULL;
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->createElement(bstr, &element);
		if ( !FAILED (hr))
		{
			result =  new ExplorerElement(element, this);
//			MZNSendDebugMessage("Crated element %s", result->toString().c_str());
		}
		else
			result = NULL;
		SysFreeString(bstr);
	}
	return result;
}

void ExplorerWebApplication::alert(const char* str) {
	IHTMLWindow2 *w;
	BSTR bstr = Utils::str2bstr(str);
	IHTMLDocument2 *pDoc = getHTMLDocument2();
	if (pDoc != NULL)
	{
		HRESULT hr = pDoc->get_parentWindow(&w);
		if (! FAILED (hr))
		{
			w->alert(bstr);
		}
	}
	SysFreeString(bstr);
}

void ExplorerWebApplication::subscribe(const char* eventName,
		WebListener* listener) {
}

void ExplorerWebApplication::unSubscribe(const char* eventName,
		WebListener* listener) {
}

SmartWebPage* ExplorerWebApplication::getWebPage() {
	return smartWebPage;
}

std::string ExplorerWebApplication::toString() {
	char ach[50];
	sprintf (ach, "ExplorerWebApplication %p", this);
	return std::string (ach);
}

void ExplorerWebApplication::installIntervalListener() {
//	if (pIntervalListener == NULL)
//	{
//		pIntervalListener = new CEventListener();
//		pIntervalListener->connectRefresh(this);
//	}
}

bool ExplorerWebApplication::supportsPageData() {
	return false;
}

PageData* ExplorerWebApplication::getPageData() {
	return NULL;
}

AbstractWebElement* ExplorerWebApplication::getElementBySoffidId(
		const char* id) {
	return NULL;
}
