

#ifdef WIN32


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <shellapi.h>
#include "resource.h"

void sendMessage(LPCSTR lpszMessage) {
	HANDLE hMailSlot;
	WCHAR achUser[1024];
	WCHAR ach[1512];
	DWORD userSize = sizeof achUser;
	DWORD dwWritten;
	GetUserNameW(achUser, &userSize);
	CharLowerW(achUser);
	wsprintfW(ach, L"\\\\.\\mailslot\\MAZINGER_LAUNCHER_%s", achUser);
	hMailSlot = CreateFileW(ach, GENERIC_WRITE, FILE_SHARE_READ
			| FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	WriteFile(hMailSlot, lpszMessage, strlen(lpszMessage), &dwWritten,
			NULL);
	CloseHandle(hMailSlot);
}

extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInst2,
		LPSTR cmdLine, int nShow) {

	int numArgs = 0;
	LPCWSTR wCmdLine = GetCommandLineW();
	LPWSTR* args = CommandLineToArgvW(wCmdLine, &numArgs);
	if (numArgs > 1)

	{
		if (wcsicmp (args[1], L"-id") == 0) {
			char ach[100];
			wcstombs(ach, args[2], 99);
			sendMessage (ach);
		} else {
			char ach[MAX_PATH];
			wcstombs(ach, args[1], MAX_PATH-1);
			FILE *f = fopen(ach, "r");
			if (f != NULL)
			{
				char achMessage[50];
				int read =fread(achMessage, 1, 49, f);
				fclose (f);
				achMessage[read]='\0';
				sendMessage(achMessage);
			}
		}
	}

	return 0;
}

#else
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#include <gtk/gtk.h>
#include <MazingerInternal.h>
#include <algorithm>

void alert (const char *msg, ...) {
	va_list va;
	va_start(va, msg);
	char achBuffer[2048];
	vsnprintf(achBuffer, sizeof achBuffer, msg, va);
	va_end(va);

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Soffid ESSO", NULL, GTK_DIALOG_MODAL,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	GtkWidget *label = gtk_label_new(achBuffer);
	gtk_box_pack_start(GTK_BOX(content), label, true, true, 10);
	gtk_widget_show(label);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);

}
void readStrings (int s, std::string &type, std::string &content) {
	std::string result;
	const int max=1024;
	int size = 0;
	int read;
	char buf[max];
	while (read = recv(s, &buf[size], max-size, 0), read > 0) {
		size += read;
		int zeros = 0;
		int firstzero = -1;
		for (int i = 0; i < size; i++) {
			if (buf [i] == '\0')
			{
				zeros ++;
				if (zeros == 1) firstzero = i;
			}
		}
		if (zeros == 2) {
			type.assign( buf );
			content.assign ( &buf[firstzero + 1] );
			break;
		} else if (size >= max) {
			break;
		}
	}
}

static void execute (const char *msg) {
	struct sockaddr_un  remote;
	socklen_t len;
	unsigned int s, s2;

	std::string path = getenv ("HOME");
	path += "/.config/mazinger/launcher";

	if (path.length() >= sizeof remote.sun_path) {
		alert ("Cannot create socket %s. Length exceeds limit (%d)",
				path.c_str(), sizeof remote.sun_path);
		return;
	}

	// Crear socket
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s >= 0) {
		remote.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
		strcpy(remote.sun_path, path.c_str());
		len = strlen(remote.sun_path) + sizeof(remote.sun_family);
		if (connect (s, (struct sockaddr *)&remote, len) == 0) {
			send (s, msg, strlen(msg)+1, 0);
			std::string type;
			std::string content;
			readStrings(s, type, content);
			printf ("Type =%s\nContent=%s\n", type.c_str(), content.c_str());
			if ( type == "ERROR") {
				alert ("Error: %s", content.c_str());
			} else if (type == "URL") {
				close (s);
				if (fork () == 0) {
					execlp("xdg-open", "xdg-open", content.c_str(), NULL);
				}
			} else if (type == "MZN") {
				MZNEvaluateJS(content.c_str());
			} else {
				char ach[20];
				std::string ext = type;
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
				std::string file ;
				file = "/var/tmp/mazinger";
				file += msg;
				file += ".";
				file += ext;
				printf ("Creando archivo %s\n", file.c_str());
				FILE *f = fopen (file.c_str(), "w");
				if (f != NULL) {
					fwrite(content.c_str(), content.length(), 1, f);
					fclose (f);
					if (fork () == 0) {
						execlp("xdg-open", "xdg-open", file.c_str(), NULL);
					}
				}

			}
		} else {
			alert ("Cannot locate mazinger session manager");
		}
		close (s);
	}

}
extern "C" int main (int numArgs, char **args) {
    gtk_init(&numArgs, &args);

    if (numArgs > 1)

	{
		if (strcmp (args[1], "-id") == 0) {
			execute (args[2]);
		} else {

			FILE *f = fopen(args[1], "r");
			if (f != NULL)
			{
				char achMessage[50];
				int read =fread(achMessage, 1, 49, f);
				fclose (f);
				achMessage[read]='\0';
				execute(achMessage);
			}
		}
	}

	return 0;
}
#endif
