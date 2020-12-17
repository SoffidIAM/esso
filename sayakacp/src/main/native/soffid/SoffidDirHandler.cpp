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

SoffidDirHandler::SoffidDirHandler():
	m_log("SoffidDirHandler")
{

}

SoffidDirHandler::~SoffidDirHandler() {
}

SOCKET SoffidDirHandler::connect() {
	std::string port;
	int portNumber = 0;
	SeyconCommon::readProperty("createUserSocket", port);
	sscanf(port.c_str(), " %d", &portNumber);
	if (portNumber <= 0) {
		SeyconCommon::warn("Shirokabute service is not registered");
		return SOCKET_ERROR;
	}

	m_log.info("Connecting to %s", port.c_str());

	SOCKADDR_IN addrin;
	SOCKET socket_in = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_log.info("Connected to %s", port.c_str());
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

void SoffidDirHandler::validate (std::wstring &szUser, std::wstring &szPassword) {
	wchar_t wchComputerName [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW (wchComputerName, &dwSize);

	valid = error = mustChange = false;
	szHostName.assign (wchComputerName);
	this->szUser = szUser;
	this->szPassword = szPassword;
	this->szOldPassword = szPassword;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);

	SOCKET s = connect();
	if (s == SOCKET_ERROR) {
		error = true;
		return;
	}

	std::wstring cmd = L"VALIDATE ";
	cmd += szUser;
	cmd += L" ";
	cmd += MZNC_strtowstr(SeyconCommon::urlEncode(szPassword.c_str()).c_str());

	writeLine(s, cmd);
	std::wstring response;
	response = readLine(s);

	closesocket(s);

	if (response.substr(0, 7) == L"EXPIRED") {
		mustChange = true;
		valid = true;
	}
	else if (response.substr(0, 6) == L"DENIED") {
		valid = false;
	}
	else if (response.substr(0, 7) == L"SUCCESS") {
		storeCredentials();
		valid = true;
	}
	else {
		error = true;
	}
}

void SoffidDirHandler::validate (std::wstring &szUser, std::wstring &szPassword, std::wstring &szNewPassword) {
	wchar_t wchComputerName [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerNameW (wchComputerName, &dwSize);

	valid = error = mustChange = false;
	szHostName.assign (wchComputerName);
	this->szUser = szUser;
	this->szPassword = szNewPassword;
	this->szOldPassword = szPassword;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);

	SOCKET s = connect();
	if (s == SOCKET_ERROR) {
		error = true;
		return;
	}

	std::wstring cmd = L"VALIDATE ";
	cmd += szUser;
	cmd += L" ";
	cmd += MZNC_strtowstr(SeyconCommon::urlEncode(szPassword.c_str()).c_str());
	cmd += L" ";
	cmd += MZNC_strtowstr(SeyconCommon::urlEncode(szNewPassword.c_str()).c_str());

	writeLine(s, cmd);
	std::wstring response;
	response = readLine(s);
	closesocket(s);

	if (response.substr(0, 7) == L"EXPIRED") {
		mustChange = true;
		valid = true;
	}
	else if (response.substr(0, 6) == L"DENIED") {
		valid = false;
	}
	else if (response.substr(0, 7) == L"SUCCESS") {
		storeCredentials();
		valid = true;
	}
	else {
		error = true;
	}
}

void SoffidDirHandler::writeLine(SOCKET socket, const std::wstring &line) {
	m_log.info("Sending %ls", line.c_str());

	send(socket, (const char*) line.c_str(), line.length() * sizeof (wchar_t), 0);
	send(socket, (const char*) L"\n", sizeof (wchar_t), 0);
}

std::wstring SoffidDirHandler::readLine(SOCKET socket) {
	std::wstring s;
	wchar_t wch;
	while (recv(socket, (char*) &wch, sizeof wch, 0) >= sizeof wch) {
		if (wch == L'\n') {
			m_log.info("Received %ls", s.c_str());
			return s;
		}
		s += wch;
	}
	m_log.info("Received %ls", s.c_str());
	return s;
}

void SoffidDirHandler::storeCredentials() {
	SOCKET s = connect();
	if (s == SOCKET_ERROR) {
		error = true;
		return;
	}

	std::wstring cmd = L"STORE_CREDENTIALS ";
	cmd += szUser;
	cmd += L" ";
	cmd += MZNC_strtowstr(SeyconCommon::urlEncode(szPassword.c_str()).c_str());

	writeLine(s, cmd);
	std::wstring response;
	response = readLine(s);
	closesocket(s);
}
