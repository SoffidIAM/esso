/*
 * WebInputSpec.h
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#ifndef WEBINPUTSPEC_H_
#define WEBINPUTSPEC_H_

#include "WebComponentSpec.h"

class WebInputSpec: public WebComponentSpec {
public:
	WebInputSpec();
	virtual ~WebInputSpec();
	regex_t *reType;
	regex_t *reValue;
	char *szType;
	char *szValue;

	virtual bool matches (AbstractWebElement *element);
	virtual const char* getTag();
	virtual void dump ();

};

#endif /* WEBINPUTSPEC_H_ */
