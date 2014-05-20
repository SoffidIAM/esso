/*
 * ExplorerWebApplication.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef CHROMEWEBAPPLICAITON_H_
#define CHROMEWEBAPPLICAITON_H_

#include "AfroditaC.h"
#include "PluginObject.h"

#include <AbstractWebApplication.h>
namespace mazinger_chrome
{

class ChromeWebApplication: public AbstractWebApplication {
public:
	ChromeWebApplication(PluginObject *plugin, NPObject *windowObject);
	virtual ~ChromeWebApplication();

	virtual void getUrl (std::string &value) ;
	virtual void getTitle (std::string &value) ;
	virtual void getContent (std::string &value) ;
	virtual void getDomain (std::string &value) ;
	virtual void getCookie (std::string &value) ;
	virtual AbstractWebElement* getDocumentElement ();
	virtual AbstractWebElement * getElementById (const char*id) ;
	virtual void getElementsByTagName (const char*tag, std::vector<AbstractWebElement*> &elements) ;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) ;
	virtual void getForms (std::vector<AbstractWebElement*> &elements);
	virtual void getLinks (std::vector<AbstractWebElement*> &elements);
	virtual void getImages (std::vector<AbstractWebElement*> &elements);
	virtual void write (const char*str);
	virtual void writeln (const char*str) ;

private:
    NPObject *windowObject;
    PluginObject *plugin;
};
}

#endif /* CHROMEWEBAPPLICAITON_H_ */
