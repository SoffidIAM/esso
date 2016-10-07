/*
 * EventHandler.cpp
 *
 *  Created on: 01/09/2015
 *      Author: gbuades
 */

#include "EventHandler.h"
#include <stdio.h>
#include <list>
#include "AfroditaF.h"
#include "FFElement.h"
#include "FFWebApplication.h"

class EventDispatcher {
public:
	long docId;
	long elementId;
	long eventId;
	std::string eventName;
	WebListener* listener;
};

static std::list<EventDispatcher*> dispatchers;
static long eventId = 0;

EventHandler::EventHandler() {
}

EventHandler* EventHandler::getInstance() {
	static EventHandler *handler = NULL;
	if (handler == NULL)
		handler = new EventHandler();
	return handler;
}

void EventHandler::registerEvent(WebListener* listener, FFElement* element,
		const char* eventName) {
	EventDispatcher *d = new EventDispatcher();
	d->elementId = element->getElementId();
	d->docId = element->getDocId();
	d->listener = listener;
	d->eventId = eventId++;
	d->eventName = eventName;
	d->listener->lock();
	dispatchers.push_back(d);

	AfroditaHandler::handler.subscribeHandler(d->docId, d->elementId,
			d->eventName.c_str(), d->eventId);
}

void EventHandler::unregisterEvent(WebListener* listener, FFElement* element,
		const char* eventName) {
	for (std::list<EventDispatcher*>::iterator it = dispatchers.begin();
			it != dispatchers.end(); it++) {
		EventDispatcher *d = *it;
		if (d->elementId == element->getElementId()
				&& d->docId == element->getDocId()
				&& d->eventName == eventName) {
			AfroditaHandler::handler.unsubscribeHandler(d->docId, d->elementId,
					d->eventName.c_str(), d->eventId);
			d->listener->release();
			dispatchers.erase(it);
			return;
		}
	}
}

void EventHandler::registerEvent(WebListener* listener, FFWebApplication* element,
		const char* eventName) {

	EventDispatcher *d = new EventDispatcher();
	d->elementId = 0;
	d->docId = element->getDocId();
	d->listener = listener;
	d->eventId = eventId++;
	d->eventName = eventName;
	d->listener->lock();
	dispatchers.push_back(d);

	AfroditaHandler::handler.subscribeHandler(d->docId, d->elementId,
			d->eventName.c_str(), d->eventId);
}

void EventHandler::unregisterEvent(WebListener* listener, FFWebApplication* element,
		const char* eventName) {
	for (std::list<EventDispatcher*>::iterator it = dispatchers.begin();
			it != dispatchers.end(); it++) {
		EventDispatcher *d = *it;
		if (d->elementId == 0
				&& d->docId == element->getDocId()
				&& d->eventName == eventName) {
			AfroditaHandler::handler.unsubscribeHandler(d->docId, d->elementId,
					d->eventName.c_str(), d->eventId);
			d->listener->release();
			dispatchers.erase(it);
			return;
		}
	}
}

void EventHandler::unregisterAllEvents(FFWebApplication* app) {
	std::list<EventDispatcher*>::iterator it = dispatchers.begin();
	while (it != dispatchers.end()) {
		EventDispatcher *d = *it;
		if (d->docId == app->getDocId()) {
			AfroditaHandler::handler.unsubscribeHandler(d->docId, d->elementId,
					d->eventName.c_str(), d->eventId);
			d->listener->release();
			it = dispatchers.erase(it);
		} else
			it++;
	}
}

EventHandler::~EventHandler() {
}

void EventHandler::process(long eventId) {
	std::list<EventDispatcher*>::iterator it = dispatchers.begin();
	while (it != dispatchers.end()) {
		EventDispatcher *d = *it;
		if (d->eventId == eventId) {
			FFWebApplication *app = new FFWebApplication(d->docId);
			FFElement *element = d->elementId == 0 ? NULL: new FFElement(app, d->elementId);
			d->listener->onEvent(d->eventName.c_str(), app, element);
			if (element != NULL)
				element->release();
			if (app != NULL)
				app->release();
			break;
		} else
			it++;
	}
}

void EventHandler::process(long eventId, long elementId) {
	std::list<EventDispatcher*>::iterator it = dispatchers.begin();
	while (it != dispatchers.end()) {
		EventDispatcher *d = *it;
		if (d->eventId == eventId) {
			FFWebApplication *app = new FFWebApplication(d->docId);
			FFElement *element = new FFElement(app, elementId);
			d->listener->onEvent(d->eventName.c_str(), app, element);
			if (element != NULL)
				element->release();
			if (app != NULL)
				app->release();
			break;
		} else
			it++;
	}
}
