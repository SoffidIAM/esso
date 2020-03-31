#include "../MazingerHookImpl.h"
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <vector>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "../java/JavaVirtualMachine.h"
#include <DomainPasswordCheck.h>
#include <ConfigReader.h>
#include <Action.h>
#include <SecretStore.h>

#include <wchar.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <pwd.h>
#include <SeyconServer.h>

void cleanMazingerData(const char *user) {
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
	MAZINGER_DATA *pData = pEnv->getDataRW();
	if (pData != NULL)
		pData->started = 0;
	delete pEnv;
//	MZNC_destroyMutex ();
	if (stopCallback != NULL)
		stopCallback(user);
	printf("Mazinger stopped\n");
}

static char* getStopSemaphoreName (const char *user) {

	if (user == NULL)
		user = MZNC_getUserName();

	char *shmName = (char*) malloc (strlen(user)+20);
	sprintf (shmName, "MazingerStop-%s", user);
	return shmName;

}

static void * waitforstop (void * data) {
	const char*user = (const char*) data;
	char *shmName = getStopSemaphoreName(user);
	int shm = shm_open (shmName, O_RDWR|O_CREAT, 0600);
	if (shm < 0) {
		printf ("Unable to create semaphore %s\n", shmName);
	} else {
		struct passwd *pw = getpwnam(user);
		if ( pw != NULL) {
			fchown (shm, pw->pw_uid, pw->pw_gid);
		}
		ftruncate(shm, sizeof (sem_t));
		sem_t* semaphore = (sem_t*) mmap (NULL, sizeof (sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
		sem_init ( semaphore, true, 0);
		sem_wait (semaphore);
		printf("Received stop request\n");
	}
	close(shm);
	free (shmName);
	shm_unlink(shmName);
	cleanMazingerData((const char*) data);
	if (data != NULL)
		free (data);
	return 0;
}


MAZINGERAPI void MZNStart(const char *user) {
	char achFileName[1024];
	sprintf (achFileName, "%s/.config", getenv ("HOME"));
	mkdir (achFileName, 0755);
	strcat (achFileName, "/mazinger");
	mkdir (achFileName, 0700);

	SeyconCommon::debug ("Loading %s for %s", achFileName, user);
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
	PMAZINGER_DATA pData = pEnv->getDataRW();
	if (pData == NULL) {
		printf ("Unable to start mazinger\n");
		return;
	}
	pData->started = 1;
	pthread_t thread1;


	pthread_create (&thread1, NULL, waitforstop, (void*) (user == NULL? user: strdup(user)));
	// Creating .java.policy
	JavaVirtualMachine::adjustPolicy();
	ConfigReader *config = pEnv->getConfigReader();
	for (std::vector<Action*>::iterator it = config->getGlobalActions().begin(); it
			!= config->getGlobalActions().end(); it++) {
		Action*action = (*it);
		action->executeAction();
	}
}

MAZINGERAPI void MZNStop(const char *user) {
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
	if ( pEnv->getDataRW() != NULL)
		pEnv->getDataRW()->started = 0;
	char *shmName = getStopSemaphoreName(user);
	int shm = shm_open (shmName, O_RDWR, 0600);
	if (shm < 0) {
		printf ("Unable to OPEN semaphore %s\n", shmName);
	} else {
		sem_t* semaphore = (sem_t*) mmap (NULL, sizeof (sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
		sem_post (semaphore);
		close(shm);
	}
	free (shmName);
}



MAZINGERAPI bool MZNLoadConfiguration (const char *user, const wchar_t * wszFile) {
	MazingerEnv *pEnv = MazingerEnv::getEnv(user);
#ifndef WIN32
	MZNC_setUserName(user);
#endif

	PMAZINGER_DATA pData = pEnv->getDataRW();

	if (wszFile == NULL)
	{
		pData -> dwRulesSize = 0;
		pData -> achRulesFile[0] = L'\0';
		return true;
	} else {
		std::string szFile = MZNC_wstrtostr(wszFile);
		if (szFile[0] != '/') {
			std::string file2 = get_current_dir_name();
			file2.append ("/");
			file2.append (szFile);
			szFile = file2;
		}
		FILE *f = fopen (szFile.c_str(), "r");
		if (f == NULL)
		{
			printf ("Unable to open file [%s]\n", szFile.c_str());
			return false;
		}
		fseek(f, 0, SEEK_END);
		pData -> dwRulesSize = ftell (f);
		fclose (f);
		std::wstring wszFile2 = MZNC_strtowstr(szFile.c_str());
		if (wszFile2.size() * sizeof (wchar_t) >= sizeof (pData->achRulesFile))
		{
			printf ("File name [%s] too large\n", szFile.c_str());
			return false;
		}
		wcscpy (pData->achRulesFile, wszFile2.c_str());
		MZNSendDebugMessageA("TESTING configuration (v2)");
		ConfigReader *currentConfig = new ConfigReader (pData);
		currentConfig->testConfiguration();
		MZNSendDebugMessageA ("CONFIGURATION TESTED  !!!");
		return true;
	}
}


MAZINGERAPI void MZNCheckPasswords(const char *user) {

}
