#include "sayaka.h"
#include "nossori.h"
#include <ssoclient.h>
#include <SeyconServer.h>
#include <wchar.h>
#include <ctype.h>
#include <string>
#include <MZNcompat.h>
#include <stdio.h>
#include "winwlx.h"

#include "Utils.h"
#include "nossoriDialog.h"




static int hexparse (wchar_t ch)
{
	if (ch >= L'0' && ch <= L'9')
		return ch - L'0';
	else if (ch >= L'a' && ch <= L'f')
		return ch - L'a' + 10;
	else if (ch >= L'A' && ch <= L'F')
		return ch - L'A' + 10;
	else
		return -1;
}

static std::wstring unscape (std::wstring s)
{
	std::string result;
	int i = 0;
	while ( i < s.length())
	{
		wchar_t wch = s[i++];
		if (wch == L'+')
			result += ' ';
		else if (wch == L'%' && i+1 < s.length())
		{
			int x = hexparse (s[i++]) * 16 + hexparse (s[i++]);
			result += (char) x;
		}
		else
			result += (char) wch;
	}
	return MZNC_utf8towstr(result.c_str());
}


bool NossoriHandler::performRequest() {
	SeyconService service;

	std::string d;
	SeyconCommon::updateConfig("SSOSoffidAgent");
    SeyconCommon::readProperty("SSOSoffidAgent", d);
	SeyconResponse* response;
    response = service.sendUrlMessage(L"/rememberPasswordServlet?action=requestChallenge&user=%ls&domain=%ls",
    		service.escapeString(dialog.user.c_str()).c_str(),
    		service.escapeString(d.c_str()).c_str());

    if (response != NULL)
    {
    	std::string status = response->getToken(0);
    	if (status == "OK")
    	{
    		domain = MZNC_strtowstr(d.c_str());
    		dialog.m_answers.clear();
    		dialog.m_questions.clear();
    		response->getToken(1, requestId);
    		int i = 2;
			std::wstring q;
    		while (response->getToken(i++, q))
			{
				dialog.m_questions.insert(dialog.m_questions.end(), unscape(q));
			}
        	delete response;
    		return true;
    	} else {
    		std::wstring msg1;
    		std::wstring msg2;
    		response->getToken(0, msg1);
    		response->getToken(1, msg2);
    		msg1 += L":" ;
    		msg1 += msg2;
    		pWinLogon->WlxMessageBox (hWlx, NULL, msg2.c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
        	delete response;
    		return false;
    	}
    } else {
    	pWinLogon->WlxMessageBox (hWlx, NULL, Utils::LoadResourcesWString(2005).c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
		return false;
    }
}

bool NossoriHandler::responseChallenge() {
	SeyconService service;

	std::wstring msg = std::wstring(L"/rememberPasswordServlet?action=responseChallenge&user=")+
			service.escapeString(dialog.user.c_str()) + L"&domain" +
			service.escapeString(domain.c_str()) + L"&id=" +
			service.escapeString(requestId.c_str());

	for (int i = 0; i < dialog.m_questions.size(); i++)
	{
		msg += L"&";
		msg += service.escapeString(dialog.m_questions[i].c_str());
		msg += L"=";
		msg += service.escapeString(dialog.m_answers[i].c_str());
	}
	SeyconResponse* response;
    response = service.sendUrlMessage(msg.c_str());

    if (response != NULL)
    {
    	std::string status = response->getToken(0);
    	if (status == "OK")
    	{
        	delete response;
    		return true;
    	} else if (status == "FAILED") {
    		pWinLogon->WlxMessageBox (hWlx, NULL, Utils::LoadResourcesWString(2006).c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
        	delete response;
        	return false;
    	} else {
    		std::wstring msg1, msg2;
    		response->getToken(0, msg1);
    		response->getToken(1, msg2);
    		msg1 += L":" ;
    		msg1 += msg2;
    		pWinLogon->WlxMessageBox (hWlx, NULL, msg2.c_str(),  L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
        	delete response;
    		return false;
    	}
    } else {
    	pWinLogon->WlxMessageBox (hWlx, NULL, Utils::LoadResourcesWString(2005).c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
		return false;
    }
}

bool NossoriHandler::resetPassword() {
	SeyconService service;

	SeyconResponse* response;
    response = service.sendUrlMessage(L"/rememberPasswordServlet?action=resetPassword&user=%ls&domain=%s&password=%ls&id=%ls",
    		service.escapeString(dialog.user.c_str()).c_str(), domain.c_str(),
    		service.escapeString(dialog.password.c_str()).c_str(),
    		requestId.c_str());

    if (response != NULL)
    {
    	std::string status = response->getToken(0);
    	if (status == "OK")
    	{
    		dialog.m_answers.clear();
    		dialog.m_questions.clear();
        	delete response;
    		return true;
    	} else if (status == "BADPASSWORD")
       	{
    		std::wstring msg2;
    		response->getToken(1, msg2);
    		pWinLogon->WlxMessageBox (hWlx, NULL, msg2.c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);

        	delete response;
    		return false;
    	} else {
    		std::wstring msg1;
    		std::wstring msg2;
    		response->getToken(0, msg1);
    		response->getToken(1, msg2);
    		msg1 += L":" ;
    		msg1 += msg2;
    		pWinLogon->WlxMessageBox (hWlx, NULL,msg1.c_str(),  L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
        	delete response;
    		return false;
    	}
    } else {
    	pWinLogon->WlxMessageBox (hWlx, NULL, Utils::LoadResourcesWString(2005).c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
		return false;
    }
}


bool NossoriHandler::perform()
{
    SeyconCommon::updateConfig("addon.retrieve-password.right_number");
    std::string v;
    if (!SeyconCommon::readProperty("addon.retrieve-password.right_number", v) ||
    		v.empty())
    {
    	pWinLogon->WlxMessageBox (hWlx, NULL, Utils::LoadResourcesWString(2008).c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
    	return false;
    }

    do
	{
		if (dialog.showUser() != IDOK)
			return false;
	} while (! performRequest());

	do
	{
		for (dialog.questionNumber = 0 ; dialog.questionNumber < dialog.m_questions.size(); dialog.questionNumber++)
		{
			if (dialog.showQuestion() == IDCANCEL)
				return false;
		}
	} while (! responseChallenge());


	do
	{
		if (dialog.showPassword()  == IDCANCEL)
			return false;
	} while (! resetPassword());

	pWinLogon->WlxMessageBox (hWlx, NULL, Utils::LoadResourcesWString(2007).c_str(), L"Soffid SSO", MB_ICONEXCLAMATION|MB_OK);
	return true;
}
