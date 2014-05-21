/*
 * Tokenizer.h
 *
 *  Created on: 24/11/2011
 *      Author: u07286
 */

#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include <string>

//Header file
class Tokenizer
{
	public:
		static const std::string DELIMITERS;
		Tokenizer (const std::string& str);
		Tokenizer (const std::string& str, const std::string& delimiters);
		bool NextToken ();
		bool NextToken (const std::string& delimiters);
		const std::string GetToken () const;
		void Reset ();
	protected:
		const std::string m_string;
		size_t m_offset;
		std::string m_token;
		std::string m_delimiters;
};

#endif /* TOKENIZER_H_ */
