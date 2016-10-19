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
	PageData *data = NULL;
	if (focus.supportsPageData())
		data = focus.getPageData();

	m_pWebApp = &focus;
	m_apps.clear();
	for (std::vector<WebApplicationSpec*>::iterator it = reader.getWebApplicationSpecs().begin();
			it != reader.getWebApplicationSpecs().end();
			it ++)
	{
		WebApplicationSpec* pSpec = *it;
		if (pSpec->matches(focus, data))
		{
			m_apps.push_back(pSpec);
			m_bMatched = true;
		}
	}

	return m_bMatched;
}


void WebMatcher::triggerLoadEvent () {
	if (m_bMatched)
	{
//		MZNSendTraceMessageA("Searching on matched apps");
		for (std::vector<WebApplicationSpec*>::iterator it1 = m_apps.begin();
				it1 != m_apps.end();
				it1++)
		{
			WebApplicationSpec* pApp = *it1;
			m_pMatchedSpec = pApp;
			std::string url;
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

#ifdef WIN32
static DWORD dwTlsIndex = TLS_OUT_OF_INDEXES;
#endif

void MZNWebMatch (AbstractWebApplication *app) {

	static ConfigReader *c = NULL;
	static bool recursive = false;
//	MZNSendTraceMessageA("Waiting for mutex");
	if (MZNC_waitMutex())
	{
#ifdef WIN32
		if (dwTlsIndex == dwTlsIndex)
		{
			if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
				return;
		}
		if (TlsGetValue (dwTlsIndex) != NULL)
		{
			MZNC_endMutex();
			return;
		}
		TlsSetValue (dwTlsIndex, (LPVOID) 1);
#endif
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
			PageData *data = app->getPageData();
			if (data != NULL)
				data->dump();
			else
				MZNSendDebugMessageA("PAGE  %s", url.c_str());
			MZNSendDebugMessageA("      ================================================================");
			m.search(*c, *app);
			MZNSendDebugMessageA("      ================================================================");
			MZNC_endMutex();
			if (m.isFound())
			{
//					MZNSendTraceMessageA("Executing matched triggers");
				m.triggerLoadEvent();
			}
			else
			{
//					MZNSendTraceMessageA("Executing auto login mechanism");
				SmartWebPage *page = app->getWebPage();
				if (page != NULL)
				{
					page->fetchAccounts(app, NULL);
					page->parse(app);
				}
			}
			MZNSendDebugMessageA("DONE  ================================================================");
		} else {
			MZNC_endMutex();
		}
#ifdef WIN32
		TlsSetValue (dwTlsIndex, NULL);
#endif
	}
}

void MZNWebMatchRefresh (AbstractWebApplication *app) {

	static ConfigReader *c = NULL;
	static bool recursive = false;
	if (MZNC_waitMutex())
	{
		if (! recursive)
		{
			recursive = true;
			PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
			if (pMazinger != NULL && pMazinger->started)
			{
				SmartWebPage *page = app->getWebPage();
				if (page != NULL)
				{
					PageData *data = app->getPageData();
					if (data != NULL)
						data->dump();
					page->fetchAccounts(app, NULL);
					page->parse(app);
				}
			}
			recursive = false;
		}
		MZNC_endMutex();
	}
}

