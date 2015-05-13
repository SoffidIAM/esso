#ifndef _CREATE_USER_DAEMON_H

#define _CREATE_USER_DAEMON_H

#include <winsock.h>



class CreateUserDaemon
{
	public:
		CreateUserDaemon ();
		int startDaemon ();
		void stopDaemon ();
		void handleConnection (SOCKET s);
		void run();

	private:
		SOCKET socket_in;
		int publicSocket;
		bool stop;

		int getErrorNumber ();
		void createInputSocket ();
};

#endif

