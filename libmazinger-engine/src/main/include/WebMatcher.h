/*
 * ComponentMatcher.h
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#ifndef WEBMATCHER_H_
#define WEBMATCHER_H_

#include <vector>
class AbstractWebApplication;
class ConfigReader;
class WebApplicationSpec;

class WebMatcher {
public:
	WebMatcher();
	virtual ~WebMatcher();
	int search (ConfigReader &reader, AbstractWebApplication& app);
	int isFound();
	bool triggerLoadEvent () ;
	void dumpDiagnostic (AbstractWebApplication *app);
	AbstractWebApplication *getWebApp ()
		{ return m_pWebApp;};
	WebApplicationSpec *getWebAppSpec ()
		{ return m_pMatchedSpec;};

private:
	AbstractWebApplication *m_pWebApp;
	WebApplicationSpec *m_pMatchedSpec;
	std::vector<WebApplicationSpec *>m_apps;
	int m_bMatched;
};

#endif /* WEBMATCHER_H_ */
