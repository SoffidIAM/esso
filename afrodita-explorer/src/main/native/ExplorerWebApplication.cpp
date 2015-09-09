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


ExplorerWebApplication::~ExplorerWebApplication() {
	if (m_pBrowser != NULL)
		m_pBrowser->Release();
	if (m_pDispatch != NULL)
		m_pDispatch->Release();
	if (m_pHtmlDoc2 != NULL)
		m_pHtmlDoc2->Release();
	if (m_pHtmlDoc3 != NULL)
		m_pHtmlDoc3->Release();
}


void ExplorerWebApplication::getUrl(std::string & value)
{
	if (m_url.empty())
	{
		value.clear ();
		if (m_pBrowser != NULL)
		{
			BSTR url;

	//		HRESULT hr = m_pBrowser->get_LocationURL(&url);
			HRESULT hr = getHTMLDocument2()->get_URL(&url);
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
	if (m_pBrowser != NULL)
	{
		BSTR url;

		HRESULT hr = m_pBrowser->get_LocationName(&url);
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
	return m_pDispatch;
}


IHTMLDocument2 * ExplorerWebApplication::getHTMLDocument2 ()
{
	IDispatch *pDispatch = getIDispatch();
	if (m_pHtmlDoc2 == NULL && pDispatch != NULL)
	{
		pDispatch->QueryInterface(IID_IHTMLDocument2,reinterpret_cast<void**>(&m_pHtmlDoc2));
	}
	return m_pHtmlDoc2;
}

IHTMLDocument3 * ExplorerWebApplication::getHTMLDocument3 ()
{
	IDispatch *pDispatch = getIDispatch();
	if (m_pHtmlDoc3 == NULL && pDispatch != NULL)
	{
		pDispatch->QueryInterface(IID_IHTMLDocument3,reinterpret_cast<void**>(&m_pHtmlDoc3));
	}
	return m_pHtmlDoc3;
}

ExplorerWebApplication::ExplorerWebApplication(IWebBrowser2 *pBrowser, IDispatch *pDispatch, const char *url)
{
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
	return NULL;
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
				elements.push_back(new ExplorerElement(pElement));
			}
		}
}

void ExplorerWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{

	IHTMLDocument3 *pDoc = getHTMLDocument3();
	elements.clear();

	MZNSendDebugMessage ("Ask to look for tag %s", tag);
	if (pDoc != NULL)
	{
		MZNSendDebugMessage ("Looking for tag %s", tag);
		BSTR bstr = Utils::str2bstr(tag);
		IHTMLElementCollection *pCol;
		HRESULT hr = pDoc-> getElementsByTagName(bstr, &pCol);
		if (FAILED(hr)) return;
	    populateVector(pCol, elements);
	    pCol->Release();
	    for (std::vector<AbstractWebElement*>::iterator it = elements.begin();
	    		it != elements.end(); it++)
	    {
	    	AbstractWebElement *e = *it;
	    	std::string s;
	    	e->getTagName(s);
	    	MZNSendDebugMessage ("FOUND element with tag %s",
	    			s.c_str());
	    }
	}
}



void ExplorerWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	HRESULT hr = getHTMLDocument2()->get_images(&pCol);
	if (! FAILED (hr))
	{
	    populateVector(pCol, elements);
	    pCol->Release();
	}
}



void ExplorerWebApplication::writeln(const char *str)
{
	SAFEARRAY *psa;
	BSTR bstrVal = Utils::str2bstr(str);
	long index = 0;
	psa = SafeArrayCreateVector(VT_BSTR, 0, 1);
	SafeArrayPutElement(psa, &index,  bstrVal);
	getHTMLDocument2()->write(psa);
	SysFreeString(bstrVal);
	SafeArrayDestroy(psa);
}



void ExplorerWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	HRESULT hr = getHTMLDocument2()->get_links(&pCol);
	if (! FAILED (hr))
	{
	    populateVector(pCol, elements);
	    pCol->Release();
	}
}



void ExplorerWebApplication::getDomain(std::string & value)
{
	value.clear ();
	if (m_pBrowser != NULL)
	{
		BSTR url;

		HRESULT hr = getHTMLDocument2()->get_domain(&url);
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
	HRESULT hr = getHTMLDocument2()->get_anchors(&pCol);
	if (! FAILED (hr))
	{
	    populateVector(pCol, elements);
	    pCol->Release();
	}
}



void ExplorerWebApplication::getCookie(std::string & value)
{
	value.clear ();
	if (m_pBrowser != NULL)
	{
		BSTR url;

		HRESULT hr = getHTMLDocument2()->get_cookie(&url);
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
		IHTMLElement *pElement;
		HRESULT hr = pDoc-> getElementById(bstr, &pElement);
		SysFreeString (bstr);

		if (FAILED(hr)) return NULL;

		IDispatch *pDispatch;
		hr = pElement->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&pDispatch));
		pElement->Release();

		if (FAILED(hr)) return NULL;

		return new ExplorerElement (pDispatch);
	}
	else
		return NULL;
}



void ExplorerWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	elements.clear ();
	IHTMLElementCollection *pCol;
	HRESULT hr = getHTMLDocument2()->get_forms(&pCol);
	if (! FAILED (hr))
	{
	    populateVector(pCol, elements);
	    pCol->Release();
	}
}



void ExplorerWebApplication::write(const char *str)
{
	SAFEARRAY *psa;
	BSTR bstrVal = Utils::str2bstr(str);
	long index = 0;
	psa = SafeArrayCreateVector(VT_BSTR, 0, 1);
	SafeArrayPutElement(psa, &index,  bstrVal);
	getHTMLDocument2()->write(psa);
	SysFreeString(bstrVal);
	SafeArrayDestroy(psa);
}


AbstractWebElement* ExplorerWebApplication::createElement(const char* tagName) {
	return NULL;
}




