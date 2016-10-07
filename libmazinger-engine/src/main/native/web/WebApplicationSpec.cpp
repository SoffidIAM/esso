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
#include <strings.h>

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

bool WebApplicationSpec::matches(AbstractWebApplication &app, PageData *data) {
	if (reUrl != NULL)
	{
		if (data == NULL)
		{
			std::string v;
			app.getUrl(v);
			if (regexec (reUrl, v.c_str(), (size_t) 0, NULL, 0 ) != 0 )
			{
				MZNSendDebugMessageA("MISS  %s", szUrl);
				return false;
			}
		} else {
			if (regexec (reUrl, data->url.c_str(), (size_t) 0, NULL, 0 ) != 0 )
			{
				MZNSendDebugMessageA("MISS  %s", szUrl);
				return false;
			}
		}
	}
	if (reTitle != NULL)
	{
		if (data == NULL)
		{
			std::string v;
			app.getTitle(v);
			if (regexec (reTitle, v.c_str(), (size_t) 0, NULL, 0 ) != 0 )
			{
				MZNSendDebugMessageA("MISS  Title %s", szTitle);
				return false;
			}
		} else {
			if (regexec (reTitle, data->title.c_str(), (size_t) 0, NULL, 0 ) != 0 )
			{
				MZNSendDebugMessageA("MISS  Title %s", szTitle);
				return false;
			}
		}
	}

	for ( std::vector<WebComponentSpec*>::iterator it = m_components.begin();
			it != m_components.end();
			it++)
	{
		(*it)->dump();
		(*it)->clearMatches();
		const char *tag = (*it)->getTag();
		if (data != NULL)
		{
			bool ok = false;
			if (stricmp(tag, "form") == 0)
			{
				for (std::vector<FormData>::iterator it2 = data->forms.begin();  !ok && it2 != data->forms.end(); it2++)
				{
					if (! ok && (*it)->matches(&app, *it2))
					{
						ok = true;
					}
				}
			} else {
				for (std::vector<InputData>::iterator it2 = data->inputs.begin(); !ok && it2 != data->inputs.end(); it2++)
				{
					if (! ok && (*it)->matches(&app, *it2))
					{
						ok = true;
					}
				}
				for (std::vector<FormData>::iterator it2 = data->forms.begin();  !ok && it2 != data->forms.end(); it2++)
				{
					for (std::vector<InputData>::iterator it3 = it2->inputs.begin(); !ok && it3 != it2->inputs.end(); it3++)
					{
						if ((*it)->matches(&app, *it3))
						{
							ok = true;
						}
					}
				}
			}
			if (!ok && ! (*it) ->m_bOptional)
			{
				MZNSendDebugMessageA("MISS Component");
				(*it)->dump();
				return false;
			}
		} else {
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
				(*it2) -> release(); // Release AbstractWebElement
			}
			elements.clear();
			if (!ok && ! (*it) ->m_bOptional)
			{
				MZNSendDebugMessageA("MISS Component");
				(*it)->dump();
				return false;
			}
		}
	}
	MZNSendDebugMessageA("MATCH URL   %s",
			szUrl == NULL ? "<any>": szUrl);
	MZNSendDebugMessageA("MATCH TITLE %s",
			szTitle == NULL ? "<any>": szTitle);

	return true;
}

