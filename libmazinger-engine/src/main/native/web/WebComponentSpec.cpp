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
#include <AbstractWebApplication.h>
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
		return matchValue(expr, value.c_str());
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

bool WebComponentSpec::matches(AbstractWebApplication *app, FormData& form) {
	if ( ! matchValue (reId, form.id.c_str()) ||
			! matchValue (reName, form.name.c_str()) )
	{
		clearMatches();
		return false;
	}
	else
	{
		AbstractWebElement *pElement = app->getElementBySoffidId(form.soffidId.c_str());
		setMatched(pElement);
		pElement->release();
		return true;
	}
}

bool WebComponentSpec::matches(AbstractWebApplication *app, InputData& input) {
	if ( ! matchValue (reId, input.id.c_str()) ||
			! matchValue (reName, input.name.c_str()) )
	{
		clearMatches();
		return false;
	}
	else
	{
		AbstractWebElement *pElement = app->getElementBySoffidId(input.soffidId.c_str());
		setMatched(pElement);
		pElement->release();
		return true;
	}
}

bool WebComponentSpec::matchValue(regex_t* expr, const char* value) {
	if (expr == NULL)
		return true;
	else if (regexec (expr, value, (size_t) 0, NULL, 0 ) != 0 )
	{
		return false;
	}
	else
	{
		return true;
	}
}
