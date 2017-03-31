/*
 * WebFormSpec.h
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#ifndef WEBLISTENER_H_
#define WEBLISTENER_H_

#include <LockableObject.h>
#include <string>

class AbstractWebElement;
class AbstractWebApplication;

class WebListener : public LockableObject{
public:
	WebListener() { };
protected:
	virtual ~WebListener() {} ;
public:
	virtual void onEvent (const char *eventName, AbstractWebApplication *app, AbstractWebElement *component, const char *data) = 0;
};

#endif /* WEBLISTENER_H_ */
