/*
 * Utils.cpp
 *
 *  Created on: 27/10/2010
 *      Author: u07286
 */

#include <objbase.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <string>
#include <lm.h>
#include "Utils.h"

Utils::Utils()
{
}

Utils::~Utils()
{
}

bool Utils::getWindowText(HWND hwnd, std::string &str)
{
	int i = GetWindowTextLength(hwnd);
	if (i < 0)
	{
		str.clear();
		return false;
	}
	if (i == 0)
		str.clear();
	else
	{
		char *ach = (char*) malloc(i + 1);
		GetWindowText(hwnd, ach, i + 1);
		str.assign(ach);
		free(ach);
	}
	return true;
}

bool Utils::getDlgCtrlText(HWND hwnd, int id, std::string &str)
{
	HWND hWnd = GetDlgItem(hwnd, id);
	if (hWnd == NULL)
	{
		str.clear();
		return false;
	}
	else
		return getWindowText(hWnd, str);

}

bool Utils::getWindowText(HWND hwnd, std::wstring &str)
{
	int i = GetWindowTextLengthW(hwnd);
	if (i < 0)
	{
		str.clear();
		return false;
	}
	if (i == 0)
		str.clear();
	else
	{
		wchar_t *ach = (wchar_t*) malloc((i + 1) * sizeof(wchar_t));
		GetWindowTextW(hwnd, ach, i + 1);
		str.assign(ach);
		free(ach);
	}
	return true;
}

bool Utils::getDlgCtrlText(HWND hwnd, int id, std::wstring &str)
{
	HWND hWnd = GetDlgItem(hwnd, id);
	if (hWnd == NULL)
	{
		str.clear();
		return false;
	}
	else
		return getWindowText(hWnd, str);
}

bool Utils::convert(std::wstring & target, std::string & src)
{
	size_t size = mbstowcs(NULL, src.c_str(), 0);
	wchar_t *achPin = (wchar_t*) malloc((size + 1) * sizeof(wchar_t));
	mbstowcs(achPin, src.c_str(), size + 1);
	target.assign(achPin);
	free(achPin);
	return true;
}

bool Utils::convert(std::string & target, std::wstring & src)
{
	size_t size = wcstombs(NULL, src.c_str(), 0);
	char *achPin = (char*) malloc(size + 1);
	wcstombs(achPin, src.c_str(), size + 1);
	target.assign(achPin);
	free(achPin);
	return true;
}

bool Utils::getErrorMessage(std::string& msg, DWORD dwError)
{
	char* buffer;
	if (FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dwError, 0, (LPSTR) &buffer, 0, NULL) > 0)
	{
		msg.assign(buffer);
		free(buffer);
		return true;
	}
	else
	{
		msg.assign("Unknown error");
		return false;
	}

}

bool Utils::getLastError(std::string &msg)
{
	return getErrorMessage(msg, GetLastError());

}

bool Utils::getLastError(std::wstring &msg)
{
	return getErrorMessage(msg, GetLastError());

}

bool Utils::getErrorMessage(std::wstring &msg, DWORD dwError)
{
	wchar_t* buffer;
	if (FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, dwError, 0, (LPWSTR) &buffer, 0, NULL) > 0)
	{
		msg.assign(buffer);
		free(buffer);
		return true;
	}
	else
	{
		msg.assign(L"Unknown error");
		return false;
	}
}

extern HINSTANCE hSoffidConfigDll;

// Load messages from resources
std::string Utils::LoadResourcesString(int id)
{
	char resource[4096];	// Resource load
	std::string result;			// Message of resource (or error)

	// Check the resource availability
	if (LoadString(hSoffidConfigDll, id, (char*) &resource,
			sizeof(resource) / sizeof(char)) > 0)
	{
		result = resource;
	}

	else
	{
		char ach[20];
		sprintf(ach, " %d ", id);
		result = "! Unknown message !";
		result += ach;
	}

	return result;
}
