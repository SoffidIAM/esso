/*
 * LoginStatus.h
 *
 *  Created on: 14/02/2011
 *      Author: u07286
 */

#ifndef LOGINSTATUS_H_
#define LOGINSTATUS_H_

#include <string>

class TokenHandler;

class LoginStatus {
public:
	LoginStatus();
	virtual ~LoginStatus();

	std::wstring szOriginalUser;
	std::wstring szUser;
	std::wstring szDomain;
	HANDLE hToken;
	std::wstring szLogonTime;

	static LoginStatus current;
};

#endif /* LOGINSTATUS_H_ */
