/*
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */


#include "AfroditaC.h"
#include "ChromeElement.h"
#include <stdio.h>
#include <string.h>
#include "json/JsonAbstractObject.h"
#include "json/JsonValue.h"
#include "json/JsonMap.h"
#include "json/JsonVector.h"
#include "CommunicationManager.h"

namespace mazinger_chrome
{

ChromeElement::ChromeElement(ChromeWebApplication *app, const char *externalId)
{
	this -> app = app;
	this -> externalId = externalId;

}

ChromeElement::~ChromeElement() {
}



void ChromeElement::getChildren(std::vector<AbstractWebElement*> &children)
{
	const char * msg [] = {"action","getChildren", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	children.clear ();
	bool error;
	json::JsonVector* response =
			dynamic_cast<json::JsonVector*>(CommunicationManager::getInstance()->call(
					error, msg));
	if (response != NULL && !error) {
		for (std::vector<json::JsonAbstractObject*>::iterator it =
				response->objects.begin(); it != response->objects.end();
				it++) {
			json::JsonValue *v = dynamic_cast<json::JsonValue*>(*it);
			if (v != NULL) {
				ChromeElement *e = new ChromeElement(this->app, v->value.c_str());
				children.push_back(e);
			}
		}
		delete response;
	}
}



void ChromeElement::click()
{
	const char * msg [] = {"action","click", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	bool error;
	json::JsonAbstractObject* response = CommunicationManager::getInstance()->call(
					error, msg);
	if (response != NULL) {
		delete response;
	}
}



void ChromeElement::getAttribute(const char *attribute, std::string & value)
{
	const char * msg [] = {"action","getAttribute", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), "attribute", attribute, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}

}

void ChromeElement::getProperty(const char *attribute, std::string & value)
{
	const char * msg [] = {"action","getProperty", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), "attribute", attribute, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}

}


void ChromeElement::blur()
{
	const char * msg [] = {"action","blur", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	bool error;
	json::JsonAbstractObject* response = CommunicationManager::getInstance()->call(
					error, msg);
	if (response != NULL) {
		delete response;
	}
}



AbstractWebElement *ChromeElement::getParent()
{
	bool error;
	const char * msg [] = {"action","getParent", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		ChromeElement* result = new ChromeElement(app, response->value.c_str());
		delete response;
		return result;
	}
	else
		return NULL;
}



void ChromeElement::setAttribute(const char *attribute, const char*value)
{
	const char * msg [] = {"action","setAttribute", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
			"attribute", attribute, "value", value, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		delete response;
	}

}



void ChromeElement::focus()
{
	const char * msg [] = {"action","focus", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	bool error;
	json::JsonAbstractObject* response = CommunicationManager::getInstance()->call(
					error, msg);
	if (response != NULL) {
		delete response;
	}
}



void ChromeElement::getTagName(std::string & value)
{
	const char * msg [] = {"action","getTagName", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}



AbstractWebElement *ChromeElement::clone()
{
	return new ChromeElement(app, externalId.c_str());
}

void ChromeElement::subscribe(const char* eventName, WebListener* listener) {
}

AbstractWebElement* ChromeElement::getPreviousSibling() {
}

AbstractWebElement* ChromeElement::getNextSibling() {
}

void ChromeElement::appendChild(AbstractWebElement* element) {
}

void ChromeElement::insertBefore(AbstractWebElement* element,
		AbstractWebElement* before) {
}

void ChromeElement::unSubscribe(const char* eventName, WebListener* listener) {
}

}

