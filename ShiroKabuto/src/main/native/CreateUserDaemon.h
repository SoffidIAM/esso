#ifndef _CREATE_USER_DAEMON_H

#define _CREATE_USER_DAEMON_H

#include <winsock.h>
#include <string>


class CreateUserDaemon
{
	public:
		CreateUserDaemon ();
		int startDaemon ();
		void stopDaemon ();
		void handleConnection (SOCKET s);
		void run();

		bool debug;

	private:
		SOCKET socket_in;
		int publicSocket;
		bool stop;

		int getErrorNumber ();
		void createInputSocket ();
		std::wstring readLine(SOCKET socket);
		void writeLine(SOCKET socket, const std::wstring &line);

		void processLocalUser(SOCKET socket);
		void validatePassword(SOCKET socket, const std::wstring &params);
		void storeCredentials(SOCKET socket, const std::wstring &params);
		void getCredentials(SOCKET socket, const std::wstring &params);
};

#endif

