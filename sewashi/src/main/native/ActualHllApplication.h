/*
 * ActualHllApplication.h
 *
 *  Created on: 26/03/2014
 *      Author: bubu
 */

#ifndef ACTUALHLLAPPLICATION_H_
#define ACTUALHLLAPPLICATION_H_

#include <HllApplication.h>
#include "hllapi.h"

class ActualHllApplication : public HllApplication {
public:
	ActualHllApplication(HllApi *api, char sessionId);
	virtual ~ActualHllApplication();

	virtual int querySesssionStatus (std::string &id, std::string& name, std::string& sessionType, int &rows, int &cols, int &codepage ) ;
	virtual int getPresentationSpace (std::string &content ) ;

	virtual int sendKeys (const char *szString) ;
	virtual int sendText (const char *szString) ;
	virtual int getCursorLocation (int &row, int &column) ;
	virtual int setCursorLocation (int row, int column);

private:
	char session;
	HllApi *api;
};

#endif /* ACTUALHLLAPPLICATION_H_ */
