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
#include "PluginObject.h"

namespace mazinger_chrome
{

class ChromeElement: public AbstractWebElement {
public:
	ChromeElement (PluginObject *plugin, NPObject *object);
	virtual ~ChromeElement();
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
	PluginObject *plugin;
	NPObject* elementObject;
};

}

#endif /* CHROMEELEMENT_H_ */
