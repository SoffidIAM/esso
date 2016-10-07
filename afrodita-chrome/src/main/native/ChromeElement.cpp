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
#include <MazingerInternal.h>
#include <PageData.h>

namespace mazinger_chrome
{

ChromeElement::ChromeElement(ChromeWebApplication *app, const char *externalId)
{
	this -> app = app;
	this -> externalId = externalId;
	app->lock();

}

ChromeElement::~ChromeElement() {
	app->release();
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

	if (response != NULL && ! error && response->value.length() > 0)
	{
		ChromeElement* result = new ChromeElement(app, response->value.c_str());
		delete response;
		return result;
	}
	else
		return NULL;
}

AbstractWebElement *ChromeElement::getOffsetParent()
{
	bool error;
	const char * msg [] = {"action","getOffsetParent", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error  && response->value.length() > 0)
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

	InputData *d = findInputData();
	if (d != NULL)
	{
		if (strcmp (attribute, "value") == 0)
			d->value = value;
		if (strcmp (attribute, "name") == 0)
			d->name = value;
		if (strcmp (attribute, "id") == 0)
			d->id = value;
		if (strcmp (attribute, "type") == 0)
			d->type = value;
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
	std::string listenerId = CommunicationManager::getInstance()
		->registerListener (this, eventName, listener);

	const char * msg [] = {"action","subscribe",
			"pageId", app->threadStatus->pageId.c_str(),
			"element", externalId.c_str(),
			"event", eventName,
			"listener", listenerId.c_str(), NULL};
	bool error;

	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL)
	{
		delete response;
	}
}

AbstractWebElement* ChromeElement::getPreviousSibling() {
	const char * msg [] = {"action","getPreviousSibling", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error  && response->value.length() > 0)
	{
		ChromeElement* result = new ChromeElement(app, response->value.c_str());
		delete response;
		return result;
	}
	return NULL;
}

AbstractWebElement* ChromeElement::getNextSibling() {
	const char * msg [] = {"action","getNextSibling", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error  && response->value.length() > 0)
	{
		ChromeElement* result = new ChromeElement(app, response->value.c_str());
		delete response;
		return result;
	}
	return NULL;
}

void ChromeElement::appendChild(AbstractWebElement* element) {
	ChromeElement *child = dynamic_cast<ChromeElement*> (element);
	if (child != NULL)
	{
		const char * msg [] = {"action","appendChild", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
					"child", child->externalId.c_str(),
					NULL};
		bool error;
		json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

		if (response != NULL)
		{
			delete response;
		}
	}
}

void ChromeElement::insertBefore(AbstractWebElement* element,
		AbstractWebElement* before) {
	if (before == NULL)
		appendChild(element);
	else
	{
		ChromeElement *child = dynamic_cast<ChromeElement*> (element);
		ChromeElement *sibling = dynamic_cast<ChromeElement*> (before);
		if (child != NULL &&  sibling != NULL)
		{
			const char * msg [] = {"action","insertBefore", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
						"child", child->externalId.c_str(),
						"before", sibling->externalId.c_str(),
						NULL};
			bool error;
			json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

			if (response != NULL && ! error)
			{
				delete response;
			}
		}
	}
}

void ChromeElement::unSubscribe(const char* eventName, WebListener* listener) {
	std::string listenerId = CommunicationManager::getInstance()->unregisterListener (this, eventName, listener);

	const char * msg [] = {"action","unSubscribe",
			"pageId", app->threadStatus->pageId.c_str(),
			"element", externalId.c_str(),
			"event", eventName,
			"listener", listenerId.c_str(),
			NULL};
	bool error;


	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL)
	{
		delete response;
	}
}

void ChromeElement::setTextContent(const char* text) {
	const char * msg [] = {"action","setTextContent", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
			"text", text, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		delete response;
	}
}

AbstractWebApplication* ChromeElement::getApplication() {
	return app;
}

bool ChromeElement::equals(AbstractWebElement* other) {
	ChromeElement *other2 = dynamic_cast<ChromeElement*> (other);
	return other2 != NULL && other2->externalId == externalId;
}

std::string ChromeElement::toString() {
	return std::string("ChromeElement "+externalId);
}

void ChromeElement::removeAttribute(const char* attribute) {
	const char * msg [] = {"action","removeAttribute", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
			"attribute", attribute, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		delete response;
	}
}

void ChromeElement::removeChild(AbstractWebElement* element) {
	ChromeElement *child = dynamic_cast<ChromeElement*> (element);
	if (child != NULL)
	{
		const char * msg [] = {"action","removeChild", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
					"child", child->externalId.c_str(),
					NULL};
		bool error;
		json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

		if (response != NULL && ! error)
		{
			delete response;
		}
	}
}

void ChromeElement::setProperty(const char* property, const char* value) {
	const char * msg [] = {"action","setProperty", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(),
			"attribute", property, "value", value, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		delete response;
	}
	InputData *d = findInputData();
	if (d != NULL)
	{
		if (strcmp (property, "value")  == 0)
			d->value = value;
		if (strcmp (property, "name")  == 0)
			d->name = value;
		if (strcmp (property, "id")  == 0)
			d->id = value;
		if (strcmp (property, "type") == 0)
			d->type = value;
	}
}

std::string ChromeElement::getComputedStyle(const char* style) {
	std::string value;
	std::string tag;
	getTagName(tag);
	if (tag.size() == 0)
		return std::string("");

	const char * msg [] = {"action","getComputedStyle", "pageId", app->threadStatus->pageId.c_str(), "element", externalId.c_str(), "style", style, NULL};
	bool error;
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
	else
	{
		MZNSendDebugMessage("Error on %s: %s", tag.c_str(), toString().c_str());
	}
	return value;
}

InputData* ChromeElement::findInputData() {
	PageData* pageData = app->getPageData();
	if (pageData == NULL)
		return NULL;
	for (std::vector<InputData>::iterator it = pageData->inputs.begin();
			it != pageData->inputs.end();
			it++)
	{
		InputData &d = *it;
		if (d.soffidId == externalId)
			return &d;
	}
	for (std::vector<FormData>::iterator it2 = pageData->forms.begin();
			it2 != pageData->forms.end();
			it2++)
	{
		FormData &f = *it2;
		for (std::vector<InputData>::iterator it = f.inputs.begin();
				it != f.inputs.end();
				it++)
		{
			InputData &d = *it;
			if (d.soffidId == externalId)
				return &d;
		}
	}
	return NULL;
}
}

