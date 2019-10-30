/*
 * ExplorerWebApplication.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef CHROMEWEBAPPLICAITON_H_
#define CHROMEWEBAPPLICAITON_H_

#include "AfroditaC.h"
#include "ThreadStatus.h"

#include <AbstractWebApplication.h>
#include <SmartWebPage.h>
class PageData;

namespace mazinger_chrome
{

class ThreadStatus;

class ChromeWebApplication: public AbstractWebApplication {
public:
    ChromeWebApplication(ThreadStatus *thread);
	virtual ~ChromeWebApplication();

	virtual bool supportsPageData () ;
	virtual PageData* getPageData() ;
	virtual void getUrl (std::string &value) ;
	virtual void getTitle (std::string &value) ;
	virtual void getContent (std::string &value) ;
	virtual void getDomain (std::string &value) ;
	virtual void getCookie (std::string &value) ;
	virtual AbstractWebElement* getDocumentElement ();
	virtual AbstractWebElement * getElementBySoffidId (const char*id) ;
	virtual AbstractWebElement * getElementById (const char*id) ;
	virtual void getElementsByTagName (const char*tag, std::vector<AbstractWebElement*> &elements) ;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) ;
	virtual void getForms (std::vector<AbstractWebElement*> &elements);
	virtual void getLinks (std::vector<AbstractWebElement*> &elements);
	virtual void getImages (std::vector<AbstractWebElement*> &elements);
	virtual void write (const char*str);
	virtual void writeln (const char*str) ;
	virtual AbstractWebElement * createElement(const char *tagName) ;
	virtual void alert (const char *str);
	virtual void subscribe ( const char *eventName, WebListener *listener) ;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) ;
	virtual std::string toString ();
	virtual void selectAction (const char * title,
			std::vector<std::string> &optionId,
			std::vector<std::string> &names,
			AbstractWebElement *element,
			WebListener *listener) ;

	ThreadStatus *threadStatus;
	virtual SmartWebPage* getWebPage () { return webPage;}
	void setPageData (PageData *pd) {
		this->pageData = pd;
		if (pd != NULL)
		{
			this->url = pd->url;
			this->title = pd->title;
		}
	}
	bool equals(ChromeWebApplication *app);

	void releaseWebPage();
	virtual void lock ();
	virtual void release ();

private:
    std::string url;
    std::string title;
    SmartWebPage *webPage;
    PageData* pageData;

	void generateCollection(const char* msg[],
			std::vector<AbstractWebElement*>& elements);
};
}

#endif /* CHROMEWEBAPPLICAITON_H_ */
