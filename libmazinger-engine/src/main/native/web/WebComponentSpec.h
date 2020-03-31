/*
 * WebComponentSpec.h
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#ifndef WEBCOMPONENTSPEC_H_
#define WEBCOMPONENTSPEC_H_

class AbstractWebElement;
class AbstractWebApplication;

#include <vector>
#include <pcreposix.h>
#include <PageData.h>

class WebComponentSpec {

public:
	WebComponentSpec();
	virtual ~WebComponentSpec();

	regex_t *reId;
	regex_t *reName;
	char *szId;
	char *szName;
	char *szRefAs;
	bool m_bOptional;
	std::vector<WebComponentSpec*> m_children;

	virtual bool matches (AbstractWebElement *element);
	virtual bool matches (AbstractWebApplication *app, FormData &form);
	virtual bool matches (AbstractWebApplication *app, InputData &input);
	virtual void dump ()=0;
	bool matchAttribute (regex_t *expr, AbstractWebElement *element, const char*attributeName) ;
	bool matchValue (regex_t *expr, const char*value) ;

	virtual void clearMatches ();
	virtual void setMatched(AbstractWebElement *element);

	AbstractWebElement *getMatched() {return m_pElement;}
	virtual const char* getTag() = 0;
private:
	AbstractWebElement *m_pElement;
};

#endif /* WEBCOMPONENTSPEC_H_ */
