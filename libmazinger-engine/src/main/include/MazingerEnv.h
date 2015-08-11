/*
 * MazingerEnv.h
 *
 *  Created on: 30/05/2011
 *      Author: u07286
 */

#ifndef MAZINGERENV_H_
#define MAZINGERENV_H_

#include <string>
#include <vector>

#ifdef WIN32
#include <windows.h>
#endif

class ConfigReader;

#define SECRET_MAX_SIZE 256
#define SECRETS_BUFFER_SIZE 128000

typedef
struct MazingerDataStruct {
	unsigned int dwRulesSize;
	unsigned int started;
	long lastUpdate;
	wchar_t achSecrets[SECRETS_BUFFER_SIZE];
	int debugLevel;
	int spy;
	wchar_t achRulesFile[SECRET_MAX_SIZE];
} MAZINGER_DATA, * PMAZINGER_DATA;



class MazingerEnv {
public:
	MazingerEnv();
	virtual ~MazingerEnv();
	static MazingerEnv* getDefaulEnv();
	static MazingerEnv* getEnv(const char *user);
	static MazingerEnv* getEnv(const char *user, const char*desktop);
	const PMAZINGER_DATA getData ();
	PMAZINGER_DATA getDataRW ();
	ConfigReader* getConfigReader () ;

private:
	static MazingerEnv *pDefaultEnv;
	static std::vector<MazingerEnv*> environments;
	std::string user;
	std::string desktop;
	bool openReadOnly;
	PMAZINGER_DATA open (bool readOnly);
	void close ();
	ConfigReader *reader;
#ifdef WIN32
	HANDLE hMapFile;
	BOOL hMapFileRW;
	PMAZINGER_DATA createMazingerData() ;

#else
	std::string shmName;
	int shm;
#endif
	PMAZINGER_DATA pMazingerData;
public:
	const char *getUser() { return user.c_str(); }
};

#endif /* MAZINGERENV_H_ */
