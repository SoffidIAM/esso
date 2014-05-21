/*
 * WebFormSpec.h
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#ifndef WEBFORMSPEC_H_
#define WEBFORMSPEC_H_

#include "WebComponentSpec.h"
#include <vector>

class WebInputSpec;

class WebFormSpec: public WebComponentSpec {
public:
	WebFormSpec();
	virtual ~WebFormSpec();

	regex_t *reAction;
	regex_t *reMethod;
	char *szAction;
	char *szMethod;

	virtual bool matches (AbstractWebElement *element);
	virtual const char* getTag() ;
	virtual void dump ();

private:
	void findChildren (AbstractWebElement *childElement);
};

#endif /* WEBFORMSPEC_H_ */
