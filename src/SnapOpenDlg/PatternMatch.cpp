#include "PatternMatch.h"
#include <tchar.h>

BOOL patternMatch(LPCTSTR text, LPCTSTR pattern)
{
	LPCTSTR nextPattern, match;
	TCHAR subPattern[MAX_PATH];

	while (TRUE)
	{
		while (*pattern == ' ')
		{
			pattern++;
		}
		if (*pattern == 0)
		{
			return TRUE;
		}
		nextPattern = _tcschr(pattern, ' ');
		if (!nextPattern)
		{
			nextPattern = pattern + _tcslen(pattern);
		}
		_tcsncpy(subPattern, pattern, nextPattern - pattern);
		subPattern[nextPattern - pattern] = 0;
		match = _tcsstr(text, subPattern);
		if (!match)
		{
			return FALSE;
		}
		text = match + (nextPattern - pattern);
		pattern = nextPattern;
	}

	return TRUE;
}
