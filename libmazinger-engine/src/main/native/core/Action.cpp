/*
 * Action.cpp
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#include <stddef.h>
#include <stdlib.h>
#include <MazingerInternal.h>
#include <WebMatcher.h>
#include "Action.h"
#include "ComponentMatcher.h"
#include "ComponentSpec.h"
#include <string.h>
#include <time.h>
#include <HllMatcher.h>

Action::Action() {
	szContent = NULL;
	szText = NULL;
	szType = NULL;
	m_component = NULL;
	m_canRepeat = 0;
	m_executed = 0;
	m_delay = -1;
	m_lastExecution = 0;
}

Action::~Action() {
	if (szContent != NULL)
		free (szContent);
	if (szText != NULL)
		free (szText);
	if (szType != NULL)
		free (szType);
}

bool Action::canExecute ()
{
	time_t now;
	time(&now);
	if (!m_executed ||
		(m_canRepeat && m_delay <= 0 ) ||
		(m_canRepeat && (now - m_lastExecution) >= m_delay) )
	{
		m_lastExecution = now;
		m_executed = true;
		return true;
	}
	else
		return false;
}

void Action::executeAction (ComponentMatcher& matcher) {
	if (m_component != NULL && m_component->m_pMatchedComponent != NULL && canExecute())
	{
		if (szType != NULL && strcmp (szType, "setText") == 0)
		{
			MZNSendDebugMessageA("Execution action: setText %s", szText);
			m_component->m_pMatchedComponent->setAttribute("text", szText);
		} else if ((szType == NULL || strcmp (szType, "script") == 0) && szContent != NULL)
		{
			MZNSendDebugMessageA("Execution ECMAscript\n%s", szContent);
			MZNEvaluateJSMatch (matcher, szContent);
		} else {
			MZNSendDebugMessageA("Ignoring action type %s", szType);
		}
	}
}

void Action::executeAction (HllMatcher& matcher) {
	if (canExecute())
	{
		if (szContent != NULL)
		{
			MZNSendDebugMessageA("Execution ECMAscript\n%s", szContent);
			MZNEvaluateJSMatch (matcher, szContent);
		} else {
			MZNSendDebugMessageA("Ignoring action type %s", szType);
		}
	}
}
void Action::executeAction () {
	if ((szType == NULL || strcmp (szType, "script") == 0) && szContent != NULL)
	{
		MZNSendDebugMessageA("Execution action: script %s", szContent);
		MZNEvaluateJS (szContent);
	} else {
		MZNSendDebugMessageA("Ignoring action type %s", szType);
	}
}

void Action::executeAction (WebMatcher& matcher) {
	if (canExecute()) {
		if ((szType == NULL || strcmp (szType, "script") == 0) && szContent != NULL)
		{
			MZNSendDebugMessageA("Execution ECMAscript\n%s", szContent);
			MZNEvaluateJSMatch (matcher, szContent);
		} else {
			MZNSendDebugMessageA("Ignoring action type %s", szType);
		}
	}
}


