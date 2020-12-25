/*
 * ComponentMatcher.cpp
 *
 *  Created on: 15/02/2010
 *      Author: u07286
 */

#include <MazingerInternal.h>
#include <ConfigReader.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <WebTransport.h>

#ifdef WIN32
static DWORD dwTlsIndex = TLS_OUT_OF_INDEXES;
#endif

std::vector<WebTransport*> MZNWebTransportMatch () {
	std::vector<WebTransport*> rules;
	static ConfigReader *c = NULL;
	if (MZNC_waitMutex())
	{
#ifdef WIN32
		if (dwTlsIndex == TLS_OUT_OF_INDEXES)
		{
			if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
				return rules;
		}
		if (TlsGetValue (dwTlsIndex) != NULL)
		{
			MZNC_endMutex();
			return rules;
		}
		TlsSetValue (dwTlsIndex, (LPVOID) 1);
#endif
		PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
		if (pMazinger != NULL && pMazinger->started)
		{
			if (c == NULL)
			{
				c = new ConfigReader(pMazinger);
				c->parseWebTransport();
				rules = c->getWebTransports();
			}
		} else {
			MZNC_endMutex();
		}
#ifdef WIN32
		TlsSetValue (dwTlsIndex, NULL);
#endif
	}

	return rules;
}
