/*
 * MazingerFF.h
 *
 *  Created on: 22/11/2010
 *      Author: u07286
 */

#ifndef MAZINGERFF_H_
#define MAZINGERFF_H_

#ifdef WIN32
#include <windows.h>
#define DEBUG(x)
//#define DEBUG(x) MZNSendDebugMessage("%s",x);
#else
#define DEBUG(x)
//#define DEBUG(x) printf("%s\n", x)
#endif


#endif /* MAZINGERFF_H_ */
