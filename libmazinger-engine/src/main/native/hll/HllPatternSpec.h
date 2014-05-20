/*
 * HllPatternSpec.h
 *
 *  Created on: 25/03/2014
 *      Author: bubu
 */

#ifndef HLLPATTERNSPEC_H_
#define HLLPATTERNSPEC_H_

#include <vector>
#include "HllRowPatternSpec.h"
#include <HllApplication.h>
#include <Action.h>

class HllPatternSpec {
public:
	HllPatternSpec();
	virtual ~HllPatternSpec();

	std::vector<Action*> m_actions;
	std::vector<HllRowPatternSpec*> m_patterns;

	bool matches(HllApplication &app);

};

#endif /* HLLPATTERNSPEC_H_ */
