/*
 * WebApplication.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef ABSTRACTWEBAPPLICATION_H_
#define ABSTRACTWEBAPPLICATION_H_

#include <string>
#include <vector>
#include <LockableObject.h>
#include "PageData.h"

class AbstractWebElement;
class SmartWebPage;
class WebListener;

class AbstractWebApplication: public LockableObject {
public:
	AbstractWebApplication() {

	}
protected:
	virtual ~AbstractWebApplication() {
	}
public:
	virtual bool supportsPageData () = 0;
	virtual PageData* getPageData() = 0;
	virtual void getUrl (std::string &value) = 0;
	virtual void getTitle (std::string &value) = 0;
	virtual void getContent (std::string &value) = 0;
	virtual void getDomain (std::string &value) = 0;
	virtual void getCookie (std::string &value) = 0;
	virtual AbstractWebElement* getDocumentElement () = 0;
	virtual AbstractWebElement * getElementById (const char*id) = 0;
	virtual AbstractWebElement * getElementBySoffidId (const char*id) = 0;
	virtual AbstractWebElement * createElement(const char *tagName) = 0;
	virtual void getElementsByTagName (const char* tag, std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getForms (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getLinks (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getImages (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void write (const char*str) = 0;
	virtual void writeln (const char*str) = 0;
	virtual void alert (const char *str) = 0;
	virtual void subscribe ( const char *eventName, WebListener *listener) = 0;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) = 0;
	virtual void selectAction (const char * title,
			std::vector<std::string> &optionId,
			std::vector<std::string> &names,
			AbstractWebElement *element,
			WebListener *listener);

	virtual SmartWebPage* getWebPage () = 0;
};
#endif /* WEBAPPLICATION_H_ */
