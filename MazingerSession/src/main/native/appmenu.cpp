/*
 * appmenu.cpp
 *
 *  Created on: 09/12/2010
 *      Author: u07286
 */

#ifdef WIN32

#include <windows.h>
#include <winsock.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlguid.h>
#include <dirent.h>

#else

#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#endif

#include <string.h>
#include <stdio.h>

#ifdef __GNUC__
#include <dirent.h>
#endif
#include <ssoclient.h>
#include "httpHandler.h"
#include "mazinger.h"

#include <MazingerInternal.h>

// #define DEBUG printf ("Trace: %s:%d\n", __FILE__, __LINE__ );
#define DEBUG

#ifdef WIN32
static std::string strMazingerDir;
static std::string strLauncher;

static const char* getMazingerDir ()
{
	if (strMazingerDir.size() == 0)
	{
		TCHAR szPath[MAX_PATH];

		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE,
						NULL, 0, szPath)))
		{
			strMazingerDir = szPath;
			strMazingerDir += "\\SoffidESSO";
		}
		else
		{
			strMazingerDir = "C:\\Program Files\\SoffidESSO";
		}
	}
	return strMazingerDir.c_str();
}

static const char* getLauncherFile ()
{
	if (strLauncher.size() == 0)
	{
		strLauncher = getMazingerDir();
		strLauncher += "\\jetscrander.exe";
	}
	return strLauncher.c_str();
}
#else
static const char* getLauncherFile ()
{
	return "/usr/bin/jetscrander";
}

#endif

#ifdef WIN32

#ifdef __GNUC__
static
void RecursiveRemoveDirectory (const char* achName)
{
	DIR *d = opendir(achName);
	if (d != NULL)
	{
		struct dirent *entry = readdir(d);
		while (entry != NULL)
		{
			if (strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name) != 0)
			{
				char achFullName[4096];
				strcpy(achFullName, achName);
				strcat(achFullName, "\\");
				strcat(achFullName, entry->d_name);
				RecursiveRemoveDirectory(achFullName);
			}
			entry = readdir(d);
		}

		//MessageBox(NULL, achName, "Deleteing dir...", MB_OK);
		rmdir(achName);
	}
	else
		unlink(achName);
}
#else
static
void RecursiveRemoveDirectory (LPCSTR achName)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char *achPattern = (char*) malloc (strlen(achName)+10);
	strcpy (achPattern, achName);
	strcat (achPattern, "\\*");

	hFind = FindFirstFile(achName, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DeleteFile (achName);
	}
	else
	{
		do
		{
			if (strcmp(".", FindFileData.cFileName) != 0 &&
					strcmp ("..", FindFileData.cFileName) != 0)
			{
				char achFullName[4096];
				strcpy (achFullName, achName);
				strcat (achFullName, "\\");
				strcat (achFullName, FindFileData.cFileName);
				RecursiveRemoveDirectory (achFullName);
			}
		}while (FindNextFile (hFind, &FindFileData));
		FindClose(hFind);
		RemoveDirectory(achName);
	}
	free (achPattern);
}

#endif

bool FileExists (const char *fileName)
{
	if ((0xFFFFFFFF == GetFileAttributes(fileName))
			&& (GetLastError() == ERROR_FILE_NOT_FOUND))
		return false;

	return true;
}

std::string iconLocation (std::string appid)
{
	std::string location;				// Icon image installation path
	std::string iconFileName;	// Icon image name
	TCHAR achDir[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
					NULL, 0, achDir)))
	{
		location.assign(achDir);
		location.append("\\SoffidESSO\\icons\\");
		iconFileName = location + appid + ".ico";

		// Check previous existing file
		if (!FileExists(iconFileName.c_str()))
		{
			CreateDirectory(location.c_str(), NULL);

			SeyconService service;
			SeyconResponse *response = service.sendUrlMessage(
					L"/getapplicationicon?appID=%hs", appid.c_str());

			if (response != NULL)
			{
				std::string status = response->getToken(0);
				if (status == "OK")
				{
					std::string image = response->getToken(1);

					std::string binary = SeyconCommon::fromBase64(image.c_str());
					FILE *f = fopen(iconFileName.c_str(), "wb+");
					fwrite(binary.c_str(), binary.length(), 1, f);
					fclose(f);
				}
			}
		}
	}
	return iconFileName;
}

static
int parseAndStoreApp (SeyconResponse *response, int &pos, const char* dir)
{
	std::string id = response->getToken(pos);
	std::string name = response->getToken(pos + 1);
	std::string fileName;

	fileName.assign(dir);
	fileName.append("\\");
	fileName.append(name.c_str());

	if (id == "MENU")
	{
		RecursiveRemoveDirectory(fileName.c_str());
		CreateDirectory(fileName.c_str(), NULL);
		pos += 2;
		int children = 0;
		id = response->getToken(pos);
		while (id != "ENDMENU")
		{
			children += parseAndStoreApp(response, pos, fileName.c_str());
			id = response->getToken(pos);
		}
		pos++;
		if (children == 0)
		{
			RecursiveRemoveDirectory(fileName.c_str());
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		std::string cmdLine = "-id ";
		cmdLine += id;
//		SeyconCommon::debug ("Created launcher %s\n", fileName.c_str());
		IShellLinkA* pShellLink;

		HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLinkA,
				(void**) &pShellLink);
		if (FAILED(hr))
		{
			SeyconCommon::warn("Unable to create shell link\n");
			return 0;
		}

		pShellLink->SetPath(getLauncherFile()); // Path to the object we are referring to
		pShellLink->SetArguments(cmdLine.c_str()); // Path to the object we are referring to
		pShellLink->SetDescription(name.c_str());

		std::string location = iconLocation(id);
		pShellLink->SetIconLocation(location.c_str(), 0);
//		pShellLink->SetIconLocation(getLauncherFile(), 1);

		IPersistFile *pPersistFile;

		pShellLink->QueryInterface(IID_IPersistFile, (void**) &pPersistFile);

		std::wstring wfullName;
		std::wstring wsFileName = MZNC_strtowstr(fileName.c_str());
		wsFileName += L".lnk";

		pPersistFile->Save(wsFileName.c_str(), TRUE);
		pPersistFile->Release();
		pShellLink->Release();

		pos += 3;
		return 1;
	}
}

static HANDLE hCurrentMailSlot = NULL;
static HANDLE createMailSlot ()
{
	if (hCurrentMailSlot != NULL)
		CloseHandle(hCurrentMailSlot);
	WCHAR achUser[10024];
	WCHAR ach[1512];
	DWORD userSize = sizeof achUser;
	GetUserNameW(achUser, &userSize);
	CharLowerW(achUser);
	wsprintfW(ach, L"\\\\.\\mailslot\\MAZINGER_LAUNCHER_%s", achUser);
	hCurrentMailSlot = CreateMailslotW(ach, 10024, MAILSLOT_WAIT_FOREVER, NULL);
	return hCurrentMailSlot;
}

static void doLaunch (const char *lpszId, SeyconSession *session)
{

	SeyconService service;
	std::wstring id = service.escapeString(lpszId);
	SeyconResponse *response = service.sendUrlMessage(
			L"/getapplication?user=%hs&key=%hs&id=%ls", session->getSoffidUser(),
			session->getSessionKey(), id.c_str());
	if (response != NULL)
	{
		std::string status = response->getToken(0);
		if (status == "OK")
		{
			std::string type = response->getToken(1);
			std::string content = response->getTail(2);
			if (type == "URL")
			{
				ShellExecuteA(NULL, "open", content.c_str(), NULL, NULL, SW_SHOW);
			}
			else if (type == "MZN")
			{
				MZNEvaluateJS(content.c_str());
			}
			else
			{
				char achNumber[20];
				sprintf(achNumber, "%d", (int) GetCurrentProcessId());

				std::string file;
				file = getenv("TMP");
				file += "\\";
				file += achNumber;
				file += ".";
				file += lpszId;
				file += ".";
				file += type;
				FILE *f = fopen(file.c_str(), "w");
				if (f != NULL)
				{
					fwrite(content.c_str(), content.size(), 1, f);
					ShellExecuteA(NULL, "open", file.c_str(), NULL, NULL, SW_SHOW);
					fclose(f);
				}
			}
		}
		else
		{
			std::string msg2 = "Unable to start application:\n";
			msg2 += response->getToken(1);
			MessageBox(NULL, msg2.c_str(), "Notice", MB_OK);
		}
		delete response;
	}
	else
	{
		MessageBox(NULL, "Cannot access to network", "Notice", MB_OK);
	}
}

static void LeerMailSlot (SeyconSession *session)
{
	DWORD cbRead;
	CHAR achMessage[10024];

	HANDLE hSlot = createMailSlot();

	while (hSlot != NULL)
	{
		cbRead = 0;
		ReadFile(hSlot, achMessage, sizeof achMessage, &cbRead, (LPOVERLAPPED) NULL);
		if (cbRead > 0)
		{
			achMessage[cbRead] = L'\0';
			SeyconCommon::debug("Launching %s command\n", achMessage);
			doLaunch(achMessage, session);
		}
		else
			return;
	}
}

static DWORD WINAPI doGenerateMenus (LPVOID lpv)
{
	SeyconSession * session = (SeyconSession*) lpv;
	const char *szSessionId = session->getSessionKey();

	DEBUG
	CoInitialize(NULL);
	DEBUG

	TCHAR achDir[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAMS | CSIDL_FLAG_CREATE,
					NULL, 0, achDir)))
	{
		DEBUG
		SeyconService service;
		DEBUG
		SeyconResponse *response = service.sendUrlMessage(
				L"/getapplications?user=%hs&key=%hs", session->getSoffidUser(),
				szSessionId);
		if (response != NULL)
		{
			DEBUG
			std::string status = response->getToken(0);
			if (status == "OK")
			{
				DEBUG
				int pos = 1;
				parseAndStoreApp(response, pos, achDir);
			DEBUG
		}
		delete response;
	}
}

DEBUG
LeerMailSlot(session);
DEBUG
return 0;
}

void SeyconSession::generateMenus ()
{
DWORD dw;
CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) doGenerateMenus, (LPVOID) this, 0, &dw);
}

#else

int parseAndStoreApp (SeyconResponse *response, int &pos, struct passwd *pwd, FILE *fMerge, int indent)
{
std::string prefix;
for (int i = 0; i <= indent; i++)
prefix += "\t";

std::string id = response->getUtf8Token(pos);
std::string name = response->getUtf8Token(pos+1);

std::string fileName = pwd->pw_dir;
fileName.append ("/.local");
mkdir (fileName.c_str(), 0755);
chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
fileName.append ("/share");
mkdir (fileName.c_str(), 0755);
chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
if (id == "MENU")
{
	char achPos[20];
	sprintf (achPos, "%d", pos);
	fileName.append ("/desktop-directories");
	mkdir (fileName.c_str(), 0755);
	chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);

	SeyconCommon::debug("Creating dir %s", name.c_str());
// Crear archivo .menu
	std::string shortName = "mazinger-";
	shortName.append (achPos);
	shortName.append (".menu");
	fileName.append ("/");
	fileName.append (shortName);

	FILE *f = fopen (fileName.c_str(), "w+");
	if (f != NULL)
	{
		chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
		fprintf (f, "#!/usr/bin/env xdg-open\n"
				"[Desktop Entry]\n"
				"Version=1.0\n"
				"Type=Directory\n"
				"Name=%s\n"
				"Icon=folder\n", name.c_str());
		fclose (f);
		chmod(fileName.c_str(), 0755);
	}
// Anotar menu en el merge
	fprintf (fMerge, "%s<Menu>\n"
			"%s<Name>%s</Name>\n"
			"%s<Directory>%s</Directory>\n"
			"%s<Layout>\n"
			"%s\t<Merge type=\"menus\"/>\n"
			"%s\t<Merge type=\"files\"/>\n"
			"%s</Layout>\n"
			"%s<DefaultLayout inline=\"false\"/>\n",
			prefix.c_str(),prefix.c_str(),name.c_str(),prefix.c_str(),shortName.c_str(),
			prefix.c_str(),prefix.c_str(),prefix.c_str(),prefix.c_str(),prefix.c_str());

	pos += 2;
	int children = 0;
	id = response->getUtf8Token(pos);
	while (id != "ENDMENU")
	{
		children += parseAndStoreApp(response, pos, pwd, fMerge, indent+1);
		id = response->getUtf8Token(pos);
	}
	pos ++;
	fprintf (fMerge, "%s</Menu>\n",prefix.c_str());
	return 1;
}
else
{
	fileName.append ("/applications");
	mkdir (fileName.c_str(), 0755);
	chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);

// Crear archivo .menu
	std::string shortName ="mazinger-";
	shortName += id;
	shortName += ".desktop";
	fileName.append ("/");
	fileName.append (shortName);
	FILE *f = fopen (fileName.c_str(), "w+");
	if (f != NULL)
	{
		chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
		fprintf (f, "#!/usr/bin/env xdg-open\n\n"
				"[Desktop Entry]\n"
				"Version=1.0\n"
				"Type=Application\n"
				"Terminal=false\n"
				"Name=%s\n"
				"Exec=/usr/bin/jetscrander -id %s\n"
				"Icon=gnome-panel-launcher\n", name.c_str(), id.c_str());
		fclose (f);
		chmod(fileName.c_str(), 0755);
	}
// Anotar menu en el merge
	fprintf (fMerge, "%s<Include>\n"
			"%s\t<Filename>%s</Filename>\n"
			"%s</Include>\n",
			prefix.c_str(),prefix.c_str(),shortName.c_str(),prefix.c_str());
	pos += 3;
	return 1;
}
}

static void launch (SeyconSession *session, int s, const char *id )
{
SeyconService service;
std::wstring wid = service.escapeString (id);
service.resetServerStatus();
SeyconResponse *response = service.sendUrlMessage(L"/getapplication?user=%hs&key=%hs&id=%ls",
		session->getSoffidUser(),
		session->getSessionKey(),
		wid.c_str());
if (response != NULL)
{
	std::string status = response->getToken(0);
	if (status == "OK")
	{
		std::string type = response->getToken(1);
		std::string content = response->getUtf8Tail(2);
		send (s, type.c_str(), type.length()+1, 0);
		send (s, content.c_str(), content.length()+1, 0);
	}
	else
	{
		send (s, "ERROR", 6, 0);
		std::string msg = response->getUtf8Token(1);
		send (s, msg.c_str(), msg.length()+1, 0);
	}
	delete response;
}
else
{
	send (s, "ERROR", 6, 0);
	const char *msg = "Unable to access to network";
	send (s, msg, strlen(msg)+1, 0);
}
}

static void listenRequests (SeyconSession *session, struct passwd *pwd)
{
struct sockaddr_un local, remote;
socklen_t len;
unsigned int s, s2;

std::string socketName = "/tmp/sayaka-";
socketName += session->getSessionId();
mkdir (socketName.c_str(), 0700);
chown(socketName.c_str(), pwd->pw_uid, pwd->pw_gid);
chmod(socketName.c_str(), 0700);

socketName += "/app";

if (socketName.length() >= sizeof local.sun_path)
{
	SeyconCommon::warn("Cannot create socket %s. Length exceeds limit (%d)",
			socketName.c_str(), sizeof local.sun_path);
	return;
}
// Crear socket
s = socket(AF_UNIX, SOCK_STREAM, 0);
if (s >= 0)
{
	local.sun_family = AF_UNIX; /* local is declared before socket() ^ */
	strcpy(local.sun_path, socketName.c_str());
	unlink(local.sun_path);

	len = strlen(local.sun_path) + sizeof(local.sun_family);
	bind(s, (struct sockaddr *)&local, len);

	chown(socketName.c_str(), pwd->pw_uid, pwd->pw_gid);
	chmod(socketName.c_str(), 0700);

	SeyconCommon::debug("Bound to %s", local.sun_path);
	chown (socketName.c_str(), pwd->pw_uid, pwd->pw_gid );
	chmod (socketName.c_str(), 0600);
	SeyconCommon::debug("Adjusted permissions on %s", socketName.c_str());

	listen(s, 5);
	SeyconCommon::debug("Listening to %s", local.sun_path);
	do
	{
		len = sizeof remote;
		s2 = accept(s, (struct sockaddr*)&remote, &len);
		SeyconCommon::debug ("Accepted connection from %s", remote.sun_path);
		const int max=1024;
		int size = 0;
		int read;
		char buf[max];
		while (read = recv(s2, &buf[size], max-size, 0), read > 0)
		{
			SeyconCommon::debug ("Received %d bytes", read);
			size += read;
			buf[size] = '\0';
			SeyconCommon::debug ("Received text: %s", buf);
			if (buf[size-1] == '\0')
			{
				SeyconCommon::debug ("Launching");
				launch (session, s2, buf);
				break;
			}
			else if (size >= max)
			{
				break;
			}
		}
		close (s2);
	}while (true);
}
}

struct gmtd
{
SeyconSession *session;
SeyconResponse *response;
};

static void* doGenerateMenus (void * lpv)
{
struct gmtd* gmtdi = reinterpret_cast<struct gmtd*>(lpv);
SeyconSession *session = gmtdi->session;
SeyconResponse *response = gmtdi->response;

struct passwd *pwd = getpwnam (session->getUser());
if (pwd != NULL)
{
	if (response != NULL)
	{
		std::string status = response->getUtf8Token(0);
		SeyconCommon::debug("Menus for %s", status.c_str());
		if (status == "OK")
		{
			std::string fileName = pwd->pw_dir;
			fileName += "/.config";
			mkdir (fileName.c_str(), 0755);
			chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
			fileName += "/menus";
			mkdir (fileName.c_str(), 0755);
			chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
			fileName += "/applications-merged";
			mkdir (fileName.c_str(), 0755);
			chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
			fileName += "/mazinger.menu";
			SeyconCommon::debug("Generating menus on %s", fileName.c_str());
			FILE *f = fopen (fileName.c_str(), "w");
			if (f == NULL)
			{
				SeyconCommon::warn ("Cannot create %s file\n", fileName.c_str());
			}
			else
			{
				chown (fileName.c_str(), pwd->pw_uid, pwd->pw_gid);
				const char *header = "<!DOCTYPE Menu PUBLIC '-//freedesktop//DTD Menu 1.0//EN' "
				"'http://standards.freedesktop.org/menu-spec/menu-1.0.dtd'>\n"
				"<Menu>\n"
				"\t<Name>Applications</Name>\n"
				"\t<MergeFile type=\"parent\">/etc/xdg/menus/applications.menu</MergeFile>\n"
				"\t<DefaultLayout inline=\"false\"/>\n";
				fwrite(header, strlen(header), 1, f);
				int pos = 1;
				parseAndStoreApp(response, pos, pwd, f, 1);
				const char *footer ="</Menu>\n";
				fwrite(footer, strlen(footer), 1, f);
				fclose (f);
			}
		}
	}
	delete response;
	listenRequests(session, pwd);
}

delete gmtdi;
return 0;
}

void SeyconSession::generateMenus ()
{
SeyconCommon::debug("Getting menus for %s", getSoffidUser());
SeyconService service;
SeyconResponse *response = service.sendUrlMessage (L"/getapplications?user=%hs&key=%hs",
		getSoffidUser(),
		getSessionKey());

struct gmtd *gmtdi = new struct gmtd;

gmtdi -> session = this;
gmtdi -> response = response;

pthread_t thread1;
pthread_create( &thread1, NULL, doGenerateMenus, (void*) gmtdi);
}

#endif
