/*
 * Action.cpp
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#include <stddef.h>
#include <stdlib.h>
#include <MazingerInternal.h>
#include <Action.h>
#include <DomainPasswordCheck.h>
#include <vector>
#include <string>
#include <SecretStore.h>
#include <string.h>

DomainPasswordCheck::DomainPasswordCheck() {
	m_szDomain = NULL;
	m_szServers = NULL;
	m_szUserSecret = NULL;
	m_szPasswordSecret = NULL;
}

DomainPasswordCheck::~DomainPasswordCheck() {
	if (m_szDomain != NULL)
		free (m_szDomain);
	if (m_szServers != NULL)
		free (m_szServers);
	if (m_szUserSecret != NULL)
		free (m_szUserSecret);
	if (m_szPasswordSecret != NULL)
		free (m_szPasswordSecret);
}

wchar_t *DomainPasswordCheck::toUnicode(const char* token)
{
    int len = strlen(token);
    wchar_t *wc = (wchar_t*)(malloc(len * 4));
    mbstowcs(wc, token, len*4);
    return wc;
}

void DomainPasswordCheck::executeActions(const char* event) {
	for (std::vector<Action*>::iterator it = m_actions.begin();
			it != m_actions.end();
			it ++)
	{
		Action*action = (*it);
		action->executeAction();
	}
}

