/*
 * WebAddonHelper.h
 *
 *  Created on: 02/09/2016
 *      Author: gbuades
 */

#ifndef WEBADDONHELPER_H_
#define WEBADDONHELPER_H_

#include <string>
#include <vector>

struct UrlStruct {
public:
	std::wstring description;
	std::wstring url;
};

class WebAddonHelper {
public:
	WebAddonHelper();
	virtual ~WebAddonHelper();
	void searchUrls (const std::wstring &filter,
			std::vector<UrlStruct> &result);
};

#endif /* WEBADDONHELPER_H_ */
