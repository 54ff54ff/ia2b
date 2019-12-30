/*========================================================================\
|: [Filename] listFile.cpp                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the utilities about printing strings             :|
<------------------------------------------------------------------------*/

#if __has_include(<filesystem>)
	#include <filesystem>
	using namespace std::filesystem;
#elif __has_include(<experimental/filesystem>)
	#include <experimental/filesystem>
	using namespace std::experimental::filesystem;
#else
	#error "No filesystem!"
#endif

#include <algorithm>
#include <iostream>
#include <iomanip>
#include "util.h"
using namespace std;

namespace _54ff
{

WrapStrList listFile(const char* dir, const char* prefix)
{
	WrapStrList fileList; //trust the RVO
	if(!is_directory(dir)) return fileList;
	const size_t cmpLen = strlen(prefix);
	for(const directory_entry& de: directory_iterator(dir))
	{
		//letting this string be a reference will introduce strange bug
		const string fileName = de.path().filename().native();
		if(strncmp(prefix, fileName.c_str(), cmpLen) == 0)
		{
			if(is_directory(de.path())) fileList.emplace_back(copyCStr(fileName + "/"), false);
			else                        fileList.emplace_back(copyCStr(fileName), false);
		}
	}

	if(cmpLen != 0)
	{
		if(strncmp(prefix, ".",  cmpLen) == 0) fileList.emplace_back( "./");
		if(strncmp(prefix, "..", cmpLen) == 0) fileList.emplace_back("../");
	}

	sort(fileList.begin(), fileList.end());
	return fileList;
}

void printStrsByOrder(const size_t winWidth, const WrapStrList& strList)
{
	Array<size_t> strLen(strList.size());
	for(size_t i = 0; i < strList.size(); ++i)
		strLen[i] = strlen(strList[i]);

	vector<size_t> paddings;
	constexpr size_t twoSpaceLen = 2;
	size_t row = 1;
	for(; row < strList.size(); ++row)
	{
		paddings.clear();
		size_t curWidth = 0;
		size_t i = 0;
		bool answer = true;
		while(answer && i < strList.size())
		{
			if(i != 0) curWidth += twoSpaceLen;
			size_t max = 0;
			size_t num = strList.size() - i;
			if(num > row) num = row;
			for(; num > 0; --num, ++i)
				if(max < strLen[i])
					max = strLen[i];
			curWidth += max;
			paddings.emplace_back(max);
			if(curWidth > winWidth) answer = false;
		}
		if(answer) break;
	}

	const char* twoSpace = "  ";
	cout << left;
	for(size_t r = 0; r < row; ++r)
	{
		for(size_t i = r, j = 0; i < strList.size(); i += row, ++j)
		{
			if(j != 0) cout << twoSpace;
			cout << setw(paddings[j]) << setfill(' ') << strList[i];
		}
		cout << endl;
	}
	cout << endl;
}

string findCommonPart(size_t prefixLen, const WrapStrList& strList)
{
	string commonPart; //trust the RVO
	for(; strList[0][prefixLen] != '\0'; ++prefixLen)
	{
		for(size_t i = 1; i < strList.size(); ++i)
			if(strList[i][prefixLen] == '\0' ||
			   strList[0][prefixLen] != strList[i][prefixLen])
				return commonPart;
		commonPart += strList[0][prefixLen];
	}
	return commonPart;
}

void printWithPadding(const size_t winWidth, const size_t padding, const char* str)
{
	//There should be only printable and newline in str
	//winWidth > padding
	//implicitly assume these
	for(size_t i = 0, col = padding; str[i] != '\0'; ++i)
	{
		auto changeLine = [&]
			{ cout << endl; for(col = 0; col < padding; ++col) cout << ' '; };
		
		if(str[i] != '\n')
			{ if(col == winWidth) changeLine(); cout << str[i]; col += 1; }
		else
			{ if(str[i+1] != '\0') changeLine(); else cout << endl; }
	}
}

}
