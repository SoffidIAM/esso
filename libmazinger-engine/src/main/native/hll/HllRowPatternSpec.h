/*
 * HllRowPatternSpec.h
 *
 *  Created on: 25/03/2014
 *      Author: bubu
 */

#ifndef HLLROWPATTERNSPEC_H_
#define HLLROWPATTERNSPEC_H_

#include <pcreposix.h>

class HllRowPatternSpec {
public:
	HllRowPatternSpec();
	virtual ~HllRowPatternSpec();
	char * row;
	char * pattern;
	regex_t *rePattern;
};

#endif /* HLLROWPATTERNSPEC_H_ */
