#include "MazingerHookImpl.h"
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <vector>
#include <time.h>

#include <DomainPasswordCheck.h>
#include <ConfigReader.h>
#include <Action.h>
#include <SecretStore.h>
#include "java/JavaVirtualMachine.h"

MAZINGER_STOP_CALLBACK stopCallback;

MAZINGERAPI void MZNSetSecrets (const char *szUser, const wchar_t *szSecrets)
{
	MazingerEnv *pEnv = MazingerEnv::getEnv(szUser);
	if (pEnv->getDataRW() != NULL)
	{
		SecretStore ss(szUser);
		ss.setSecrets(szSecrets);
	}
}

MAZINGERAPI void MZNSetDebugLevel (const char *user, int debug)
{
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
	PMAZINGER_DATA pData = pEnv->getDataRW();
	if (pData != NULL)
	{
		pData->debugLevel = debug;
	}
}

MAZINGERAPI void MZNEnableSpy (const char *user, int enable)
{
	PMAZINGER_DATA pData = MazingerEnv::getEnv(user)->getDataRW();
	if (pData != NULL)
	{
		pData->spy = enable;
	}
}

MAZINGERAPI void MZNStatus (const char *user)
{
	PMAZINGER_DATA pData = MazingerEnv::getEnv(user)->getDataRW();
	if (pData != NULL && pData->started)
	{
		printf("Mazinger started\n");
		printf("Secrets list for user %s:\n", user);
		SecretStore ss(user);
		ss.dump();
	}
	else
	{
		printf("Mazinger not started\n");
	}
}

MAZINGERAPI void MZNSetStopNotifyCallback (MAZINGER_STOP_CALLBACK callback)
{
	stopCallback = callback;
}

MAZINGERAPI int MZNIsStarted (const char *user)
{
	PMAZINGER_DATA pData = MazingerEnv::getEnv(user)->getDataRW();
	if (pData != NULL)
		return pData->started;
	else
		return 0;
}
