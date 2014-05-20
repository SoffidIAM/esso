/*
 * Utils.h
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <string>

class Utils {
public:
	Utils();
	virtual ~Utils();

	static bool duplicateString (char* &szTarget, const char* sz);
	static void freeString (char* &wsz);

	static bool changePassword (char* achUser, char*achPass, char *achNewPassword,
			std::string &szStatusText);
};

#endif /* UTILS_H_ */
