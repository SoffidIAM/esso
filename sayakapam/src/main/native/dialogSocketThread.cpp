#include "PamHandler.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pwd.h>
#include <unistd.h>

void * PamHandler::pamSocketDialogThread (void *data) {
	PamHandler *pamh = (PamHandler*) data;
	pamh->dialogThread();
	return 0;
}


int PamHandler::createSocket (const char *name) {
	std::string socketName = "/tmp/sayaka-";
	socketName += session.getSessionId();
	mkdir (socketName.c_str(), 0700);
	socketName += "/";
	socketName += name;

	struct passwd *pwd = getpwnam(user);

	SeyconCommon::debug ("Creating socket %s for user %s", socketName.c_str(), user );
	// Crear socket
	struct sockaddr_un local;
	int s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s >= 0) {
		local.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
		strcpy(local.sun_path, socketName.c_str());
		unlink(local.sun_path);
		int len = strlen(local.sun_path) + sizeof(local.sun_family);


		bind(s, (struct sockaddr *)&local, len);

		SeyconCommon::debug("Bound to %s", local.sun_path);
		chown (socketName.c_str() , pwd->pw_uid, pwd->pw_gid );
		chmod (socketName.c_str(), 0600);
		SeyconCommon::debug("Adjusted permissions on %s [%d-%d]", socketName.c_str(), pwd->pw_uid, pwd->pw_gid);
		SeyconCommon::debug("Listening to %s", local.sun_path);
	}
	SeyconCommon::debug ("Created socket %d", s );
	return s;
}

void PamHandler::dialogThread () {
	listenSocket = createSocket("dlg");
	if (listenSocket >= 0) {
		listen(listenSocket, 1);
		do {
			struct sockaddr_un remote;
			socklen_t len = sizeof remote;
			int s2 = accept(listenSocket, (struct sockaddr*)&remote,  &len);
			if (s2 >= 0) {
				SeyconCommon::debug ("Accepted connection %d from %s", s2, remote.sun_path);
				if (dialogSocket != -1)
					close (dialogSocket);
				dialogSocket = s2;
			}
		} while (listenSocket >= 0);
	}
}

