/*
 * Encoder.h
 *
 *  Created on: 14/12/2014
 *      Author: gbuades
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include <string>

namespace json {

class Encoder {
public:
	Encoder();
	virtual ~Encoder();
	static std::string encode (const char *str);
	static std::string decode (const char *str);
};

} /* namespace json */

#endif /* ENCODER_H_ */
