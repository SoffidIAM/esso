/*
 * HllMatcher.cpp
 *
 *  Created on: 26/03/2014
 *      Author: bubu
 */

#include <MazingerInternal.h>

#include <HllMatcher.h>
#include <HllApplication.h>
#include "HllRowPatternSpec.h"
#include "HllPatternSpec.h"
#include <ConfigReader.h>

#include <stdlib.h>
#include <stdio.h>

HllMatcher::HllMatcher():
	m_apps(0)
{
	m_bMatched = 0;
	m_pMatchedSpec = NULL;
	m_pApp = NULL;

}

HllMatcher::~HllMatcher() {
	m_apps.clear();
}

int HllMatcher::search(ConfigReader& reader, HllApplication& app) {
	if (MZNC_waitMutex()) {
		m_pApp = &app;
		m_apps.clear();
		for (std::vector<HllPatternSpec*>::iterator it = reader.getHllPatternSpecs().begin();
				it != reader.getHllPatternSpecs().end();
				it ++)
		{
			HllPatternSpec* pSpec = *it;
			if (pSpec->matches(app))
			{
				m_apps.push_back(pSpec);
				m_bMatched = true;
			}
		}
		MZNC_endMutex();
	}

	return m_bMatched;
}

int HllMatcher::isFound() {
	return m_bMatched;
}

void HllMatcher::triggerMatchEvent() {
	if (m_bMatched)
	{
		printf ("Triggering mathced apps\n");
		for (std::vector<HllPatternSpec*>::iterator it1 = m_apps.begin();
				it1 != m_apps.end();
				it1++)
		{
			printf ("Triggering mathced app\n");
			HllPatternSpec* pApp = *it1;
			m_pMatchedSpec = pApp;
			for (std::vector<Action*>::iterator it = pApp->m_actions.begin();
					it != pApp->m_actions.end();
					it ++)
			{
				printf ("Triggering matched action\n");
				(*it)->executeAction(*this);
			}
			m_pMatchedSpec = NULL;
		}
	}
}

void HllMatcher::dumpDiagnostic(HllApplication* app) {
}

void MZNHllMatch (HllApplication *app) {

	static ConfigReader *c = NULL;

	if (MZNC_waitMutex())
	{
		PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
		if (pMazinger != NULL && pMazinger->started)
		{
			HllMatcher m;
			if (c == NULL)
			{
				c = new ConfigReader(pMazinger);
				c->parseHll();

			}
			std::string url;
			m.search(*c, *app);
			m.triggerMatchEvent();
		}
		MZNC_endMutex();
	}
}

