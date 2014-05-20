/*
 * RenameExecutor.h
 *
 *  Created on: 16/09/2011
 *      Author: u07286
 */

#ifndef RENAMEEXECUTOR_H_
#define RENAMEEXECUTOR_H_

#include "Log.h"
class RenameExecutor {
public:
	RenameExecutor();
	virtual ~RenameExecutor();

	void execute();
private:
	Log log;
};

#endif /* RENAMEEXECUTOR_H_ */
