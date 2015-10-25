/*
 * ExplorerElement.cpp
 *
 *  Created on: 11/11/2010
 *      Author: u07286
 */
#define NS_SCRIPTABLE


#include "AfroditaF.h"
#include "FFElement.h"
#include <stdio.h>
#include "EventHandler.h"

FFElement::FFElement(long docId, long elementId)
{
	this->docId = docId;
	this->elementId = elementId;
}

FFElement::~FFElement() {
}



void FFElement::getChildren(std::vector<AbstractWebElement*> &children)
{
	children.clear();
	if (AfroditaHandler::handler.getChildrenHandler != NULL) {
		const long* id = AfroditaHandler::handler.getChildrenHandler (docId, elementId);
		for (int i = 0; id != NULL && id[i] != 0; i++) {
			FFElement *element = new FFElement(docId, id[i]);
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
			return new FFElement (docId, id);
	}
	return NULL;
}

AbstractWebElement *FFElement::getOffsetParent()
{
	if ( AfroditaHandler::handler.getOffsetParentHandler != NULL)
	{
		long id = AfroditaHandler::handler.getOffsetParentHandler(docId, elementId);
		if (id != 0)
			return new FFElement (docId, id);
	}
	return NULL;
}



void FFElement::setAttribute(const char *attribute, const char*value)
{
	if ( AfroditaHandler::handler.setAttributeHandler != NULL)
	{
		AfroditaHandler::handler.setAttributeHandler(docId, elementId, attribute, value);
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
	return new FFElement(docId, elementId);
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
			return new FFElement (docId, id);
	}
	return NULL;
}

AbstractWebElement* FFElement::getNextSibling() {
	if ( AfroditaHandler::handler.getNextSiblingHandler != NULL)
	{
		long id = AfroditaHandler::handler.getNextSiblingHandler(docId, elementId);
		if (id != 0)
			return new FFElement (docId, id);
	}
	return NULL;
}

AbstractWebApplication* FFElement::getApplication() {
	return new FFWebApplication (docId);
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
	sprintf (ach, "FFElement %d/%d ", docId, elementId);
	std::string result = ach;
	std::string tag;
	getTagName(tag);
	result += tag;
	return result;
}
