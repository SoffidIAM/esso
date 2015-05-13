#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <MazingerInternal.h>

#ifdef WIN32

#include <sddl.h>
#include <psapi.h>
#include <accctrl.h>
#include <aclapi.h>

#else

#include <fcntl.h>
#include <sys/mman.h>
#include <syslog.h>
#include <stdarg.h>
#include <wchar.h>
#include <unistd.h>

#endif

#include <vector>
#include <DomainPasswordCheck.h>
#include <ConfigReader.h>
#include <Action.h>
#include <SecretStore.h>


#ifdef WIN32
// Missing at sddl.h
wchar_t achMailSlotName[4096] = L"";
static FILE *logFile = NULL;
void sendMessage(HANDLE hMailSlot, LPCWSTR lpszMessage) {

	if (hMailSlot != NULL) {
		DWORD dwWritten = 0;
		WriteFile(hMailSlot, lpszMessage, 2 * wcslen(lpszMessage), &dwWritten,
				NULL);
	}
}

void sendDebugMessage(LPCWSTR lpszMessage) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel) {
		if (achMailSlotName[0] == L'\0') {
			WCHAR achUser[1024];
			DWORD userSize = sizeof achUser;
			GetUserNameW(achUser, &userSize);
			CharLowerW(achUser);
			wsprintfW(achMailSlotName, L"\\\\.\\mailslot\\MAZINGER_%s", achUser);
		}
		HANDLE hMailSlot = CreateFileW(achMailSlotName, GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0,
				NULL);
		if (hMailSlot != NULL) {
			sendMessage(hMailSlot, lpszMessage);
			CloseHandle(hMailSlot);
		}
	}
}

void sendSpyMessage(LPCWSTR lpszMessage) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->spy) {
		HANDLE hSpyMailSlot;
		WCHAR achUser[1024];
		WCHAR ach[1512];
		DWORD userSize = sizeof achUser;
		GetUserNameW(achUser, &userSize);
		CharLowerW(achUser);
		wsprintfW(ach, L"\\\\.\\mailslot\\MAZINGER_SPY_%s", achUser);
		hSpyMailSlot = CreateFileW(ach, GENERIC_WRITE, FILE_SHARE_READ
				| FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		sendMessage(hSpyMailSlot, lpszMessage);
		CloseHandle(hSpyMailSlot);
	}
}

void MZNSendDebugMessageW(LPCWSTR lpszMessage, ...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel) {
		va_list v;

		va_start(v, lpszMessage);
		WCHAR achMessage[5001];
		_vsnwprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = L'\0';
		sendDebugMessage(achMessage);
	}
}

void MZNSendDebugMessageA(LPCSTR lpszMessage,
		...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if ( pMazinger != NULL && pMazinger->debugLevel) {
		va_list v;

		va_start(v, lpszMessage);
		char achMessage[5001];
		_vsnprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = '\0';
		WCHAR wchMessage[10024];
		mbstowcs(wchMessage, achMessage, strlen(achMessage));
		wchMessage[strlen(achMessage)] = L'\0';
		sendDebugMessage(wchMessage);
	}
}

void MZNSendTraceMessageW(LPCWSTR lpszMessage,
		...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel > 1) {
		va_list v;

		va_start(v, lpszMessage);
		WCHAR achMessage[5001];
		_vsnwprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = L'\0';
		sendDebugMessage(achMessage);
	}
}

void MZNSendTraceMessageA(LPCSTR lpszMessage,
		...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel > 1) {
		va_list v;

		va_start(v, lpszMessage);
		char achMessage[5001];
		_vsnprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = '\0';
		WCHAR wchMessage[10024];
		mbstowcs(wchMessage, achMessage, strlen(achMessage));
		wchMessage[strlen(achMessage)] = L'\0';
		sendDebugMessage(wchMessage);
	}
}

void MZNSendSpyMessageW(LPCWSTR lpszMessage, ...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->spy) {
		va_list v;

		va_start(v, lpszMessage);
		WCHAR achMessage[5001];
		_vsnwprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = L'\0';
		sendSpyMessage(achMessage);
	}
}

void MZNSendSpyMessageA(LPCSTR lpszMessage, ...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->spy) {
		va_list v;

		va_start(v, lpszMessage);
		char achMessage[5001];
		_vsnprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = '\0';
		WCHAR wchMessage[10024];
		mbstowcs(wchMessage, achMessage, strlen(achMessage));
		wchMessage[strlen(achMessage)] = L'\0';
		sendSpyMessage(wchMessage);
	}
}


#else

#if 0
static bool openReadOnly = false;
static PMAZINGER_DATA pMazingerData = NULL;
static int shm = -1;
char *shmName;
PMAZINGER_DATA openMazingerData(bool readOnly) {
	if (pMazingerData != NULL && ( readOnly || ! openReadOnly))
		return pMazingerData;

	if (shm < 0) {
		if (shmName == NULL) {
			const char *user = MZNC_getUserName();
			shmName = (char *) malloc (strlen(user)+20);
			sprintf (shmName, "MazingerData-%s", user);
		}
		shm = shm_open (shmName, O_RDWR, 0600);
		if (shm < 0) {
			if (readOnly) return NULL;

			shm = shm_open (shmName, O_RDWR|O_CREAT, 0600);
			if (shm < 0) {
				printf ("Unable to create shared memory %s", shmName);
				return NULL;
			}
		}
		ftruncate(shm, sizeof (MAZINGER_DATA));
	}

	pMazingerData = (PMAZINGER_DATA) mmap (NULL, sizeof (MAZINGER_DATA), PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
	if (pMazingerData == MAP_FAILED) {
		pMazingerData = NULL;
	}
	openReadOnly = readOnly;

	return pMazingerData;
}

void releaseMazingerData () {
	if (pMazingerData != NULL && shm >= 0)
	{
		munmap(pMazingerData, sizeof (MAZINGER_DATA));
		close (shm);
		shm_unlink(shmName);
	}
}

#endif
void sendMessage( int priority, const char* lpszMessage) {
	static char achMessageFile[500] = "";
	FILE *debugFile =NULL;

//	openlog ("mazinger", LOG_PID|LOG_ODELAY, LOG_USER);
//	syslog (priority, "%s", lpszMessage);

	if (achMessageFile[0] == '\0') {
		sprintf (achMessageFile, "%s/.config/mazinger/debug", getenv ("HOME"));
	}
//	fprintf (stderr, "[%s]: %s\n", achMessageFile, lpszMessage);
	debugFile = fopen (achMessageFile, "a");
	if (debugFile != NULL)
	{
		fprintf (debugFile, "%s\n", lpszMessage);
		fclose (debugFile);
		debugFile = NULL;
	}
}

void sendDebugMessage(const char *lpszMessage) {
	MazingerEnv *pEnv = MazingerEnv::getDefaulEnv();
	PMAZINGER_DATA pMazinger = pEnv->getData();
	if (pMazinger != NULL && pMazinger->debugLevel) {
		sendMessage (LOG_INFO, lpszMessage);
	}
}

void sendSpyMessage(const char* lpszMessage) {
	static char achSpyFile[500] = "";
	FILE *spyFile = NULL;
	if (achSpyFile[0] == '\0') {
		sprintf (achSpyFile, "%s/.config/mazinger/spy", getenv ("HOME"));
	}
	spyFile = fopen (achSpyFile, "a");
	if (spyFile != NULL)
	{
		fprintf (spyFile, "%s\n", lpszMessage);
		fclose (spyFile);
	}
}

void MZNSendDebugMessageW(const wchar_t* lpszMessage, ...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel) {
		va_list v;

		va_start(v, lpszMessage);
		wchar_t achMessage[5001];
		vswprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = L'\0';
		std::string sz = MZNC_wstrtostr(achMessage);
		sendDebugMessage(sz.c_str());
	}
}

void MZNSendDebugMessageA(const char* lpszMessage,
		...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel) {
		va_list v;

		va_start(v, lpszMessage);
		char achMessage[5001];
		vsnprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = '\0';
		sendDebugMessage(achMessage);
	}
}

void MZNSendTraceMessageW(const wchar_t* lpszMessage,
		...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel > 1) {
		va_list v;

		va_start(v, lpszMessage);
		wchar_t achMessage[5001];
		vswprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = L'\0';
		std::string sz = MZNC_wstrtostr(achMessage);
		sendDebugMessage(sz.c_str());
	}
}

void MZNSendTraceMessageA(const char* lpszMessage,
		...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->debugLevel > 1) {
		va_list v;

		va_start(v, lpszMessage);
		char achMessage[5001];
		vsnprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = '\0';
		sendDebugMessage(achMessage);
	}
}

void MZNSendSpyMessageW(const wchar_t* lpszMessage, ...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->spy) {
		va_list v;

		va_start(v, lpszMessage);
		wchar_t achMessage[5001];
		vswprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = L'\0';
		std::string sz = MZNC_wstrtostr(achMessage);
		sendSpyMessage(sz.c_str());
	}
}

void MZNSendSpyMessageA(const char * lpszMessage, ...) {
	PMAZINGER_DATA pMazinger = MazingerEnv::getDefaulEnv()->getData();
	if (pMazinger != NULL && pMazinger->spy) {
		va_list v;

		va_start(v, lpszMessage);
		char achMessage[5001];
		vsnprintf(achMessage, sizeof achMessage, lpszMessage, v);
		va_end(v);
		achMessage[5000] = '\0';
		sendSpyMessage(achMessage);
	}
}

#endif

#if 0
PMAZINGER_DATA getMazingerData() {
	return openMazingerData(true);
}

PMAZINGER_DATA getMazingerDataRW() {
	return openMazingerData(false);
}


#endif


const char* xmlEncode(const char* str)
{
	static char result[10025];
	int i = 0;
	int j = 0;
	while (j < 10024 && str[i]) {
		switch (str[i]) {
		case '&':
			result[j++] = '&';
			result[j++] = 'a';
			result[j++] = 'm';
			result[j++] = 'p';
			result[j++] = ';';
			break;
		case '<':
			result[j++] = '&';
			result[j++] = 'l';
			result[j++] = 't';
			result[j++] = ';';
			break;
		case '>':
			result[j++] = '&';
			result[j++] = 'g';
			result[j++] = 't';
			result[j++] = ';';
			break;
		case '\'':
			result[j++] = '&';
			result[j++] = 'a';
			result[j++] = 'p';
			result[j++] = 'o';
			result[j++] = 's';
			result[j++] = ';';
			break;
		case '\"':
			result[j++] = '&';
			result[j++] = 'q';
			result[j++] = 'u';
			result[j++] = 'o';
			result[j++] = 't';
			result[j++] = ';';
			break;
		case '^':
		case '$':
		case '.':
		case '[':
		case ']':
		case '\\':
		case '(':
		case ')':
		case '{':
		case '}':
		case '?':
		case '+':
		case '*':
			result[j++] = '\\';
		default:
			result[j++] = str[i];
		}
		i++;
	}
	result[j] = L'\0';
	return result;
}

