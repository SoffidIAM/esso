/*
 * HllApplication.h
 *
 *  Created on: 26/03/2014
 *      Author: bubu
 */

#ifndef ABSTACT_HLLAPPLICATION_H_
#define ABSTACT_HLLAPPLICATION_H_

#include <string>

class HllApplication {
public:
	HllApplication();
	virtual ~HllApplication();

	virtual int querySesssionStatus (std::string &id, std::string& name, std::string& sessionType, int &rows, int &cols, int &codepage ) = 0;
	virtual int getPresentationSpace (std::string &content ) = 0;

	virtual int sendKeys (const char *szString) = 0;
	virtual int sendText (const char *szString) = 0;
	virtual int getCursorLocation (int &row, int &column) = 0;
	virtual int setCursorLocation (int row, int column) = 0;
};

#endif /* ABSTACT_HLLAPPLICATION_H_ */
