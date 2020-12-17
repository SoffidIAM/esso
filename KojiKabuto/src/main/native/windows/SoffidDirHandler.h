/*
 * LocalAdminHandler.h
 *
 *  Created on: 30/03/2011
 *      Author: u07286
 */

#ifndef SOFFIDDIRNHANDLER_H_
#define SOFFIDDIRHANDLER_H_

#include <string>

class SoffidDirHandler {
public:
	SoffidDirHandler();
	virtual ~SoffidDirHandler();

	std::wstring getPassword (std::wstring &szUser);

private:
	void writeLine(SOCKET socket, const std::wstring &line) ;
	std::wstring readLine(SOCKET socket);
	SOCKET connect ();
};

#endif /* LOCALADMINHANDLER_H_ */
