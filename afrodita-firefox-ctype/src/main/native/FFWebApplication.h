/*
 * ExplorerWebApplication.h
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#ifndef EXPLORERWEBAPPLICATION_H_
#define EXPLORERWEBAPPLICATION_H_


#include <AbstractWebApplication.h>

class FFWebApplication: public AbstractWebApplication {
public:
	FFWebApplication(long docId);
	virtual ~FFWebApplication();

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
    long docId;
};

#endif /* EXPLORERWEBAPPLICATION_H_ */
