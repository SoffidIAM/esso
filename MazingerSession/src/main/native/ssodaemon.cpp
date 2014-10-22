#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <ctype.h>
#include <winuser.h>
#include <wtsapi32.h>
typedef int socklen_t;

#else

#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR, *LPSOCKADDR;
typedef unsigned int DWORD;
#define closesocket close

#endif

#include <stdio.h>
#include "mazinger.h"
#include "ssodaemon.h"
#include <ssoclient.h>
#include <MZNcompat.h>
#include <MazingerInternal.h>
#include <MazingerHook.h>
#include <string>

#define MAX_CHALLENGE 128000

SsoDaemon::SsoDaemon ()
{
	socket_in = 0;
	publicSocket = 0;
	stop = false;
	session = NULL;
}

int SsoDaemon::getErrorNumber ()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

///////////////////////////////////////////////////////////7
void SsoDaemon::createInputSocket ()
{
	socklen_t size;
	SOCKADDR_IN addrin;
	socket_in = socket(PF_INET, SOCK_STREAM, 0);
	SeyconCommon::debug("Created socket: %d\n", (int) socket_in);
	size = sizeof addrin;
	if (socket_in < 0)
	{
		fprintf(stderr, "Error creating socket\n");
		exit (1);
	}
	addrin.sin_family = AF_INET;
	addrin.sin_addr.s_addr = 0;
	addrin.sin_port = 0;
	if (bind(socket_in, (LPSOCKADDR) &addrin, sizeof(addrin)) != 0)
	{
		printf("Error linking socket\n");
		exit(1);
	}
	if (getsockname(socket_in, (LPSOCKADDR) &addrin, &size) != 0)
	{
		fprintf(stderr, "Error obtaining socket name:%d \n",getErrorNumber());
		exit (1);
	}
	publicSocket = ntohs(addrin.sin_port);
	if (listen(socket_in, 2) != 0)
	{
		fprintf(stderr, "Error listening port\n");
		exit (1);
	}
}

struct ThreadParam
{
	public:
		ThreadParam (SsoDaemon *d, SOCKET s)
		{
			this->daemon = d;
			this->s = s;
		}
		SsoDaemon *daemon;
		SOCKET s;
};

//////////////////////////////////////////////////////////
#ifdef WIN32
static int WINAPI _s_handleConnection (LPVOID lpv)
#else
static void* _s_handleConnection (void * lpv)
#endif
{
	ThreadParam *p = (ThreadParam*) lpv;
	p->daemon->handleConnection(p->s);
	delete p;
#ifdef WIN32
	ExitThread(0);
	return 0;
#else
	pthread_exit (NULL);
	return NULL;
#endif
}

void SsoDaemon::handleConnection (SOCKET s)
{

	int len;
	char achBuffer[2048];

	len = recv(s, achBuffer, sizeof achBuffer - 1, 0);
	achBuffer[len] = '\0';
	if (strncmp(achBuffer, "WHO", 3) == 0)
	{
		char ach[2048];
		sprintf(ach, "%s\n", sessionId.c_str());
		send(s, ach, strlen(ach), 0);
	}
	else if (strncmp(achBuffer, "KEY", 3) == 0)
	{
		if (session != NULL)
			session->renewSecrets(&achBuffer[3]);
	}
	else if (strncmp(achBuffer, "ALERT ", 6) == 0)
	{
		send(s, "\n", 1, 0);
		if (session != NULL)
			session->notify(MZNC_utf8tostr(&achBuffer[6]).c_str());
	}
	else if (strncmp(achBuffer, "LOGOUT ", 7) == 0)
	{
		if (session != NULL)
		{
			char *key = &achBuffer[7];
			int l;
			for (l = strlen(key); l > 0 && key[l - 1] <= ' '; l--)
				;
			key[l] = '\0';
			session->remoteClose(key);
		}
	}
	else if (strncmp(achBuffer, "APP ", 4) == 0)
	{
		if (session != NULL)
		{
			std::string key = &achBuffer[4];
			size_t end = key.find(' ');
			size_t begin2 = key.find_first_not_of(' ', end);
			size_t end2 = key.find(' ', begin2);
			std::string id = key.substr(begin2, end2 - begin2);
			key = key.substr(0, end);

			SeyconService service;
			std::wstring wid = service.escapeString(id.c_str());
			service.resetServerStatus();
			SeyconResponse *response = service.sendUrlMessage(
					L"/getapplication?user=%hs&key=%hs&id=%ls", session->getUser(),
					session->getSessionKey(), wid.c_str());
			if (response != NULL)
			{
				std::string status = response->getToken(0);
				if (status == "OK")
				{
					std::string type = response->getToken(1);
					std::string content = response->getUtf8Tail(2);
				}
				else
				{
					send(s, "ERROR", 6, 0);
					std::string msg = response->getUtf8Token(1);
					send(s, msg.c_str(), msg.length() + 1, 0);
				}
				delete response;
			}
			else
			{
				send(s, "ERROR", 6, 0);
				const char *msg = "Unable to access network";
				send(s, msg, strlen(msg) + 1, 0);
			}
		}
	}
	len = recv(s, achBuffer, sizeof achBuffer - 1, 0);
	closesocket(s);
}

//////////////////////////////////////////////////////////
#ifdef WIN32
static int WINAPI _s_startLoop (LPVOID lpv)
#else
static void* _s_startLoop (void* lpv )
#endif
{
	ThreadParam *p = (ThreadParam*) lpv;
	p->daemon->runSessionServer();
//	delete p;
#ifdef WIN32
	ExitThread(0);
	return 0;
#else
	pthread_exit (NULL);
	return NULL;
#endif
}

#ifdef WIN32
static int WINAPI _s_startLoop2 (LPVOID lpv)
#else
static void* _s_startLoop2 (void* lpv )
#endif
{
	ThreadParam *p = (ThreadParam*) lpv;
	p->daemon->runKeepAlive();
//	delete p;
#ifdef WIN32
	ExitThread(0);
	return 0;
#else
	pthread_exit (NULL);
	return NULL;
#endif
}

void SsoDaemon::runSessionServer ()
{
	SOCKADDR addr;
	SOCKET s;
	socklen_t len;

	while (!stop)
	{
		len = sizeof addr;
		s = accept(socket_in, &addr, &len);

		if (stop || s == (SOCKET) -1)
		{
#ifdef WIN32
			ExitThread(0);
#else
			pthread_exit (NULL);
#endif
		}
		else
		{
			ThreadParam *data = new ThreadParam(this, s);
#ifdef WIN32
			HANDLE hThread;
			hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) _s_handleConnection,
					(LPVOID) data, 0, (LPDWORD) &hThread);
			CloseHandle(hThread);
#else
			pthread_t thread1;
			pthread_create( &thread1, NULL, _s_handleConnection, (void*) data);
#endif
		}
	}
}

void SsoDaemon::runKeepAlive ()
{
	SeyconCommon::updateConfig("soffid.esso.session.keepalive");

	std::string kastring;
	long keepalive = 600;
	if (SeyconCommon::readProperty("soffid.esso.session.keepalive", kastring))
	{
		keepalive = atoi(kastring.c_str());
	}

	int delay = keepalive;
	while (keepalive > 0 && !stop)
	{
#ifdef WIN32
		Sleep(delay*1000);
#else
		sleep(delay);
#endif
		if (! stop)
		{
			if ( !doKeepAlive () && delay > 5)
			{
				delay = delay / 2;
			}
			else
				delay = keepalive;
		}
	}
}

//////////////////////////////////////////////////////////
int SsoDaemon::startDaemon ()
{
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif

	createInputSocket();

	ThreadParam *data = new ThreadParam(this, socket_in);
	ThreadParam *data2 = new ThreadParam(this, socket_in);
	stop = false;
#ifdef WIN32
	DWORD dwThreadId, dwThreadId2;
	HANDLE hThreadListen = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) _s_startLoop,
			(LPVOID) data, 0, &dwThreadId);

	CloseHandle(hThreadListen);
	HANDLE hThreadListen2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) _s_startLoop2,
			(LPVOID) data2, 0, &dwThreadId2);
	CloseHandle(hThreadListen2);
#else
	pthread_t thread1;
	pthread_create( &thread1, NULL, _s_startLoop, (void*) data);
	pthread_t thread2;
	pthread_create( &thread2, NULL, _s_startLoop2, (void*) data2);
#endif

	return publicSocket;
}

//////////////////////////////////////////////////////////
void SsoDaemon::stopDaemon ()
{
	stop = true;
#ifdef WIN32
	closesocket(socket_in);
#else
	close (socket_in);
#endif
}

bool SsoDaemon::doKeepAlive() {
	bool ok = false;
	SeyconService service;
	SeyconResponse *response = service.sendUrlMessage(
			L"/keepAliveSession?user=%hs&key=%hs", session->getSoffidUser(),
			session->getSessionKey());

	if (response != NULL)
	{
		std::string status = response->getToken(0);
		if (status == "OK")
			ok = true;
		else if (status == "EXPIRED")
		{
			MZNStop(MZNC_getUserName());
			session->close ();
			if (session->restartSession() != LOGIN_SUCCESS)
			{
				if (session->getDialog() != NULL)
					session->getDialog()->notify("Soffid session has been remotely closed");
			}
		}
		delete response;
	}

	return ok;
}

