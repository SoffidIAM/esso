/*
 * MazingerEnv.cpp
 *
 *  Created on: 30/05/2011
 *      Author: u07286
 */

#ifdef WIN32
#include <windows.h>
#include <sddl.h>
#include <psapi.h>
#include <accctrl.h>
#include <aclapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#endif

#include <MazingerEnv.h>
#include <MZNcompat.h>
#include <ConfigReader.h>
#include <stdio.h>
#include <string.h>
#include <MazingerInternal.h>
#include <SeyconServer.h>

MazingerEnv::MazingerEnv() {
#ifdef WIN32
	hMapFile = NULL;
	hMapFileRW = FALSE;
#else
	pMazingerData = NULL;
	shm = -1;
#endif
	openReadOnly = true;
	reader = NULL;

}

MazingerEnv::~MazingerEnv() {
	close ();
	if (pDefaultEnv == this)
		pDefaultEnv = NULL;

	for (std::vector<MazingerEnv*>::iterator it = environments.begin ();
			it != environments.end(); it++)
	{
		MazingerEnv* env = *it;
		if (env == this) {
			environments.erase(it);
			return;
		}
	}
}

MazingerEnv *MazingerEnv::pDefaultEnv = NULL;
std::vector<MazingerEnv*> MazingerEnv::environments;


static char *defaultDesktop = NULL;

static char *getDefaultDesktop ()
{
	if (defaultDesktop == NULL)
	{
#ifdef WIN32
		HANDLE hd = GetThreadDesktop (GetCurrentThreadId());
		char desktopName[1024];

		strcpy (desktopName, "Default");

		GetUserObjectInformation (hd,
			UOI_NAME,
			desktopName, sizeof desktopName,
			NULL);
		defaultDesktop = strdup (desktopName);
#else
		defaultDesktop = "Default";
#endif
	}
	return defaultDesktop;
}

MazingerEnv* MazingerEnv::getDefaulEnv() {
	if (pDefaultEnv == NULL) {
		pDefaultEnv = new MazingerEnv;
		pDefaultEnv->user.assign (MZNC_getUserName());
		pDefaultEnv->desktop = getDefaultDesktop();
		environments.push_back(pDefaultEnv);
	}
	return pDefaultEnv;
}

static char currentDesktop[1000] = "";
MazingerEnv* MazingerEnv::getEnv(const char *user) {
	return getEnv (user, getDefaultDesktop());
}

MazingerEnv* MazingerEnv::getEnv(const char *user, const char*desktop) {
	if ( (user == NULL || strcmp(user, MZNC_getUserName()) == 0) &&
			(desktop == NULL || strcmp(desktop, getDefaultDesktop()) == 0))
		return getDefaulEnv();
	for (std::vector<MazingerEnv*>::iterator it = environments.begin ();
			it != environments.end(); it++)
	{
		MazingerEnv* env = *it;
		if (env->user == user && env->desktop == desktop)
			return env;
	}
	MazingerEnv *env = new MazingerEnv;
	env->user.assign (user);
	env->desktop = desktop;
	environments.push_back(env);
	return env;
}

const PMAZINGER_DATA MazingerEnv::getData () {
	return open (true);
}

PMAZINGER_DATA MazingerEnv::getDataRW () {
	return open (false);
}

#ifdef WIN32
extern "C" BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(
  LPCWSTR StringSecurityDescriptor,
  DWORD StringSDRevision,
  PSECURITY_DESCRIPTOR *SecurityDescriptor,
  PULONG SecurityDescriptorSize
);
#define SDDL_REVISION_1 1
#define SDDL_REVISION SDDL_REVISION_1
//


#define MAZINGER_HEADER_SIZE (sizeof (MAZINGER_DATA)+10)
#define MAZINGER_DATA_PREFIX L"MAZINGER_CONFIG_%hs_%hs"
HINSTANCE hMazingerInstance;

static void fatalError() {
	LPWSTR pstr;
	DWORD dw = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dw, 0, (LPWSTR) & pstr, 0, NULL);
	if (pstr == NULL)
		wprintf(L"Unknown error: %d\n", dw);
	else
		wprintf(L"Error: %s\n", pstr);
	LocalFree(pstr);
	ExitProcess(1);
}




/////////////////////////////////////////////////
static PSECURITY_DESCRIPTOR createSecurityDescriptor() {
	static PSECURITY_DESCRIPTOR pSD;

	/* Initialize a security descriptor. */

	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
			SECURITY_DESCRIPTOR_MIN_LENGTH); /* defined in WINNT.H */
	if (pSD == NULL) {
		return NULL;
	}

	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) { /* defined in WINNT.H */
		goto Cleanup;
	}

	/* Add a NULL disc. ACL to the security descriptor. */

	if (!SetSecurityDescriptorDacl(pSD, TRUE, /* specifying a disc. ACL  */
			(PACL) NULL, FALSE)) { /* not a default disc. ACL */
		goto Cleanup;
	}

	return pSD;
	/* Add the security descriptor to the file. */
	Cleanup: return NULL;
}

#define LOW_INTEGRITY_SDDL_SACL_W L"S:(ML;;NW;;;LW)"

static void SetLowLabelToFile(LPWSTR lpszFileName) {
	// The LABEL_SECURITY_INFORMATION SDDL SACL to be set for low integrity
	DWORD dwErr = ERROR_SUCCESS;
	PSECURITY_DESCRIPTOR pSD = NULL;

	PACL pSacl = NULL; // not allocated
	BOOL fSaclPresent = FALSE;
	BOOL fSaclDefaulted = FALSE;

	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
			LOW_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL)) {
		if (GetSecurityDescriptorSacl(pSD, &fSaclPresent, &pSacl,
				&fSaclDefaulted)) {
			// Note that psidOwner, psidGroup, and pDacl are
			// all NULL and set the new LABEL_SECURITY_INFORMATION
			dwErr = SetNamedSecurityInfoW(lpszFileName, SE_FILE_OBJECT,
					LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, pSacl);
		}
		LocalFree(pSD);
	}
}

PMAZINGER_DATA MazingerEnv::createMazingerData() {
	WCHAR ach[200];

	wsprintfW(ach, MAZINGER_DATA_PREFIX, user.c_str(), desktop.c_str());

	CharLowerW(ach);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof sa;
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = createSecurityDescriptor();


	hMapFile = CreateFileMappingW((HANDLE) -1, // Current file handle.
			&sa, // Default security.
			PAGE_READWRITE, // Read/write permission.
			0,
			sizeof (MAZINGER_DATA), // Size of hFile.
			ach); // Name of mapping object.

	if (hMapFile == NULL) {
		wprintf (L"Unable to create shared memory %s\n", ach);
		fatalError();
		return NULL;
	}

	SetLowLabelToFile (ach);
	hMapFileRW = true;


	pMazingerData = (PMAZINGER_DATA) MapViewOfFile(hMapFile, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, 0);
	if (pMazingerData == NULL) {
		wprintf(L"Unable to map shared memory (RW) %s\n", ach);
		fatalError();
		return NULL;
	}

	pMazingerData -> achSecrets[0] = L'\0';
	pMazingerData -> debugLevel = 0;
	pMazingerData -> spy = 0;
	pMazingerData -> dwRulesSize = 0;
	pMazingerData -> achRulesFile[0] = '\0';
	return pMazingerData;
}

static MAZINGER_DATA nullMazingerData;

PMAZINGER_DATA MazingerEnv::open (bool readOnly) {
	if (pMazingerData != NULL && hMapFile != NULL && (readOnly || hMapFileRW)) {
		return pMazingerData;
	}

	WCHAR ach[200];

	if (! MZNC_waitMutex()) {
		return NULL;
	}


	if (hMapFile != NULL) {
		if (pMazingerData != NULL)
			UnmapViewOfFile(pMazingerData);
		pMazingerData = NULL;
		if (!CloseHandle(hMapFile))
			MZNSendDebugMessageA("Cannot close memory handle");
		hMapFile = NULL;
	}


	if (hMapFile == NULL) {
		wsprintfW(ach, MAZINGER_DATA_PREFIX, user.c_str(), desktop.c_str());
		CharLowerW(ach);
		hMapFile = OpenFileMappingW(readOnly ? FILE_MAP_READ : FILE_MAP_WRITE, // Read/write permission.
				FALSE, // Max. object size.
				ach); // Name of mapping object.
		if (hMapFile != NULL) {
			hMapFileRW = !readOnly;
		}
	}

	if (hMapFile == NULL) {
		if (!readOnly) {
			createMazingerData();
		} else {
			nullMazingerData.achSecrets[0] = L'\0';
			nullMazingerData.achSecrets[1] = L'\0';
			nullMazingerData.debugLevel = 0;
			nullMazingerData.dwRulesSize = 0;
			nullMazingerData.spy = 0;
			nullMazingerData.started = FALSE;
			nullMazingerData.achRulesFile[0] = '\0';
			pMazingerData = &nullMazingerData;
		}
	} else {

		pMazingerData = (PMAZINGER_DATA) MapViewOfFile(hMapFile, readOnly? FILE_MAP_READ: FILE_MAP_ALL_ACCESS,
				0, 0, 0);
	}

	MZNC_endMutex ();
	return pMazingerData;
}

void MazingerEnv::close() {
	if (pMazingerData != NULL)
		UnmapViewOfFile(pMazingerData);
	pMazingerData = NULL;
	if (hMapFile != NULL)
		CloseHandle(hMapFile);
	hMapFile = NULL;
}
#else
PMAZINGER_DATA MazingerEnv::open (bool readOnly) {
	if (pMazingerData != NULL && ( readOnly || ! openReadOnly))
		return pMazingerData;

	if (shm < 0 || ( readOnly && ! openReadOnly)) {
		if (shm < 0)
			::close (shm);
		if (shmName.size() == 0) {
			shmName.assign ("/MazingerData-");
			shmName.append (user);
		}
		SeyconCommon::debug ("Opening %s\n", shmName.c_str());
		shm = shm_open (shmName.c_str(), readOnly ? O_RDONLY : O_RDWR, 0600);
		if (shm < 0) {
			if (readOnly) return NULL;

			shm = shm_open (shmName.c_str(), O_RDWR|O_CREAT, 0600);
			if (shm < 0) {
				if (! readOnly) {
					fprintf (stderr, "Unable to create shared memory %s", shmName.c_str());
					perror ("Error: ");
				}
				return NULL;
			}
			ftruncate(shm, sizeof (MAZINGER_DATA));
			struct passwd *pw = getpwnam(user.c_str());
			if ( pw != NULL) {
	//			printf ("Asigning to permission to %s\n", pw->pw_gecos);
				fchown (shm, pw->pw_uid, pw->pw_gid);
				fchmod (shm, 0600);
			}
		}
	}

	if (pMazingerData != NULL)
		munmap(pMazingerData, sizeof (MAZINGER_DATA));
	pMazingerData = (PMAZINGER_DATA) mmap (NULL, sizeof (MAZINGER_DATA),
			readOnly ? PROT_READ: PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
	if (pMazingerData == MAP_FAILED) {
		pMazingerData = NULL;
		SeyconCommon::debug ("Cannot map %s\n", shmName.c_str());
	}
	else
		SeyconCommon::debug ("Opened %s\n", shmName.c_str());
	openReadOnly = readOnly;

	return pMazingerData;
}

void MazingerEnv::close () {
	if (pMazingerData != NULL && shm >= 0)
	{
		munmap(pMazingerData, sizeof (MAZINGER_DATA));
		::close (shm);
	}
}


#endif

ConfigReader * MazingerEnv::getConfigReader () {
	MZNC_waitMutex();
	if (reader == NULL) {
		PMAZINGER_DATA pData = getData();
		if (pData == NULL)
			return NULL;
		reader = new ConfigReader (open(true));
		MZNSendDebugMessageA("Parsing Mazinger data");
		reader->parse();
		MZNSendDebugMessageA("END Parsing Mazinger data");
	}
	MZNC_endMutex();
	return reader;
}
