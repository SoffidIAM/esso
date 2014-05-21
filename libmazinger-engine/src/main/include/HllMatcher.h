/*
 * HllMatcher.h
 *
 *  Created on: 26/03/2014
 *      Author: bubu
 */

#ifndef HLLMATCHER_H_
#define HLLMATCHER_H_

#include <vector>

class ConfigReader;
class HllApplication;
class HllPatternSpec;

class HllMatcher {
public:
	HllMatcher();
	virtual ~HllMatcher();

	int search (ConfigReader &reader, HllApplication& app);
	int isFound();
	void triggerMatchEvent () ;
	void dumpDiagnostic (HllApplication *app);
	HllApplication *getHllApplication ()
		{ return m_pApp;};
	HllPatternSpec *getPatternSpec ()
		{ return m_pMatchedSpec;};

private:
	HllApplication *m_pApp;
	HllPatternSpec *m_pMatchedSpec;
	std::vector<HllPatternSpec *>m_apps;
	int m_bMatched;
};

#endif /* HLLMATCHER_H_ */
