/*
 * HllPatternSpec.cpp
 *
 *  Created on: 25/03/2014
 *      Author: bubu
 */

#include <MazingerInternal.h>
#include "HllPatternSpec.h"
#include <string.h>
#include <stdlib.h>
#include <pcreposix.h>
#include <string>
#include <stdio.h>

HllPatternSpec::HllPatternSpec() {
}

HllPatternSpec::~HllPatternSpec() {
}

bool HllPatternSpec::matches(HllApplication& app) {
	std::string id, name, sessionType;
	int rows, cols, codepage;


	app.querySesssionStatus(id, name, sessionType, rows, cols, codepage);

	std::string content;
	if ( app.getPresentationSpace(content) != 0)
		return false;

	std::wstring wcontent = MZNC_strtowstr(content.c_str());


	for ( std::vector<HllRowPatternSpec*>::iterator it = m_patterns.begin();
			it != m_patterns.end();
			it++)
	{
		HllRowPatternSpec * row = *it;
		std::wstring rowContent;
		if (row->row != NULL)
		{
			int rowNumber = atoi(row->row);
			if (rowNumber >= 1 && rowNumber <= rows)
			{
				int first = cols * (rowNumber -1);
				rowContent = wcontent.substr(first, cols);
			}
			else
			{
				return false;
			}
		}
		if (row->rePattern != NULL)
		{
			std::string utf8RowContent  = MZNC_wstrtoutf8(rowContent.c_str());
			if (regexec (row->rePattern, utf8RowContent.c_str(), (size_t) 0, NULL, 0 ) != 0 )
			{
				MZNSendTraceMessageA("Row %s does not match %s",
						row->row, row->pattern);
				return false;
			}
		}
	}

	MZNSendDebugMessageA("Matched HLL application. session=%s",
			name.c_str());

	return true;
}
