#ifndef _SSO_DAEMON_H

#define _SSO_DAEMON_H

#ifdef WIN32

#include <winsock.h>

#else

typedef int SOCKET;

#endif

class SeyconSession;

class SsoDaemon
{
	public:
		SsoDaemon ();
		std::string sessionId;
		int startDaemon ();
		void stopDaemon ();
		SeyconSession *session;

		void runSessionServer ();
		void runKeepAlive ();
		void handleConnection (SOCKET s);

	private:
		SOCKET socket_in;
		int publicSocket;
		bool stop;

		int getErrorNumber ();
		void createInputSocket ();
		bool doKeepAlive ();
};

#endif

