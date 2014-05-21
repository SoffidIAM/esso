/*
 * ComponentSpec.cpp
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#include <MazingerInternal.h>
#include "ComponentSpec.h"
#include "Action.h"
#include "FailureReason.h"
#include <ComponentMatcher.h>
#include <stdlib.h>
#include <jni.h>
using namespace std;

#undef TRACE
#define TRACE NULL

ComponentSpec::ComponentSpec():
	m_actions (0),
	m_children(0)
{
	dwDialogId = 0;
	fullMatch = 0;
	m_pMatchedComponent = NULL;
	m_parent = NULL;
	szDlgId = NULL;
	szName = NULL;
	szClass = NULL;
	szTitle = NULL;
	szId = NULL;
	szText = NULL;
	reDlgId = reClass = reTitle = reText = reName = NULL;
	optional = 0;
	m_repeat = 0;
}

ComponentSpec::~ComponentSpec() {
	for (unsigned int i= 0; i <m_actions.size(); i++)
	{
		delete m_actions[i];
	}
	m_actions.clear();
	for (unsigned int i= 0; i <m_children.size(); i++)
	{
		delete m_children[i];
	}
	m_children.clear();
	setMatchedComponent(NULL);
}

void ComponentSpec::setMatchedComponent (NativeComponent *component) {
	m_repeat = 0;
	if (m_pMatchedComponent != NULL)
	{
		if (component == NULL)
			m_repeat = false;
		else
			m_repeat = component->equals(*m_pMatchedComponent);
		delete m_pMatchedComponent;
	}
	if (!m_repeat)
	{
		for (unsigned int i = 0; i < m_actions.size(); i++)
		{
			m_actions[i]->m_executed = 0;
		}
	}
	m_pMatchedComponent = component == NULL ? NULL : component->clone();
}

NativeComponent* ComponentSpec::getMatchedComponent () {
	return m_pMatchedComponent;
}

void ComponentSpec::dump ()
{
	if (szName != NULL)
		MZNSendDebugMessage("Required name=%s", szName);
	if (szClass != NULL)
		MZNSendDebugMessage("Required class=%s", szClass);
	if (szTitle != NULL)
		MZNSendDebugMessage("Required title=%s", szTitle);
	if (szText != NULL)
		MZNSendDebugMessage("Required text=%s", szText);
	if (szDlgId != NULL)
		MZNSendDebugMessage("Required dlgId=%s", szDlgId);
}

int ComponentSpec::matchAttribute (regex_t *expr, NativeComponent &native, const char*attributeName) {
	if (expr != NULL)
	{
		std::string attribute;
		native.getAttribute(attributeName, attribute);
		if (regexec (expr, attribute.c_str(), (size_t) 0, NULL, 0 ) != 0 )
		{
			return 0;
		}
	}
	return 1;
}

static void clearVector (std::vector<NativeComponent*> &v)
{
	for (vector<NativeComponent*>::iterator it = v.begin(); it != v.end(); it++)
	{
		delete *it;
	}
}

int ComponentSpec::match (NativeComponent &component, ComponentMatcher &matcher)
{
	if ( ! matchAttribute (reClass, component, "class") ||
			! matchAttribute (reText, component, "text") ||
			! matchAttribute (reName, component, "name") ||
			! matchAttribute (reDlgId, component, "dlgId") ||
			! matchAttribute (reTitle, component, "title"))
	{
		matcher.clearFailures();
		matcher.registerFailure(new FailureReason(this, &component));
		return 0;
	}

	// Recuperar los hijos
	vector<NativeComponent*> children ;
	component.getChildren(children);
	if (fullMatch)
	{
		unsigned int it;
		for (it = 0; it < m_children.size() && it < children.size(); it++)
		{
			if ( ! m_children[it]->match(*children[it], matcher) && ! m_children[it]->optional)
			{
				matcher.registerFailure(new FailureReason(this, it));
				clearVector(children);
				return 0;
			}
		}
		// Avanzar los opcionales
		while (it < m_children.size() && m_children[it]->optional)
			it++;

		if (it < m_children.size())
		{
			matcher.clearFailures();
			matcher.registerFailure(new FailureReason(m_children[it]));
			matcher.registerFailure(new FailureReason(this, it));
			clearVector(children);
			return 0;
		}
		if (it < children.size())
		{
			MZNSendDebugMessage("Failed due to unexpected component");
			matcher.clearFailures();
			matcher.registerFailure(new FailureReason(children[it]));
			matcher.registerFailure(new FailureReason(this, it));
			clearVector(children);
			return 0;
		}
	} else {
		unsigned int itNative = 0;
		unsigned int itSpec = 0;
		while (itNative < children.size() && itSpec < m_children.size())
		{
			if (m_children[itSpec]->match(*children[itNative], matcher))
			{
				itSpec ++;
				itNative ++;
			}
			else if (m_children[itSpec]->optional)
			{
				itSpec ++;
			}
			else
			{
				itNative ++;
			}
		}
		if (itSpec != m_children.size())
		{
			matcher.registerFailure(new FailureReason(m_children[itSpec]));
			matcher.registerFailure(new FailureReason(this, itSpec));
			clearVector(children);
			return 0;
		}
	}

	clearVector(children);
	matcher.notifyMatch (*this, component);

	return 1;
}
