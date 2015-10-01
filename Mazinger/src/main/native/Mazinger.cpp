#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>

#else

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <pthread.h>
#include <locale.h>

#include <sys/stat.h>

#include <unistd.h>

#define ExitProcess exit
#define wcsicmp wcscasecmp

#endif

#include <MZNcompat.h>
#include <stdio.h>
#include <MazingerHook.h>
#include <ssoclient.h>
#include <time.h>

extern bool MZNEvaluateJS(const char *script, std::string &msg);


char achUser[8192] = "";

#ifdef WIN32
// Missing at sddl.h
extern "C" BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(
  LPCWSTR StringSecurityDescriptor,
  DWORD StringSDRevision,
  PSECURITY_DESCRIPTOR *SecurityDescriptor,
  PULONG SecurityDescriptorSize
);
#define SDDL_REVISION_1 1
#define SDDL_REVISION SDDL_REVISION_1
//

DWORD CALLBACK LeerMailSlot(LPVOID lpData);
HANDLE createDebugMailSlot ();
#else

void* LeerMailSlot (void * lpv) ;

#endif

int debugLevel = 0;
FILE *logFile = NULL;


/** Arguments */
bool bCreateSession = true;
std::wstring configFile;
std::wstring credentialsFile;


void parseArgs (int argc, char **argv)
{
	strcpy (achUser, MZNC_getUserName());
	int i = 2;
	while (i < argc) {
		if (argv[i][0] == '-')
		{
			if (strcmp (argv[i], "-koji") == 0)
			{
				bCreateSession = false;
				i++;
			}
			else if (strcmp (argv[i], "-debug") == 0)
			{
				debugLevel = 1;
				i++;
			}
			else if (strcmp (argv[i], "-trace") == 0)
			{
				debugLevel = 2;
				i++;
			}
			else if (strcmp (argv[i], "-credentials") == 0)
			{
				i++;
				if ( i < argc ) {
					credentialsFile = MZNC_strtowstr (argv[i++]);
				}
				else {
					printf ("Missing credentials parameter\n");
					ExitProcess(2);
				}
			}
			else if (strcmp (argv[i], "-user") == 0)
			{
				i++;
				strcpy (achUser,argv[i++]);
			}
			else
			{
				printf ("Unknown switch %s\n", argv[i]);
				ExitProcess(2);
			}
		}
		else if (configFile.empty())
		{
			configFile = MZNC_strtowstr (argv[i++]);
		}
		else
		{
			printf ("More than one configuration file specified:\n> %ls\n> %s\n",
					configFile.c_str(), argv[i]);
			ExitProcess(2);
		}
	}
}

void MazingerStopHandler (const char *user) {
	printf ("Mazinger stoped !\n");
	ExitProcess (0);
}


#ifdef WIN32
HANDLE ghSvcStopEvent = NULL;

void mazingerCallback (const char *user) {
	if (ghSvcStopEvent != NULL)
		SetEvent(ghSvcStopEvent);
}

#else
void mazingerCallback (const char *user) {
	exit(0);
}

#endif


void doStart() {
	wchar_t wchSecrets[8192] = L"user\0USER\0password\0Secret\0\0";
	memset (wchSecrets, 0, sizeof wchSecrets);
	wchar_t wchUser[8192] = L"";
	wchar_t wchPassword[8192] = L"";
	FILE *secretsFile = NULL;


	if (! credentialsFile.empty ())
	{
		std::string credFile = MZNC_wstrtostr(credentialsFile.c_str());
		printf("Reading secrets from %s\n", credFile.c_str());
		secretsFile = fopen(credFile.c_str(), "r");
	}
	if (secretsFile == NULL)
	{
		printf("Enter secrets\n");
		secretsFile = stdin;
	}

	// Initialization complete - report running status

	wchar_t wchSecret[512];
	int secretLen = 0;
	char achSecret[512];
	fgets(achSecret, 500, secretsFile);
	int l = strlen (achSecret)-1;
	while ( l > 0 )
	{
		achSecret[l] = '\0';
		mbstowcs(wchSecret, achSecret, sizeof achSecret);
		int len = wcslen (wchSecret);
		int i = 0;
		int foundEqual = 0;
		while (wchSecret[i] != '\0')
		{
			if (wchSecret[i] == L'=')
			{
				wchSecret[i] = L'\0';
				foundEqual = 1;
				if (wcsicmp (wchSecret, L"user") == 0)
				{
					strcpy (achUser, &achSecret[i+1]);
					wcscpy (wchUser, &wchSecret[i+1]);
				}
				else if (wcsicmp (wchSecret, L"password") == 0)
				{
					wcscpy (wchPassword, &wchSecret[i+1]);
				}
			}
			else
				i++;
		}
		if ( !foundEqual )
		{
			printf ("ERROR: Missing secret value [name=value]\n");
		}
		else
		{
			memcpy (&wchSecrets[secretLen], wchSecret, sizeof (wchar_t) * (len+1));
			secretLen += len+1;
		}
		fgets(achSecret, 500, secretsFile);
		l = strlen (achSecret)-1;
	}
	wchSecrets[secretLen] = '\0';

	if (secretsFile != stdin)
		fclose (secretsFile);

	if (MZNIsStarted(achUser))
	{
		printf("Mazinger is already started for user %s\n", achUser);
		return;
	}
	if (debugLevel > 0)
		printf ("Activating debug\n");
	MZNSetDebugLevel(achUser, debugLevel);
	SeyconCommon::setDebugLevel(debugLevel);

#ifdef WIN32
	if (debugLevel > 0)
	{
		HANDLE hSlot = createDebugMailSlot ();
		CreateThread(NULL, // sECURITY ATTRIBUTES
				0, // Stack Size,
				LeerMailSlot, hSlot, // Param
				0, // Options,
				NULL); // Thread id
	}
#else
	pthread_t thread1;
	pthread_create( &thread1, NULL, LeerMailSlot, (void*) "debug");
#endif

	SeyconSession s;
	s.setUser(achUser);
	s.setPassword(wchPassword);
	if (!configFile.empty())
	{
		if (debugLevel > 1)
			printf ("Loading configuration file %ls\n", configFile.c_str());

		if (! MZNLoadConfiguration(achUser, configFile.c_str()) )
		{
			printf ("Unable to load file %ls\n", configFile.c_str());
			return;
		}
	} else {
		printf ("Loading default configuration for user %s\n", achUser);
		s.updateMazingerConfig();
	}

	if (debugLevel > 1)
		printf ("Storing secrets\n");
	MZNSetSecrets (achUser,wchSecrets);
	if (debugLevel > 1)
		printf ("Installing hooks for %s\n", achUser);
	MZNStart(achUser);
	MZNSetStopNotifyCallback(MazingerStopHandler);
	if (debugLevel > 1)
		printf ("Checking expired passwords\n");
	MZNCheckPasswords(achUser);

	if (debugLevel > 1)
		printf ("Starting seycon session\n");

	if (bCreateSession)
	{
		s.weakSessionStartup(achUser, wchPassword);
	}


	if (debugLevel > 0)
	{
		if (debugLevel > 1)
			printf ("Started\n");
		printf("Press enter to stop\n");

		char buffer[10];
		fgets(buffer, 5, stdin);
		MZNStop(achUser);

	} else {
		MZNSetStopNotifyCallback(mazingerCallback);

#ifdef WIN32
		ghSvcStopEvent = CreateEventW(NULL, // default security attributes
				FALSE, // manual reset event
				FALSE, // not signaled
				NULL);
	    WaitForSingleObject(ghSvcStopEvent, INFINITE);
#endif
	}


}

void doStatus() {
	printf ("Testing status for user %s\n", achUser);
	MZNStatus(achUser);
}

void doDebug(int debugLevel) {
	// Initialization complete - report running status
	if (! MZNIsStarted(achUser))
	{
		printf ("Mazinger is not started for user %s\n", achUser);
//		return;
	}

#ifdef WIN32
	CreateThread(NULL, // sECURITY ATTRIBUTES
			0, // Stack Size,
			LeerMailSlot, NULL, // Param
			0, // Options,
			NULL); // Thread id
#else
	pthread_t thread1;
	pthread_create( &thread1, NULL, LeerMailSlot, (void*) "debug");
#endif
	if (MZNIsStarted(achUser))
		MZNSetDebugLevel(achUser,debugLevel);

	printf("Press enter to stop\n");

	char buffer[10];
	fgets(buffer, 5, stdin);

	MZNSetDebugLevel(achUser,0);

	ExitProcess(0);
}


#ifdef WIN32
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
			dwErr = SetNamedSecurityInfoW(lpszFileName, SE_KERNEL_OBJECT,
					LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, pSacl);
		}
		LocalFree(pSD);
	}
}

void SetLowLabelToMailslot(HANDLE hKernelObj) {
	// The LABEL_SECURITY_INFORMATION SDDL SACL to be set for low integrity
	DWORD dwErr = ERROR_SUCCESS;
	PSECURITY_DESCRIPTOR pSD = NULL;

	PACL pSacl = NULL; // not allocated
	BOOL fSaclPresent = FALSE;
	BOOL fSaclDefaulted = FALSE;

	if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
			L"S:(ML;;NW;;;LW)", SDDL_REVISION_1, &pSD, NULL)) {
		if (GetSecurityDescriptorSacl(pSD, &fSaclPresent, &pSacl,
				&fSaclDefaulted)) {
			// Note that psidOwner, psidGroup, and pDacl are
			// all NULL and set the new LABEL_SECURITY_INFORMATION
			dwErr = SetSecurityInfo(hKernelObj, SE_KERNEL_OBJECT,
					LABEL_SECURITY_INFORMATION, NULL, NULL, NULL, pSacl);
		}
		LocalFree(pSD);
	}
}

HANDLE createDebugMailSlot ()
{
	WCHAR achUser[10024];
	WCHAR ach[1512];
	DWORD userSize = sizeof achUser;
	GetUserNameW(achUser, &userSize);
	CharLowerW(achUser);
	wsprintfW(ach, L"\\\\.\\mailslot\\MAZINGER_%s", achUser);
	HANDLE hResult = CreateMailslotW(ach, 10024, MAILSLOT_WAIT_FOREVER, NULL);
	SetLowLabelToMailslot(hResult);
//	SetLowLabelToFile(ach);
	return hResult;
}

HANDLE createSpyMailSlot ()
{
	WCHAR achUser[10024];
	WCHAR ach[1512];
	DWORD userSize = sizeof achUser;
	GetUserNameW(achUser, &userSize);
	CharLowerW(achUser);
	wsprintfW(ach, L"\\\\.\\mailslot\\MAZINGER_SPY_%s", achUser);
	HANDLE hResult = CreateMailslotW(ach, 10024, MAILSLOT_WAIT_FOREVER, NULL);
	SetLowLabelToMailslot(hResult);
	return hResult;
}

DWORD CALLBACK LeerMailSlot(LPVOID lpData) {
	DWORD cbRead;
	WCHAR achMessage[10024];

	HANDLE hSlot = (HANDLE) lpData;
	if (hSlot == NULL)
		hSlot = createDebugMailSlot ();

	while (TRUE)
	{
		cbRead = 0;
		ReadFile (hSlot,
				achMessage,
				sizeof achMessage,
				&cbRead,
				(LPOVERLAPPED) NULL);
		if (cbRead > 0 )
		{
			time_t t;
			time(&t);
			struct tm* tm = localtime(&t);
			achMessage[cbRead/2] = L'\0';
			fwprintf (logFile == NULL ? stdout: logFile, L"%02d:%02d:%02d %ls\n", tm->tm_hour, tm->tm_min, tm->tm_sec, achMessage);
		}
	}
	return 0;
}

#else
void* LeerMailSlot (void * lpv) {
	char *name = (char*) lpv;
	char achFileName [1024];
	strcpy (achFileName, getenv ("HOME"));
	strcat (achFileName, "/.config/mazinger");
	mkdir (achFileName, 0700);
	strcat (achFileName, "/");
	strcat (achFileName, name);


	unlink (achFileName);

	long int pos = 0;
	while (true) {
		FILE *f = fopen (achFileName, "r+");
		if (f != NULL)
		{
			char ch;
			fseek (f, pos, SEEK_SET);
			int read = fread (&ch, 1, 1, f);
			while (read > 0) {
				fwrite (&ch, 1, 1, logFile == NULL ? stdout: logFile);
				read = fread (&ch, 1, 1, f);
			}
			pos = ftell (f);
			fclose (f);
		}
		sleep(1);
	}
	return 0;
}


#endif

void doSpy(const char *fileName) {

	if (! MZNIsStarted(achUser))
	{
		printf ("Mazinger is not started for user %s\n", achUser);
		return;
	}

	// Initialization complete - report running status
	if (fileName != NULL)
	{
		logFile = fopen (fileName, "w");
		if (logFile != NULL)
		{
			printf ("Dumping data to %s\n", fileName);
			setbuf(logFile, NULL);
		}
	}


#ifdef WIN32
	HANDLE mailSlot = createSpyMailSlot ();
	CreateThread(NULL, // sECURITY ATTRIBUTES
			0, // Stack Size,
			LeerMailSlot, mailSlot, // Param
			0, // Options,
			NULL); // Thread id
#else
	pthread_t thread1;
	pthread_create( &thread1, NULL, LeerMailSlot, (void*) "spy");
#endif
	MZNEnableSpy(achUser,1);

	printf("Press enter to stop\n");

	char buffer[10];
	fgets(buffer, 5, stdin);

	MZNEnableSpy(achUser, 0);

	ExitProcess(0);
}

void doScript (const char *file) {
	std::string s;

	FILE *f  = fopen (file, "r");
	if (f == NULL) {
		printf ("Cannot open file %s\n", file);
	} else {
		std::string result;
		char buffer[513];
		while (true) {
			int read = fread (buffer, 1, (sizeof buffer) -1, f);
			if (read <= 0 )
				break;
			buffer[read] = '\0';
			s += buffer;
		}
		if ( ! MZNEvaluateJS(s.c_str(), result) ) {
			printf ("Error evaluating script:\n%s\n", result.c_str());

		}
	}
}

extern "C" int main(int argc, char**argv) {
	if (argc == 1) {
		printf("Mazinger version 1.1\n");
		printf("Usage: mazinger start [-debug] [-trace] [configuration_file]\n");
		printf("       mazinger script <script_file>\n");
		printf("       mazinger stop\n");
		printf("       mazinger status\n");
		printf("       mazinger debug\n");
		printf("       mazinger trace\n");
		printf("       mazinger spy [log_file]\n");
		ExitProcess(0);
	}

	strcpy (achUser, MZNC_getUserName());
	if (stricmp("start", argv[1]) == 0)
	{
		parseArgs(argc, argv);
		doStart ();
	}
	else if (stricmp("spy", argv[1]) == 0)
	{
		doSpy (argc > 2 ? argv[2]: NULL);
	}
	else if (stricmp("trace", argv[1]) == 0)
	{
		doDebug (2);
	}
	else if (stricmp("debug", argv[1]) == 0)
	{
		doDebug (1);
	}
	else if (stricmp("status", argv[1]) == 0)
	{
		parseArgs(argc, argv);
		doStatus ();
	}
	else if (stricmp("stop", argv[1]) == 0)
	{
		parseArgs(argc, argv);
		MZNStop (achUser);

		printf ("Mazinger stopped\n");
	}
	else if (stricmp("pause", argv[1]) == 0)
	{
		printf ("Press enter to stop\n");

		char buffer[10];
		fgets (buffer, 5, stdin);
	}
	else if (stricmp("script", argv[1]) == 0 && argc >= 2)
	{
		doScript (argv[2]);
	} else {
		printf ("Syntax error: %s\n", argv[1]);
	}
	ExitProcess( 0 );

}


const wchar_t* askCard(wchar_t const* lpszCardNumber , wchar_t const* lpszCardPosition) {
	return NULL;
}
