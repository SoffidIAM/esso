/*
 * WebFormSpec.cpp
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#include "WebInputSpec.h"
#include "WebFormSpec.h"
#include "AbstractWebElement.h"
#include <string.h>
#include <MazingerInternal.h>

WebFormSpec::WebFormSpec() {
	reAction = NULL;
	reMethod = NULL;
	szAction = NULL;
	szMethod = NULL;
}

WebFormSpec::~WebFormSpec() {
}


bool WebFormSpec::matches (AbstractWebElement *element) {
	if ( ! matchAttribute (reAction, element, "action") ||
			! matchAttribute (reMethod, element, "method") )
	{
		return false;
	}
	if (!WebComponentSpec::matches(element))
	{
		return false;
	}
	if (m_children.size() == 0)
	{
		return true;
	}


	for (std::vector<WebComponentSpec*>::iterator it = m_children.begin();
			it != m_children.end();
			it++)
	{
		(*it)->clearMatches();
	}
	findChildren (element);
	bool ok = true;
	for (std::vector<WebComponentSpec*>::iterator it = m_children.begin();
			it != m_children.end();
			it++)
	{
		if ((*it)->getMatched() == NULL && ! (*it)->m_bOptional)
		{
			std::string name;
			element->getAttribute("name", name);
			MZNSendDebugMessageA("Form %s: missing element",
					name.c_str());
			ok = false;
		}
	}
	if (!ok)
	{
		for (std::vector<WebComponentSpec*>::iterator it = m_children.begin();
				it != m_children.end();
				it++)
		{
			(*it)->clearMatches();
		}
		clearMatches();
		return false;
	}
	else
		return true;
}




void WebFormSpec::findChildren (AbstractWebElement *childElement)
{
	// Look for current element
	std::string tag;
	childElement->getTagName(tag);
	if (stricmp (tag.c_str(), "input") == 0)
	{
		bool found = false;
		for (std::vector<WebComponentSpec*>::iterator it = m_children.begin();
				!found && it != m_children.end();
				it++)
		{
			if ( (*it) ->getMatched() == NULL ) {
				if ( (*it) ->matches(childElement))
					found = true;
			}

		}
	}

	// Look for childrens
	std::vector<AbstractWebElement*>children;
	childElement->getChildren(children);
	for (std::vector<AbstractWebElement*>::iterator it = children.begin();
			it != children.end();
			it++)
	{
		findChildren ( *it);
		delete (*it);
	}

}


const char* WebFormSpec::getTag() {
	static const char* achTag = "form";
	return achTag;
}

void WebFormSpec::dump() {
	MZNSendDebugMessageA("Form name='%s' action='%s' method='%s' id='%s' ref-as='%s'",
			szName == NULL? "": szName,
			szAction == NULL? "": szAction,
			szMethod == NULL? "": szMethod,
			szId == NULL? "": szId,
			szRefAs == NULL? "": szRefAs);
}
