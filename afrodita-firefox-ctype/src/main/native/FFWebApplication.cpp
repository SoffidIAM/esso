/*
 * ExplorerWebApplication.cpp
 *
 *  Created on: 19/10/2010
 *      Author: u07286
 */

#include "AfroditaF.h"
#include "FFWebApplication.h"
#include "FFElement.h"
#include "EventHandler.h"

FFWebApplication::~FFWebApplication() {
}


void FFWebApplication::getUrl(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getUrlHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getUrlHandler(docId);
		if (v != NULL)
			value = v;
	}
}



void FFWebApplication::getTitle(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getTitleHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getTitleHandler(docId);
		if (v != NULL)
			value = v;
	}
}



void FFWebApplication::getContent(std::string & value)
{
	value.assign("<not supported>");
}

FFWebApplication::FFWebApplication(long docId)
{
	this->docId = docId;
}

AbstractWebElement *FFWebApplication::getDocumentElement()
{
	if (AfroditaHandler::handler.getDocumentElementHandler != NULL) {
		long id = AfroditaHandler::handler.getDocumentElementHandler (docId);
		return new FFElement (docId, id);
	} else {
		return NULL;
	}
}



void FFWebApplication::getElementsByTagName(const char*tag, std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getElementsByTagNameHandler != NULL) {
		long* id = AfroditaHandler::handler.getElementsByTagNameHandler (docId, tag);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(docId, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::getImages(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getImagesHandler != NULL) {
		long* id = AfroditaHandler::handler.getImagesHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(docId, id[i]);
			elements.push_back(element);
		}
	}
}




void FFWebApplication::getLinks(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getLinksHandler != NULL) {
		long* id = AfroditaHandler::handler.getLinksHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(docId, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::getDomain(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getDomainHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getDomainHandler(docId);
		if (v != NULL)
			value = v;
	}
}



void FFWebApplication::getAnchors(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getAnchorsHandler != NULL) {
		long* id = AfroditaHandler::handler.getAnchorsHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(docId, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::getCookie(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getCookieHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getCookieHandler(docId);
		if (v != NULL)
			value = v;
	}
}



AbstractWebElement *FFWebApplication::getElementById(const char *id)
{
	if (AfroditaHandler::handler.getElementByIdHandler != NULL) {
		long internalId = AfroditaHandler::handler.getElementByIdHandler (docId, id);
		if (internalId == 0)
			return NULL;
		else
			return new FFElement(docId, internalId);
	}
	else
		return NULL;
}



void FFWebApplication::getForms(std::vector<AbstractWebElement*> & elements)
{
	elements.clear();
	if (AfroditaHandler::handler.getFormsHandler != NULL) {
		long* id = AfroditaHandler::handler.getFormsHandler (docId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(docId, id[i]);
			elements.push_back(element);
		}
	}
}



void FFWebApplication::write(const char *str)
{
	if (AfroditaHandler::handler.writeHandler != NULL) {
		AfroditaHandler::handler.writeHandler(docId, str);
	}
}

void FFWebApplication::writeln(const char *str)
{
	if (AfroditaHandler::handler.writeLnHandler != NULL) {
		AfroditaHandler::handler.writeLnHandler(docId, str);
	}
}

AbstractWebElement* FFWebApplication::createElement(const char* tagName) {
	if (AfroditaHandler::handler.createElementHandler != NULL) {
		long internalId = AfroditaHandler::handler.createElementHandler(docId, tagName);
		if (internalId == 0)
			return NULL;
		else
			return new FFElement(docId, internalId);
	}
	else
		return NULL;
}

SmartWebPage* FFWebApplication::getWebPage() {
	return page;
}

void FFWebApplication::alert(const char* str) {
	if (AfroditaHandler::handler.alertHandler != NULL)
		AfroditaHandler::handler.alertHandler(docId, str);
}

void FFWebApplication::subscribe(const char* eventName, WebListener* listener) {
	EventHandler::getInstance()->registerEvent(listener, this, eventName);
}

void FFWebApplication::unSubscribe(const char* eventName,
		WebListener* listener) {
	EventHandler::getInstance()->unregisterEvent(listener, this, eventName);
}

