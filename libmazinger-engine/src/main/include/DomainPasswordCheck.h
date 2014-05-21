/*
 * Action.h
 *
 *  Created on: 16/02/2010
 *      Author: u07286
 */

#ifndef DOMAIN_PASSWORD_H_
#define DOMAIN_PASSWORD_H_

#include <vector>

class Action;

class DomainPasswordCheck {
public:
	DomainPasswordCheck();
	virtual ~DomainPasswordCheck();
	char *m_szDomain;
	char *m_szServers;
	char *m_szUserSecret;
	char *m_szPasswordSecret;
	std::vector<Action*> m_actions;

    void executeActions(const char* event);
    wchar_t *toUnicode(const char * token);
};

#endif /* DOMAIN_PASSWORD_H_ */
