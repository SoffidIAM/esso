/*
 * SoffidDirHandler.cpp
 *
 *  Created on: 30/03/2011
 *      Author: u07286
 */

#include <windows.h>
#include <ctype.h>
#include <ssoclient.h>
#include <stdlib.h>
#include <MZNcompat.h>
#include "SoffidDirHandler.h"

# include "Utils.h"

SoffidDirHandler::SoffidDirHandler()
{

}

SoffidDirHandler::~SoffidDirHandler() {
}

SOCKET SoffidDirHandler::connect() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);

	std::string port;
	int portNumber = 0;
	SeyconCommon::readProperty("createUserSocket", port);
	sscanf(port.c_str(), " %d", &portNumber);
	if (portNumber <= 0) {
		SeyconCommon::warn("Shirokabuto service is not registered");
		return SOCKET_ERROR;
	}

	SOCKADDR_IN addrin;
	SOCKET socket_in = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	SeyconCommon::debug("Created socket: %d\n", (int) socket_in);
	if (socket_in == SOCKET_ERROR) {
		SeyconCommon::warn("Error creating socket");
		return SOCKET_ERROR;
	}

	addrin.sin_family = AF_INET;
	addrin.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrin.sin_port = htons(portNumber);
	if (::connect(socket_in, (LPSOCKADDR) &addrin, sizeof(addrin)) != 0) {
		SeyconCommon::warn("Error connecting to ShiroKabuto service");
		return SOCKET_ERROR;
	}
	return socket_in;
}

std::wstring SoffidDirHandler::getPassword (std::wstring &szUser) {
	std::wstring pass;
	SOCKET s = connect();
	if (s == SOCKET_ERROR) {
		return pass;
	}

	std::wstring cmd = L"GET_CREDENTIALS ";
	cmd += szUser;

	writeLine(s, cmd);
	std::wstring response;
	response = readLine(s);

	closesocket(s);

	if (response.substr(0, 3) == L"OK ") {
		pass = response.substr(3);
	}
	return pass;
}


void SoffidDirHandler::writeLine(SOCKET socket, const std::wstring &line) {
	send(socket, (const char*) line.c_str(), line.length() * sizeof (wchar_t), 0);
	send(socket, (const char*) L"\n", sizeof (wchar_t), 0);
}

std::wstring SoffidDirHandler::readLine(SOCKET socket) {
	std::wstring s;
	wchar_t wch;
	while (recv(socket, (char*) &wch, sizeof wch, 0) >= sizeof wch) {
		if (wch == L'\n') {
			return s;
		}
		s += wch;
	}
	return s;
}

