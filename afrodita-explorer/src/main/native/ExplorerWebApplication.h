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
class CEventListener;

#include <string>
#include <AbstractWebApplication.h>
#include <SmartWebPage.h>

class IHTMLElementCollection;
class IHTMLDocument4;
class IDocumentEvent;

class ExplorerWebApplication: public AbstractWebApplication {
public:
	ExplorerWebApplication(IWebBrowser2* pBrowser, IDispatch *pDocument, const char *url);
	virtual ~ExplorerWebApplication();

	virtual bool supportsPageData () ;
	virtual PageData* getPageData() ;
	virtual void getUrl (std::string &value) ;
	virtual void getTitle (std::string &value) ;
	virtual void getContent (std::string &value) ;
	virtual void getDomain (std::string &value) ;
	virtual void getCookie (std::string &value) ;
	virtual AbstractWebElement* getDocumentElement ();
	virtual AbstractWebElement * getElementById (const char*id) ;
	virtual AbstractWebElement * getElementBySoffidId (const char*id) ;
	virtual void getElementsByTagName (const char*tag, std::vector<AbstractWebElement*> &elements) ;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) ;
	virtual void getForms (std::vector<AbstractWebElement*> &elements);
	virtual void getLinks (std::vector<AbstractWebElement*> &elements);
	virtual void getImages (std::vector<AbstractWebElement*> &elements);
	virtual void write (const char*str);
	virtual void writeln (const char*str) ;
	virtual AbstractWebElement * createElement(const char *tagName);
	virtual void alert (const char *str) ;
	virtual void subscribe ( const char *eventName, WebListener *listener) ;
	virtual void unSubscribe ( const char *eventName, WebListener *listener);
	virtual std::string toString() ;
	virtual void execute (const char* script);

	virtual SmartWebPage* getWebPage ();


	SmartWebPage* smartWebPage;

	IWebBrowser2 *getBrowser() { return m_pBrowser; }
	long getNextCounter() { return m_counter ++;}
	IHTMLDocument2 * getHTMLDocument2 ();
	IHTMLDocument4 * getHTMLDocument4 ();
	IDocumentEvent * getDocumentEvent ();
	void installIntervalListener ();
	void installLoadListener ();
private:
	CEventListener *pLoadListener;
	CEventListener *pIntervalListener;
	IWebBrowser2* m_pBrowser;
	std::string m_url;
	IDispatch* m_pDispatch;
	IHTMLDocument2* m_pHtmlDoc2;
	IHTMLDocument3* m_pHtmlDoc3;
	IHTMLDocument4* m_pHtmlDoc4;
	IDocumentEvent* m_pDocumentEvent;
    void populateVector(IHTMLElementCollection *pCol, std::vector<AbstractWebElement*> & elements);
	IDispatch * getIDispatch ();
	IHTMLDocument3 * getHTMLDocument3 ();
	long m_counter;
};

#endif /* EXPLORERWEBAPPLICATION_H_ */
