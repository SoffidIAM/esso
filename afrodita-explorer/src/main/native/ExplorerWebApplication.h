/*
 * ExplorerWebApplication.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef EXPLORERWEBAPPLICATION_H_
#define EXPLORERWEBAPPLICATION_H_

class IWebBrowser2;
class IDispatch;
class IHTMLDocument2;
class IHTMLDocument3;

#include <string>
#include <AbstractWebApplication.h>

class IHTMLElementCollection;

class ExplorerWebApplication: public AbstractWebApplication {
public:
	ExplorerWebApplication(IWebBrowser2* pBrowser, IDispatch *pDocument, const char *url);
	virtual ~ExplorerWebApplication();

	virtual void getUrl (std::string &value) ;
	virtual void getTitle (std::string &value) ;
	virtual void getContent (std::string &value) ;
	virtual void getDomain (std::string &value) ;
	virtual void getCookie (std::string &value) ;
	virtual AbstractWebElement* getDocumentElement ();
	virtual AbstractWebElement * getElementById (const char*id) ;
	virtual void getElementsByTagName (const char*tag, std::vector<AbstractWebElement*> &elements) ;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) ;
	virtual void getForms (std::vector<AbstractWebElement*> &elements);
	virtual void getLinks (std::vector<AbstractWebElement*> &elements);
	virtual void getImages (std::vector<AbstractWebElement*> &elements);
	virtual void write (const char*str);
	virtual void writeln (const char*str) ;
	virtual AbstractWebElement * createElement(const char *tagName);

private:
	IWebBrowser2* m_pBrowser;
	std::string m_url;
	IDispatch* m_pDispatch;
	IHTMLDocument2* m_pHtmlDoc2;
	IHTMLDocument3* m_pHtmlDoc3;
    void populateVector(IHTMLElementCollection *pCol, std::vector<AbstractWebElement*> & elements);
	IDispatch * getIDispatch ();
	IHTMLDocument2 * getHTMLDocument2 ();
	IHTMLDocument3 * getHTMLDocument3 ();
};

#endif /* EXPLORERWEBAPPLICATION_H_ */
