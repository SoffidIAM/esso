/*
 * ExplorerElement.cpp
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */
#define NS_SCRIPTABLE


#include "AfroditaF.h"
#include "FFElement.h"
#include "FFWebApplication.h"
#include <stdio.h>
#include "EventHandler.h"
#include <PageData.h>
#include <string.h>

FFElement::FFElement(FFWebApplication *app, long elementId)
{
	this->app = app;
	this->docId = app->getDocId();
	this->elementId = elementId;
	this->app->lock();
}

FFElement::~FFElement() {
	this->app->release();
}



void FFElement::getChildren(std::vector<AbstractWebElement*> &children)
{
	children.clear();
	if (AfroditaHandler::handler.getChildrenHandler != NULL) {
		const long* id = AfroditaHandler::handler.getChildrenHandler (docId, elementId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(app, id[i]);
			children.push_back(element);
		}
	}
}



void FFElement::click()
{
	if ( AfroditaHandler::handler.clickHandler != NULL)
	{
		AfroditaHandler::handler.clickHandler(docId, elementId);
	}
}



void FFElement::getAttribute(const char *attribute, std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getAttributeHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getAttributeHandler(docId, elementId, attribute);
		if (v != NULL)
			value = v;
	}
}

void FFElement::removeAttribute(const char* attribute) {
	if ( AfroditaHandler::handler.removeAttributeHandler != NULL)
	{
		AfroditaHandler::handler.removeAttributeHandler(docId, elementId, attribute);
	}
}



void FFElement::blur()
{
	if ( AfroditaHandler::handler.blurHandler != NULL)
	{
		AfroditaHandler::handler.blurHandler(docId, elementId);
	}
}



AbstractWebElement *FFElement::getParent()
{
	if ( AfroditaHandler::handler.getParentHandler != NULL)
	{
		long id = AfroditaHandler::handler.getParentHandler(docId, elementId);
		if (id != 0)
			return new FFElement (app, id);
	}
	return NULL;
}

AbstractWebElement *FFElement::getOffsetParent()
{
	if ( AfroditaHandler::handler.getOffsetParentHandler != NULL)
	{
		long id = AfroditaHandler::handler.getOffsetParentHandler(docId, elementId);
		if (id != 0)
			return new FFElement (app, id);
	}
	return NULL;
}



void FFElement::setAttribute(const char *attribute, const char*value)
{
	if ( AfroditaHandler::handler.setAttributeHandler != NULL)
	{
		AfroditaHandler::handler.setAttributeHandler(docId, elementId, attribute, value);
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



void FFElement::focus()
{
	if ( AfroditaHandler::handler.focusHandler != NULL)
	{
		AfroditaHandler::handler.focusHandler(docId, elementId);
	}
}



void FFElement::getTagName(std::string & value)
{
	value.clear ();
	if ( AfroditaHandler::handler.getTagNameHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getTagNameHandler(docId, elementId);
		if (v != NULL)
			value = v;
	}
}



AbstractWebElement *FFElement::clone()
{
	return new FFElement(app, elementId);
}


void  FFElement::subscribe ( const char *eventName, WebListener *listener)
{
	EventHandler::getInstance()->registerEvent(listener, this, eventName);
}

void  FFElement::unSubscribe ( const char *eventName, WebListener *listener)
{
	EventHandler::getInstance()->unregisterEvent(listener, this, eventName);
}

void FFElement::getProperty(const char* property, std::string& value) {
	value.clear ();
	if ( AfroditaHandler::handler.getPropertyHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getPropertyHandler(docId, elementId, property);
		if (v != NULL)
			value = v;
	}
}


void FFElement::appendChild(AbstractWebElement* element) {
	FFElement * ffElement = dynamic_cast<FFElement*> (element);
	if ( AfroditaHandler::handler.appendChildHandler != NULL && ffElement != NULL)
	{
		AfroditaHandler::handler.appendChildHandler(docId, elementId, ffElement->elementId);
	}
}

void FFElement::insertBefore(AbstractWebElement* element,
		AbstractWebElement* before) {
	FFElement * ffElement = dynamic_cast<FFElement*> (element);
	FFElement * ffElementBefore = dynamic_cast<FFElement*> (before);
	if ( AfroditaHandler::handler.insertBeforeHandler != NULL &&
			AfroditaHandler::handler.appendChildHandler != NULL &&
			ffElement != NULL)
	{
		if (ffElementBefore == NULL)
		{
			AfroditaHandler::handler.appendChildHandler(docId, elementId, ffElement->elementId);
		}
		else
		{
			AfroditaHandler::handler.insertBeforeHandler(docId, elementId, ffElement->elementId, ffElementBefore->elementId);
		}
	}
}

AbstractWebElement* FFElement::getPreviousSibling() {
	if ( AfroditaHandler::handler.getPreviousSiblingHandler != NULL)
	{
		long id = AfroditaHandler::handler.getPreviousSiblingHandler(docId, elementId);
		if (id != 0)
			return new FFElement (app, id);
	}
	return NULL;
}

AbstractWebElement* FFElement::getNextSibling() {
	if ( AfroditaHandler::handler.getNextSiblingHandler != NULL)
	{
		long id = AfroditaHandler::handler.getNextSiblingHandler(docId, elementId);
		if (id != 0)
			return new FFElement (app, id);
	}
	return NULL;
}

AbstractWebApplication* FFElement::getApplication() {
	app->lock();
	return app;
}

bool FFElement::equals(AbstractWebElement* other) {
	FFElement *ffElement = dynamic_cast<FFElement*>(other);
	return ffElement != NULL && ffElement->docId == docId && ffElement->elementId == elementId;
}

void FFElement::setProperty(const char* property, const char* value) {
	if ( AfroditaHandler::handler.setPropertyHandler != NULL)
	{
		AfroditaHandler::handler.setPropertyHandler(docId, elementId, property, value);
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
		if (strcmp (property, "soffidMirrorOf") == 0)
			d->mirrorOf = value;
		if (strcmp (property, "soffidInputType") == 0)
			d->inputType = value;
	}
}

void FFElement::setTextContent(const char* value) {
	if ( AfroditaHandler::handler.setTextContentHandler != NULL)
	{
		AfroditaHandler::handler.setTextContentHandler(docId, elementId, value);
	}
}

void FFElement::removeChild(AbstractWebElement* child) {
	FFElement *ffElement = dynamic_cast<FFElement*>(child);
	if ( AfroditaHandler::handler.removeChildHandler != NULL && ffElement != NULL)
	{
		AfroditaHandler::handler.removeChildHandler(docId, elementId, ffElement->elementId);
	}
}

std::string FFElement::getComputedStyle(const char* style)
{
	std::string value;
	if ( AfroditaHandler::handler.getComputedStyleHandler != NULL)
	{
		const char *v  = AfroditaHandler::handler.getComputedStyleHandler(docId, elementId, style);
		if (v != NULL)
			value = v;
	}
	return value;
}


std::string FFElement::toString() {
	char ach[100];
	sprintf (ach, "FFElement %ld/%ld ", docId, elementId);
	std::string result = ach;
	std::string tag;
	getTagName(tag);
	result += tag;
	return result;
}

InputData* FFElement::findInputData() {
	if (app->pageData == NULL)
		return NULL;
	char ach[100];
	sprintf (ach, "%ld", elementId);
	for (std::vector<InputData>::iterator it = app->pageData->inputs.begin();
			it != app->pageData->inputs.end();
			it++)
	{
		InputData &d = *it;
		if (d.soffidId == ach)
			return &d;
	}
	for (std::vector<FormData>::iterator it2 = app->pageData->forms.begin();
			it2 != app->pageData->forms.end();
			it2++)
	{
		FormData &f = *it2;
		for (std::vector<InputData>::iterator it = f.inputs.begin();
				it != f.inputs.end();
				it++)
		{
			InputData &d = *it;
			if (d.soffidId == ach)
				return &d;
		}
	}
	return NULL;
}
