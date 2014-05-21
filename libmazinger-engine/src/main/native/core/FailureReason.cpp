/*
 * FailureReason.cpp
 *
 *  Created on: 24/02/2010
 *      Author: u07286
 */

#include <MazingerInternal.h>
#include "FailureReason.h"
#include "NativeComponent.h"
#include "ComponentSpec.h"

FailureReason::FailureReason(ComponentSpec *spec, NativeComponent *native) {
	m_pComponent = native->clone();
	m_pComponentSpec = spec;
	m_order = -1;
}

FailureReason::FailureReason(ComponentSpec *spec) {
	m_pComponent = NULL;
	m_pComponentSpec = spec;
	m_order = -1;
}

FailureReason::FailureReason(NativeComponent *native) {
	m_pComponent = native->clone();
	m_pComponentSpec = NULL;
	m_order = -1;
}


FailureReason::FailureReason(ComponentSpec *spec, int order) {
	m_pComponent = NULL;
	m_pComponentSpec = spec;
	m_order = order;
}


FailureReason::~FailureReason() {
	if (m_pComponent != NULL)
		delete (m_pComponent);
}

void FailureReason::notifyAttribute (const char *attribute, regex_t* regExp, const char* regExpStr) {
	std::string value;
	m_pComponent->getAttribute(attribute, value);
	if (! m_pComponentSpec->matchAttribute (regExp, *m_pComponent, attribute))
	{
		MZNSendDebugMessage ("%s=[%s] does not match regular expression [%s]",
				attribute, MZNC_utf8tostr(value.c_str()).c_str(), MZNC_utf8tostr(regExpStr).c_str() );
	}
	else if (value.length() > 0)
	{
		MZNSendDebugMessage ("%s=[%s] (MATCH)", attribute, value.c_str());
	}
}

void FailureReason::notify () {
	if (m_pComponentSpec == NULL && m_pComponent != NULL)
	{
		MZNSendDebugMessage("Unexpected component:");
		m_pComponent->dump();
	}
	if (m_pComponentSpec != NULL && m_pComponent == NULL)
	{
		MZNSendDebugMessage("Missing component:");
		m_pComponentSpec->dump();
	}
	if (m_pComponentSpec != NULL && m_pComponent != NULL)
	{
		notifyAttribute ("class", m_pComponentSpec->reClass, m_pComponentSpec->szClass);
		notifyAttribute ("title", m_pComponentSpec->reTitle, m_pComponentSpec->szTitle);
		notifyAttribute ("name", m_pComponentSpec->reName, m_pComponentSpec->szName);
		notifyAttribute ("text", m_pComponentSpec->reText, m_pComponentSpec->szText);
		notifyAttribute ("dlgId", m_pComponentSpec->reDlgId, m_pComponentSpec->szDlgId);
	}
}

