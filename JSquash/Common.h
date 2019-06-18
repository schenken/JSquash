#pragma once

#include <fstream>
#include <vector>
#include <algorithm>
#include <locale>
#include <iostream>
#include <random>
#include <signal.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <map>
#include <iostream>
#include <codecvt>
#include <sstream>

size_t LoadTextFileIntoVector(const std::wstring filename, std::vector<std::wstring>& v);
int WriteVectorToTextFile (const std::wstring filename, const std::vector<std::wstring> v);
int SubstituteTextInVector (std::vector<std::wstring>& v, const std::wstring oldText, const std::wstring newText);
void Replace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr);

size_t LoadTextFileIntoMap (const std::wstring filename, std::map<std::wstring, int>& m);

std::vector<std::wstring> Split (const std::wstring& line, const std::wstring& dlim);

// Remove Whitespace Functor.
// 
// Options (Mode enum) to strip leading, trailing, all, or condense blocks of ws to just a single space.
//
// Usage:
//
//	RemoveWhitespace rmws( L"  Andrew Kendall  " );
//
//	wstring z1 = rmws();	// gives "Andrew Kendall" - strips leading & trailing.
//
//	wstring s1 = rmws(static_cast<RemoveWhitespace::Mode>
//		(RemoveWhitespace::Leading | RemoveWhitespace::Trailing));	// also gives "Andrew Kendall"
//
//	wstring s1 = rmws(static_cast<RemoveWhitespace::Mode>(RemoveWhitespace::All));	// gives "AndrewKendall"
//
struct RemoveWhitespace
{
	std::wstring s;
	const WCHAR* pChar;
	int sSize;

	enum Mode { Leading = 1, Trailing = 2, All = 4, Condense = 8, LeadingTrailingCondense = 11 };

	RemoveWhitespace (const std::wstring s);

	// Set new string content.
	void operator= (const std::wstring _s);

	std::wstring operator() (Mode mode = Mode::All);

	// Set new string, process and return immediately.
	std::wstring operator() (const std::wstring _s, Mode mode = Mode::All);
};

//-----------------------------------------------------------------------------

std::wstring MyGetFilename (std::wstring filename);
