/*
 * EventHandler.h
 *
 *  Created on: 01/09/2015
 *      Author: gbuades
 */

#ifndef EVENTHANDLER_H_
#define EVENTHANDLER_H_

#include <WebListener.h>
#include "FFWebApplication.h"
#include "FFElement.h"

class EventHandler {

public:
	static EventHandler* getInstance();
	void registerEvent (WebListener* listener, FFElement *element, const char *eventName);
	void unregisterEvent (WebListener* listener, FFElement *element, const char *eventName);
	void registerEvent (WebListener* listener, FFWebApplication *element, const char *eventName);
	void unregisterEvent (WebListener* listener, FFWebApplication *element, const char *eventName);
	void unregisterAllEvents (FFWebApplication *app);
	void process (long eventId);
	void process (long eventId, long elementId);
private:
	EventHandler();
	virtual ~EventHandler();
};

#endif /* EVENTHANDLER_H_ */
