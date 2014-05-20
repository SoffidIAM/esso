/*
 * CWebApplicationSpec.h
 *
 *  Created on: 18/10/2010
 *      Author: u07286
 */

#ifndef CWEBAPPLICATIONSPEC_H_
#define CWEBAPPLICATIONSPEC_H_


#include <pcreposix.h>
#include <vector>

class Action;
class AbstractWebApplication;
class WebComponentSpec;

class WebApplicationSpec {
public:
	WebApplicationSpec();
	virtual ~WebApplicationSpec();
	bool matches(AbstractWebApplication &app);
	regex_t *reUrl;
	regex_t *reTitle;
	regex_t *reContent;
	char *szUrl;
	char *szContent;
	char *szTitle;
	std::vector<Action*> m_actions;
	std::vector<WebComponentSpec*> m_components;
};

#endif /* CWEBAPPLICATIONSPEC_H_ */
