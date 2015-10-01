/*
 * ExplorerElement.h
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */

#ifndef EXPLORERELEMENT_H_
#define EXPLORERELEMENT_H_

#include <AbstractWebElement.h>
#include <WebListener.h>
#include <string>

class ExplorerWebApplication;
class CEventListener;
class ExplorerElement: public AbstractWebElement {
public:
	ExplorerElement(IDispatch *pdispElement, ExplorerWebApplication *app);
	virtual ~ExplorerElement();
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
	virtual AbstractWebElement* clone();
	virtual void subscribe ( const char *eventName, WebListener *listener) ;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) ;
	virtual void appendChild (AbstractWebElement *element);
	virtual void insertBefore(AbstractWebElement *element, AbstractWebElement *before);
	virtual void setTextContent (const char*text);
	virtual AbstractWebApplication* getApplication () ;
	virtual bool equals (AbstractWebElement *other) ;
	virtual std::string toString() ;
	virtual void removeAttribute (const char* attribute) ;
	virtual void removeChild (AbstractWebElement* child) ;
	virtual void setProperty (const char* property, const char *value);

	IDispatch *getIDispatch () { return m_pElement;}
private:
	IDispatch *m_pElement;
	ExplorerWebApplication *m_pApp;
	std::string m_internalId;
};

#endif /* EXPLORERELEMENT_H_ */
