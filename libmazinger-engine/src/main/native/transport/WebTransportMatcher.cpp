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

std::vector<WebTransport*> MZNWebTransportMatch () {
	std::vector<WebTransport*> rules;
	static ConfigReader *c = NULL;
	if (MZNC_waitMutex())
	{
		PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
		if (pMazinger != NULL && pMazinger->started)
		{
			if (c == NULL)
			{
				c = new ConfigReader(pMazinger);
				c->parseWebTransport();
				rules = c->getWebTransports();
			}
		}
		MZNC_endMutex();
	}

	return rules;
}
