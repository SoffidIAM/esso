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

class AbstractWebElement;

class AbstractWebApplication {
public:
	AbstractWebApplication() {

	}
	virtual ~AbstractWebApplication() {

	}
	virtual void getUrl (std::string &value) = 0;
	virtual void getTitle (std::string &value) = 0;
	virtual void getContent (std::string &value) = 0;
	virtual void getDomain (std::string &value) = 0;
	virtual void getCookie (std::string &value) = 0;
	virtual AbstractWebElement* getDocumentElement () = 0;
	virtual AbstractWebElement * getElementById (const char*id) = 0;
	virtual void getElementsByTagName (const char* tag, std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getAnchors (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getForms (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getLinks (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void getImages (std::vector<AbstractWebElement*> &elements) = 0;
	virtual void write (const char*str) = 0;
	virtual void writeln (const char*str) = 0;
};
#endif /* WEBAPPLICATION_H_ */
