/*
 * ExplorerElement.h
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */

#ifndef EXPLORERELEMENT_H_
#define EXPLORERELEMENT_H_

#include <AbstractWebElement.h>

class FFElement: public AbstractWebElement {
public:
	FFElement(nsCOMPtr<nsIDOMHTMLElement> &pElement);
	virtual ~FFElement();
	virtual void getChildren (std::vector<AbstractWebElement*> &children);
	virtual AbstractWebElement* getParent();
	virtual void getTagName (std::string &value);
	virtual void getAttribute (const char* attribute, std::string &value);
	virtual void setAttribute (const char* attribute, const char *value);
	virtual void focus ();
	virtual void click ();
	virtual void blur ();
	virtual AbstractWebElement* clone();
private:
	nsCOMPtr<nsIDOMHTMLElement> m_Element;
};

#endif /* EXPLORERELEMENT_H_ */
