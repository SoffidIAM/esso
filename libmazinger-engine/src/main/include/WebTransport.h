/*
 * HllApplication.h
 *
 *  Created on: 26/03/2014
 *      Author: bubu
 */

#ifndef ABSTACT_WEBTRANSPORT_H_
#define ABSTACT_WEBTRANSPORT_H_

#include <string>

class WebTransport {
public:
	WebTransport();
	~WebTransport();

	std::wstring url;
	std::wstring system;
	std::wstring domain;
};

#endif /* ABSTACT_HLLAPPLICATION_H_ */
