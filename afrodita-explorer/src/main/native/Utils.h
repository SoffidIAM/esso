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

	static void bstr2str (std::string &str, BSTR bstr);
	static BSTR str2bstr (const char* str);

};

#endif /* UTILS_H_ */
