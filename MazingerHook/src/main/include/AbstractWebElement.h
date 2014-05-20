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

class AbstractWebElement {
public:
	AbstractWebElement() {

	}
	virtual ~AbstractWebElement() {

	}
	virtual void getChildren (std::vector<AbstractWebElement*>&children) = 0;
	virtual AbstractWebElement* getParent() = 0;
	virtual void getTagName (std::string &value) = 0;
	virtual void getAttribute (const char* attribute, std::string &value) = 0;
	virtual void setAttribute (const char* attribute, const char *value) = 0;
	virtual void focus () = 0;
	virtual void click () = 0;
	virtual void blur () = 0;
	virtual AbstractWebElement* clone() = 0;
};

#endif /* ABSTRACTWEBELEMENT_H_ */
