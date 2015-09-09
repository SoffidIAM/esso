/*
 * ExplorerElement.h
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */

#ifndef FFRELEMENT_H_
#define FFRELEMENT_H_

#include <AbstractWebElement.h>

#include <WebListener.h>


class FFElement: public AbstractWebElement {
public:
	FFElement(long docId, long elementId);
	virtual void getChildren (std::vector<AbstractWebElement*> &children);
	virtual AbstractWebElement* getParent();
	virtual AbstractWebElement* getPreviousSibling();
	virtual AbstractWebElement* getNextSibling() ;
	virtual void getTagName (std::string &value);
	virtual void getProperty (const char* property, std::string &value) ;
	virtual void setProperty (const char* property, const char *value) ;
	virtual void getAttribute (const char* attribute, std::string &value);
	virtual void setAttribute (const char* attribute, const char *value);
	virtual void removeAttribute (const char* attribute);
	virtual void removeChild (AbstractWebElement* child) ;
	virtual void focus ();
	virtual void click ();
	virtual void blur ();
	virtual AbstractWebElement* clone();
	virtual void subscribe ( const char *eventName, WebListener *listener) ;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) ;
	long getDocId () { return docId; }
	long getElementId () { return elementId;}
	virtual void appendChild (AbstractWebElement *element);
	virtual void insertBefore(AbstractWebElement *element, AbstractWebElement *before);
	virtual AbstractWebApplication* getApplication () ;
	virtual bool equals (AbstractWebElement *other);
	virtual void setTextContent (const char *text);
private:
	long docId;
	long elementId;
protected:
	virtual ~FFElement();
};

#endif /* FFRELEMENT_H_ */
