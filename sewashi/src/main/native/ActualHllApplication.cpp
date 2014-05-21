/*
 * ActualHllApplication.cpp
 *
 *  Created on: 26/03/2014
 *      Author: bubu
 */

#include "ActualHllApplication.h"

ActualHllApplication::ActualHllApplication(HllApi *api, char sessionId) {
	this->session = sessionId;
	this->api = api;
}

ActualHllApplication::~ActualHllApplication() {
}

int ActualHllApplication::querySesssionStatus(std::string& id,
		std::string& name, std::string& sessionType, int& rows, int& cols,
		int& codepage) {
	return api->querySesssionStatus(session, id, name, sessionType, rows, cols, codepage);
}

int ActualHllApplication::getPresentationSpace(std::string& content) {
	return api->getPresentationSpace(session, content);
}

int ActualHllApplication::sendKeys(const char* szString) {
	return api->sendKeys(session, szString);
}

int ActualHllApplication::sendText(const char* szString) {
	std::string s;
	while (*szString)
	{
		if (*szString == '@')
			s+= '@';
		s += *szString ++;
	}
	return api->sendKeys(session, s.c_str());
}

int ActualHllApplication::getCursorLocation(int& row, int& column) {
	return api->getCursorLocation(session, row, column);
}

int ActualHllApplication::setCursorLocation(int row, int column) {
	return api->setCursorLocation(session, row, column);
}
