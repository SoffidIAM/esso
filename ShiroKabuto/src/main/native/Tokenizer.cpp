/*
 * Tokenizer.cpp
 *
 *  Created on: 24/11/2011
 *      Author: u07286
 */

#include "Tokenizer.h"

using namespace std;

//CPP file
const string Tokenizer::DELIMITERS(" \t\n\r");

Tokenizer::Tokenizer (const std::string& s)
		: m_string(s), m_offset(0), m_delimiters(DELIMITERS)
{
}

Tokenizer::Tokenizer (const std::string& s, const std::string& delimiters)
		: m_string(s), m_offset(0), m_delimiters(delimiters)
{
}

bool Tokenizer::NextToken ()
{
	return NextToken(m_delimiters);
}

bool Tokenizer::NextToken (const std::string& delimiters)
{
	size_t i = m_string.find_first_not_of(delimiters, m_offset);
	if (string::npos == i)
	{
		m_offset = m_string.length();
		return false;
	}

	size_t j = m_string.find_first_of(delimiters, i);
	if (string::npos == j)
	{
		m_token = m_string.substr(i);
		m_offset = m_string.length();
		return true;
	}

	m_token = m_string.substr(i, j - i);
	m_offset = j;
	return true;
}

const std::string Tokenizer::GetToken () const
{
	return m_token;
}
