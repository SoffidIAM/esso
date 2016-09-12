/*
 * WebAddonHelper.cpp
 *
 *  Created on: 02/09/2016
 *      Author: gbuades
 */

#include "WebAddonHelper.h"
#include <MazingerInternal.h>
#include <SecretStore.h>
#include <SeyconServer.h>

#ifndef WIN32
#include <wctype.h>
#endif


WebAddonHelper::WebAddonHelper() {

}

WebAddonHelper::~WebAddonHelper() {
}

void WebAddonHelper::searchUrls(const std::wstring& filter,
		std::vector<UrlStruct>& result) {

	result.clear();
	std::string defaultSoffidSystem;

	std::wstring upperFilter;

	for (int i = 0; i < filter.length(); i++)
		upperFilter += towupper(filter[i]);

	SeyconCommon::readProperty("AutoSSOSystem", defaultSoffidSystem);

	SecretStore s (MZNC_getUserName());
	std::wstring secret = L"accdesc.";

	std::vector<std::pair<std::wstring,std::wstring> > accounts = s.getSecretsByPrefix2(secret.c_str());
	std::map<std::wstring,std::wstring> urls = s.getSecretsByPrefix(L"sso.");

	for (std::vector<std::pair<std::wstring,std::wstring> >::iterator it = accounts.begin(); it != accounts.end(); it++)
	{
		std::pair<std::wstring,std::wstring> pair = *it;
		std::wstring account = pair.first.substr(8);
		std::wstring descr = pair.second;

		std::wstring upperDesc;
		for (int i = 0; descr[i]; i++)
			upperDesc += towupper(descr[i]);

		if ( wcsstr(upperDesc.c_str(), upperFilter.c_str()) != NULL)
		{

			std::wstring secret2 = std::wstring(L"sso.")+account+L".URL";

			std::map<std::wstring,std::wstring>::iterator finder = urls.find(secret2);
			if (finder != urls.end())
			{
				UrlStruct st;
				st.description = descr;
				st.url = finder->second;

				result.push_back(st);
			}

		}
	}

}


