/*
 * AbstractWebElement.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef ABSTRACTWEBELEMENT_H_
#define ABSTRACTWEBELEMENT_H_

#include <vector>
#include <string>

#include <WebListener.h>
#include <LockableObject.h>

class AbstractWebApplication;

class AbstractWebElement: public LockableObject {
public:
	AbstractWebElement() {

	}

	bool isVisible()
	{
		AbstractWebElement *parent = getOffsetParent();
		if (parent == NULL)
		{
			return false;
		}
		else
		{
			parent->release();
			return true;
		}
	}
protected:
	virtual ~AbstractWebElement() {

	}
public:
	virtual void getChildren (std::vector<AbstractWebElement*>&children) = 0;
	virtual void appendChild (AbstractWebElement *element) = 0;
	virtual void insertBefore(AbstractWebElement *element, AbstractWebElement *before) = 0;
	virtual AbstractWebElement* getParent() = 0;
	virtual AbstractWebElement* getOffsetParent() = 0;
	virtual AbstractWebElement* getPreviousSibling() = 0;
	virtual AbstractWebElement* getNextSibling() = 0;
	virtual void removeChild (AbstractWebElement* child) = 0;
	virtual void getTagName (std::string &value) = 0;
	virtual void getAttribute (const char* attribute, std::string &value) = 0;
	virtual void getProperty (const char* property, std::string &value) = 0;
	virtual void setProperty (const char* property, const char *value) = 0;
	virtual void setAttribute (const char* attribute, const char *value) = 0;
	virtual void removeAttribute (const char* attribute) = 0;
	virtual void focus () = 0;
	virtual void click () = 0;
	virtual void blur () = 0;
	virtual AbstractWebElement* clone() = 0;
	virtual void subscribe ( const char *eventName, WebListener *listener) = 0;
	virtual void unSubscribe ( const char *eventName, WebListener *listener) = 0;
	virtual void setTextContent (const char*text) = 0;
	virtual AbstractWebApplication* getApplication () = 0;
	virtual bool equals (AbstractWebElement *other) = 0;
	virtual std::string toString() = 0;
	virtual std::string getComputedStyle(const char* style) = 0;

};

#endif /* ABSTRACTWEBELEMENT_H_ */
