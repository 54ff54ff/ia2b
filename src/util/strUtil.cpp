/*========================================================================\
|: [Filename] strUtil.cpp                                                :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the utilities about string manipulation          :|
<------------------------------------------------------------------------*/

#include "util.h"

namespace _54ff
{

MatchType myStrNCmp(const char* cand, const char* model, size_t mandLen)
{
	size_t i = 0;
	for(; model[i] != 0; ++i)
	{
		if(cand[i] == 0)
			return i >= mandLen ? MATCH : PARTIAL;
		if(tolower(cand[i]) != tolower(model[i]))
			return NOTMATCH;
	}
	return cand[i] == 0 ? MATCH : NOTMATCH;
}

char* getToken(char* curToken, bool* endFlag)
{
	for(; *curToken != '\0' && *curToken != ' '; ++curToken);
	if(*curToken == '\0') { if(endFlag) *endFlag = true; return curToken; }
	for(*(curToken++) = '\0'; *curToken == ' '; ++curToken);
	if(endFlag) *endFlag = false;
	return curToken;
}

PureStrList breakToTokens(char* str)
{
	PureStrList tokenList; //trust the RVO
	for(; *str != '\0'; str = getToken(str))
		tokenList.push_back(str);
	return tokenList;
}

char* findLastToken(char* str)
{
	char* prev = str;
	bool isEnd = *str == '\0';
	for(; !isEnd; str = getToken(prev=str, &isEnd));
	return prev;
}

bool isValidVarName(const char* str)
{
	//expect at least a null character in the string
	if(*str != '_' && !isalpha(*str))
		return false;
	for(++str; *str != '\0'; ++str)
		if(*str != '_' && !isalnum(*str))
			return false;
	return true;
}

}
