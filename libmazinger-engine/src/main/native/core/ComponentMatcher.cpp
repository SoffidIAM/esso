/*
 * ComponentMatcher.cpp
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#include <MazingerInternal.h>
#include <ComponentMatcher.h>
#include "ComponentSpec.h"
#include "Action.h"
#include "FailureReason.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>

ComponentMatcher::ComponentMatcher():
	m_aliasedComponents(0)
{
	m_bMatched = 0;
	m_component = NULL;
	m_focus = NULL;
	m_nativeFocus = NULL;
}

ComponentMatcher * ComponentMatcher::clone() {
	ComponentMatcher *m = new ComponentMatcher();
	m->m_aliasedComponents.assign(m_aliasedComponents.begin(), m_aliasedComponents.end());
	m->m_bMatched = m_bMatched;
	m->m_component = m_component;
	m->m_focus = m_focus;
	m->m_nativeFocus = m_nativeFocus;
	m->m_failedComponents.assign(m_failedComponents.begin(), m_failedComponents.end());
	return m;
}

ComponentMatcher::~ComponentMatcher() {
	clearFailures();
}

int ComponentMatcher::isFound() {
	return m_bMatched;
}

ComponentSpec *ComponentMatcher::getFocusComponent() {
	return m_focus;
}


void ComponentMatcher::registerFailure(FailureReason *reason)
{
	m_failedComponents.push_back(reason);
}

void ComponentMatcher::dumpDiagnostic (NativeComponent *top, NativeComponent *focus)
{
	PMAZINGER_DATA data = MazingerEnv::getDefaulEnv()->getData();
	if (data != NULL && data->spy)
	{
		const char* szCmdLine = MZNC_getCommandLine();
		MZNSendSpyMessage("<!----------------------------------------------------------------->");
		MZNSendSpyMessage("<Application cmdLine='^%s$'>",
				xmlEncode(szCmdLine));
		top->dumpXML(2, focus);
		MZNSendSpyMessage("</Application>",
				xmlEncode(szCmdLine));
	}
}


int ComponentMatcher::search (ConfigReader &reader, NativeComponent& focus) {

	m_bMatched = false;
	if (MZNC_waitMutex())
	{

		NativeComponent *lastparent = &focus;
		NativeComponent *parent = lastparent->getParent();
		std::string s;

		m_nativeFocus = &focus;

		m_bMatched = 0;
		while (parent != NULL) {
			lastparent = parent;
			parent = lastparent->getParent();
		}
		//TRACE;
		PMAZINGER_DATA data = MazingerEnv::getDefaulEnv()->getData();
		if (data != NULL && data->spy)
		{
			dumpDiagnostic(lastparent, m_nativeFocus);
		}
		//TRACE;
		int ruleNumber = 0;
		if (! reader.getComponents().empty() )
			MZNSendTraceMessage("Evaluating rules for: [%s]", MZNC_getCommandLine());
		for (std::vector<ComponentSpec*>::iterator it = reader.getComponents().begin();
				!m_bMatched &&
				it != reader.getComponents().end();
				it ++)
		{
			m_aliasedComponents.clear();
			m_focus = NULL;
			if ( (*it) -> match(*lastparent, *this))
			{
				m_bMatched = 1;
				MZNSendTraceMessage("MATCHED Application rule: [%d]", ruleNumber);
			} else {
				if (data != NULL &&
						data->debugLevel > 1 && m_failedComponents.size()>0)
				{
					char achBuffer[50];
					std::string path ;
					path.clear();
					sprintf (achBuffer, "[%d]", ruleNumber);
					path.append(achBuffer);
					for (int i = m_failedComponents.size()-1; i > 0 ; i--)
					{
						sprintf (achBuffer, "/[%d]", m_failedComponents.at(i)->m_order);
						path.append(achBuffer);
					}
					MZNSendDebugMessage("NOT MATCHED. Application rule: %s", path.c_str());
					m_failedComponents[0]->notify();
				}
				ruleNumber ++;
			}
		}
		MZNC_endMutex ();
	}

	return m_bMatched;
}


void ComponentMatcher::triggerFocusEvent () {
	if (m_bMatched)
	{
		ComponentSpec *current = m_focus;
		while (current != NULL)
		{
			for (std::vector<Action*>::iterator it = current->m_actions.begin();
					it != current->m_actions.end();
					it ++)
			{
				(*it)->executeAction(*this);
			}
			current = current->m_parent;
		}
	}
}

std::vector<ComponentSpec*> &ComponentMatcher::getAliasedComponents() {
	return m_aliasedComponents;
}



void ComponentMatcher::notifyMatch (ComponentSpec &spec, NativeComponent &native) {
	spec.setMatchedComponent ( &native );
	if (m_nativeFocus != NULL && native.equals (*m_nativeFocus))
		m_focus = &spec;
	if (spec.szId != NULL)
	{
//		TRACE;
		m_aliasedComponents.push_back(&spec);
	}
}

void ComponentMatcher::clearFailures() {
	for (std::vector<FailureReason*>::iterator it = m_failedComponents.begin();
			it != m_failedComponents.end();
			it ++)
	{
		FailureReason* reason = *it;
		delete reason;
	}
	m_failedComponents.clear();
}
