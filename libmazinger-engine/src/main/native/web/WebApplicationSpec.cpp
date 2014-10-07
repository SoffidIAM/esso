/*
 * CWebApplicationSpec.cpp
 *
 *  Created on: 18/10/2010
 *      Author: u07286
 */

#include "WebApplicationSpec.h"
#include <Action.h>
#include <string>
#include <vector>
#include <AbstractWebApplication.h>
#include <AbstractWebElement.h>
#include "WebComponentSpec.h"
#include <pcreposix.h>
#include <MazingerInternal.h>

WebApplicationSpec::WebApplicationSpec(): m_actions(0) {
	szUrl = NULL;
	szTitle = NULL;
	szContent = NULL;
	reUrl = reTitle = reContent = NULL;
}

WebApplicationSpec::~WebApplicationSpec() {
	for (unsigned int i= 0; i <m_actions.size(); i++)
	{
		delete m_actions[i];
	}
}

bool WebApplicationSpec::matches(AbstractWebApplication &app) {
	MZNSendTraceMessageA("Testing web application %s",
			szUrl);
	if (reUrl != NULL)
	{
		std::string v;
		app.getUrl(v);
		if (regexec (reUrl, v.c_str(), (size_t) 0, NULL, 0 ) != 0 )
		{
			MZNSendTraceMessageA("Web application %s does not match %s",
					szUrl, v.c_str());
			return false;
		}
	}
	if (reTitle != NULL)
	{
		std::string v;
		app.getTitle(v);
		if (regexec (reTitle, v.c_str(), (size_t) 0, NULL, 0 ) != 0 )
		{
			MZNSendTraceMessageA("Web application title %s does not match title %s",
					szTitle, v.c_str());
			return false;
		}
	}

	for ( std::vector<WebComponentSpec*>::iterator it = m_components.begin();
			it != m_components.end();
			it++)
	{
		(*it)->clearMatches();
		const char *tag = (*it)->getTag();
		std::vector<AbstractWebElement*> elements;
		app.getElementsByTagName(tag, elements);
		bool ok = false;
		for (std::vector<AbstractWebElement*>::iterator it2 = elements.begin();
				it2 != elements.end();
				it2++)
		{
			if (! ok && (*it)->matches(*it2))
			{
				ok = true;
			}
			delete (*it2); // Release AbstractWebElement
		}
		elements.clear();
		if (!ok && ! (*it) ->m_bOptional)
		{
			MZNSendDebugMessageA("Missing required component: ");
			(*it)->dump();
			return false;
		}
	}
	MZNSendDebugMessageA("Matched web application. url=%s title=%s",
			szUrl == NULL ? "<any>" : szUrl,
			szTitle == NULL ? "<any>": szTitle);

	return true;
}

