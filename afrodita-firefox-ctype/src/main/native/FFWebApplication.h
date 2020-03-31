/*
 * ExplorerWebApplication.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef EXPLORERWEBAPPLICATION_H_
#define EXPLORERWEBAPPLICATION_H_


#include <AbstractWebApplication.h>

class FFWebApplication: public AbstractWebApplication {
public:
	FFWebApplication(long docId);
protected:
	virtual ~FFWebApplication();
public:
	virtual bool supportsPageData ();
	virtual PageData* getPageData();
	virtual AbstractWebElement * getElementBySoffidId (const char*id) ;
	virtual void getUrl (std::string &value) ;
	virtual void getTitle (std::string &value) ;
	virtual void getContent (std::string &value) ;
	virtual void getDomain (std::string &value) ;
	virtual void getCookie (std::string &value) ;
	virtual AbstractWebElement * createElement(const char *tagName) ;
	virtual AbstractWebElement* getDocumentElement ();
	virtual AbstractWebElement * getElementById (const char*id) ;
	virtual void getElementsByTagName (const char*tag, std::vector<AbstractWebElement*> &elements) ;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) ;
	virtual void getForms (std::vector<AbstractWebElement*> &elements);
	virtual void getLinks (std::vector<AbstractWebElement*> &elements);
	virtual void getImages (std::vector<AbstractWebElement*> &elements);
	virtual void write (const char*str);
	virtual void writeln (const char*str) ;
	virtual void alert (const char *str) ;
	long getDocId() { return docId; }
	virtual void subscribe ( const char *eventName, WebListener *listener) ;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) ;
	virtual void selectAction (const char * title,
			std::vector<std::string> &optionId,
			std::vector<std::string> &names,
			AbstractWebElement *element,
			WebListener *listener);

	virtual SmartWebPage* getWebPage ();
	virtual std::string toString () ;

	void setPage(SmartWebPage* page) {
		this->page = page;
	}

    PageData *pageData;
private:
	SmartWebPage *page;
    long docId;
};

#endif /* EXPLORERWEBAPPLICATION_H_ */
