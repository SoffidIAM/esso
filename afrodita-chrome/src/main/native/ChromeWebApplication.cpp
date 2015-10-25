/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include "AfroditaC.h"
#include "ChromeWebApplication.h"
#include "ChromeElement.h"
#include <string.h>
#include <stdio.h>

#include "json/JsonMap.h"
#include "json/JsonVector.h"
#include "json/JsonValue.h"

#include "CommunicationManager.h"
#include <MazingerInternal.h>

namespace mazinger_chrome
{


ChromeWebApplication::~ChromeWebApplication() {
	this->webPage->release();
}


void ChromeWebApplication::getUrl(std::string & value)
{
	value = url;
}



void ChromeWebApplication::getTitle(std::string & value)
{
	bool error;
	const char * msg [] = {"action","getTitle", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}



void ChromeWebApplication::getContent(std::string & value)
{
	value.assign("<not supported>");
}


AbstractWebElement *ChromeWebApplication::getDocumentElement()
{
	bool error;
	const char * msg [] = {"action","getDocumentElement", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		ChromeElement* result = new ChromeElement(this, response->value.c_str());
		delete response;
		return result;
	}
	else
		return NULL;
}



void ChromeWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getElementsByTagName", "pageId", threadStatus->pageId.c_str(), "tagName", tag, NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getImages", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}




void ChromeWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getLinks", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::getDomain(std::string & value)
{
	bool error;
	const char * msg [] = {"action","getDomain", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}

std::string ChromeWebApplication::toString() {
	return std::string("ChromeWebApplication ")+threadStatus->pageId;
}

void ChromeWebApplication::generateCollection(const char* msg[],
		std::vector<AbstractWebElement*>& elements) {
	elements.clear ();
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
				ChromeElement *e = new ChromeElement(this, v->value.c_str());
				elements.push_back(e);
			}
		}
		delete response;
	}
}

void ChromeWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getAnchors", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::getCookie(std::string & value)
{
	bool error;
	const char * msg [] = {"action","getCookie", "pageId", threadStatus->pageId.c_str(), NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error)
	{
		value = response->value;
		delete response;
	}
}



AbstractWebElement *ChromeWebApplication::getElementById(const char *id)
{
	bool error;
	const char * msg [] = {"action","getElementById", "pageId", threadStatus->pageId.c_str(), "elementId", id, NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL && ! error && response->value.size() > 0)
	{
		ChromeElement* result = new ChromeElement(this, response->value.c_str());
		delete response;
		return result;
	}
	else
		return NULL;
}



void ChromeWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	const char * msg [] = {"action","getForms", "pageId", threadStatus->pageId.c_str(), NULL};
	generateCollection(msg, elements);
}



void ChromeWebApplication::write(const char *text)
{
	bool error;
	const char * msg [] = {"action","write", "pageId", threadStatus->pageId.c_str(), "text", text, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}

void ChromeWebApplication::writeln(const char *text)
{
	bool error;
	const char * msg [] = {"action","writeln", "pageId", threadStatus->pageId.c_str(), "text", text, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}




ChromeWebApplication::ChromeWebApplication(ThreadStatus *threadStatus) {
	this->threadStatus = threadStatus;
	title = threadStatus->title;
	url = threadStatus->url;
	this->webPage = new SmartWebPage;
}


AbstractWebElement* ChromeWebApplication::createElement(const char* tagName) {
	bool error;
	AbstractWebElement * result = NULL;
	const char * msg [] = {"action","createElement", "pageId", threadStatus->pageId.c_str(), "tagName", tagName, NULL};
	json::JsonValue * response = dynamic_cast<json::JsonValue*>(CommunicationManager::getInstance()->call(error,msg));

	if (response != NULL )
	{
		if (! error  && !response->value.empty())
			result = new ChromeElement(this, response->value.c_str());
		delete response;
	}
	return result;
}

void ChromeWebApplication::alert(const char* str) {
	bool error;
	const char * msg [] = {"action","alert", "pageId", threadStatus->pageId.c_str(), "text", str, NULL};
	json::JsonAbstractObject * response = CommunicationManager::getInstance()->call(error,msg);

	if (response != NULL)
		delete response;
}

void ChromeWebApplication::subscribe(const char* eventName,
		WebListener* listener) {
}

void ChromeWebApplication::unSubscribe(const char* eventName,
		WebListener* listener) {
}

}

