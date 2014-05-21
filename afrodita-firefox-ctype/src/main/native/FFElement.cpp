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


