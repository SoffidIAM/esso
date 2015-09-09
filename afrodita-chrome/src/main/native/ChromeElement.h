/*
 * ExplorerElement.h
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */

#ifndef CHROMEELEMENT_H_
#define CHROMEELEMENT_H_

#include <AbstractWebElement.h>
#include "AfroditaC.h"
#include "ChromeWebApplication.h"
#include <WebListener.h>

namespace mazinger_chrome
{

class ChromeElement: public AbstractWebElement {
public:
	ChromeElement (ChromeWebApplication *app, const char *externalId);
	virtual ~ChromeElement();
	virtual void getChildren (std::vector<AbstractWebElement*> &children);
	virtual AbstractWebElement* getParent();
	virtual AbstractWebElement* getPreviousSibling();
	virtual AbstractWebElement* getNextSibling() ;
	virtual void getTagName (std::string &value);
	virtual void getProperty (const char* attribute, std::string &value);
	virtual void getAttribute (const char* attribute, std::string &value);
	virtual void setAttribute (const char* attribute, const char *value);
	virtual void focus ();
	virtual void click ();
	virtual void blur ();
	virtual void appendChild (AbstractWebElement *element);
	virtual void insertBefore(AbstractWebElement *element, AbstractWebElement *before);
	virtual AbstractWebElement* clone();
	virtual void subscribe ( const char *eventName, WebListener *listener) ;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) ;
private:
	ChromeWebApplication *app;
	std::string externalId;
};

}

#endif /* CHROMEELEMENT_H_ */
