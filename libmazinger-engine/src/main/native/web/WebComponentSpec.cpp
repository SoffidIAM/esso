/*
 * WebComponentSpec.cpp
 *
 *  Created on: 15/11/2010
 *      Author: u07286
 */

#include <stddef.h>
#include <string>
#include "WebComponentSpec.h"
#include <AbstractWebElement.h>
#include <MazingerInternal.h>

WebComponentSpec::WebComponentSpec() {
	reId = NULL;
	reName = NULL;
	szId = NULL;
	szName = NULL;
	szRefAs = NULL;
	m_pElement = NULL;
}

WebComponentSpec::~WebComponentSpec() {
	if (m_pElement != NULL)
		m_pElement -> release();
}


bool WebComponentSpec::matches (AbstractWebElement *element) {
	if ( ! matchAttribute (reId, element, "id") ||
			! matchAttribute (reName, element, "name") )
	{
		clearMatches();
		return false;
	}
	else
	{
		setMatched(element);
		return true;
	}
}


bool WebComponentSpec::matchAttribute (regex_t *expr, AbstractWebElement *element, const char*attributeName) {
	if (expr != NULL)
	{
		std::string value;
		element->getAttribute(attributeName, value);
//		MZNSendTraceMessageA("Checking for attributes %s value =[%s]", attributeName, value.c_str());
		if (regexec (expr, value.c_str(), (size_t) 0, NULL, 0 ) != 0 )
		{
			return false;
		}
	}
	return true;
}


void WebComponentSpec::setMatched(AbstractWebElement *element)
{
	m_pElement = element->clone();
}



void WebComponentSpec::clearMatches()
{
	if (m_pElement != NULL)
		m_pElement -> release();
	m_pElement = NULL;
}

