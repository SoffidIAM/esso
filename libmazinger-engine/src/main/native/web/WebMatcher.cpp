/*
 * ComponentMatcher.cpp
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#include <MazingerInternal.h>
#include "WebMatcher.h"
#include "WebApplicationSpec.h"
#include <AbstractWebApplication.h>
#include <Action.h>
#include <ConfigReader.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <SmartWebPage.h>

WebMatcher::WebMatcher():
	m_apps(0)
{
	m_bMatched = 0;
	m_pMatchedSpec = NULL;
	m_pWebApp = NULL;
}


WebMatcher::~WebMatcher() {
	m_apps.clear();
}

int WebMatcher::isFound() {
	return m_bMatched;
}


int WebMatcher::search (ConfigReader &reader, AbstractWebApplication& focus) {

	if (MZNC_waitMutex()) {
		m_pWebApp = &focus;
		m_apps.clear();
		for (std::vector<WebApplicationSpec*>::iterator it = reader.getWebApplicationSpecs().begin();
				it != reader.getWebApplicationSpecs().end();
				it ++)
		{
			WebApplicationSpec* pSpec = *it;
			if (pSpec->matches(focus))
			{
				m_apps.push_back(pSpec);
				m_bMatched = true;
			}
		}
		MZNC_endMutex();
	}

	return m_bMatched;
}


void WebMatcher::triggerLoadEvent () {
	if (m_bMatched)
	{
		for (std::vector<WebApplicationSpec*>::iterator it1 = m_apps.begin();
				it1 != m_apps.end();
				it1++)
		{
			WebApplicationSpec* pApp = *it1;
			m_pMatchedSpec = pApp;
			for (std::vector<Action*>::iterator it = pApp->m_actions.begin();
					it != pApp->m_actions.end();
					it ++)
			{
				(*it)->executeAction(*this);
			}
			m_pMatchedSpec = NULL;
		}
	}
}


void MZNWebMatch (AbstractWebApplication *app) {

	static ConfigReader *c = NULL;

	if (MZNC_waitMutex())
	{
		PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
		if (pMazinger != NULL && pMazinger->started)
		{
			WebMatcher m;
			if (c == NULL)
			{
				c = new ConfigReader(pMazinger);
				c->parseWeb();

			}
			std::string url;
			app->getUrl(url);
			MZNSendDebugMessageA("Looking match for %s", url.c_str());
			m.search(*c, *app);
			if (m.isFound())
				m.triggerLoadEvent();
			else
			{
				MZNSendDebugMessageA("Running default handler for %s", url.c_str());
				SmartWebPage *page = app->getWebPage();
				if (page != NULL)
					page->parse(app);
			}

		}
		MZNC_endMutex();
	}
}

