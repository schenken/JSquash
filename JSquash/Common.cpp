#include "pch.h"
#include "Common.h"

size_t LoadTextFileIntoVector (const std::wstring filename, std::vector<std::wstring>& v)
{
    std::wifstream f;
    f.open (filename.c_str());
    std::wstring line;
    while (getline (f, line))
    {
        v.push_back (line);
    }
    f.close();
   
    return v.size();
}

int WriteVectorToTextFile (const std::wstring filename, const std::vector<std::wstring> v)
{
    int result = 0;
   
    std::wofstream f;
    f.open (filename.c_str());
   
    if (f)
    {
        for (std::vector<std::wstring>::const_iterator it = v.begin(); it <v.end(); it++)
        {
            f << *it << L"\n";
        }
        f.close();
        result = 1;
    }
   
    return result;
}

int SubstituteTextInVector (std::vector<std::wstring>& v, const std::wstring oldText, const std::wstring newText)
{
    int numChanges = 0;
   
    for (std::vector<std::wstring>::iterator it = v.begin(); it <v.end(); it++)
		Replace (*it, oldText, newText);
   
    return numChanges;
}

void Replace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr)
{
	std::wstring::size_type pos = 0u;
	while ((pos = str.find (oldStr, pos)) != std::wstring::npos)
	{
		str.replace (pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
}

size_t LoadTextFileIntoMap(const std::wstring filename, std::map<std::wstring, int>& m)
{
    std::wifstream f;
    f.open (filename.c_str());
    std::wstring line;
    while (getline (f, line))
    {
        m[line] = 0;
    }
    f.close();
   
    return m.size();
}

std::vector<std::wstring> Split(const std::wstring& line, const std::wstring& dlim)
{
	std::vector<std::wstring> v;
	RemoveWhitespace rmws(line);
	std::wstring line2 = rmws(RemoveWhitespace::LeadingTrailingCondense);

	std::size_t found = line2.find_first_of(dlim);
	int prev = 0;
	while (found != std::wstring::npos)
	{
		v.push_back(line2.substr(prev, found - prev));
		prev = found + dlim.length();
		found = line2.find_first_of(dlim, prev);
	}

	std::wstring x = line2.substr(prev, line2.length() - prev);
	if (x.length() > 0)
		v.push_back(x);

	return v;
}

//---------------------------------------------------------------------------
// RemoveWhitespace implementation
RemoveWhitespace::RemoveWhitespace(const std::wstring _s) : s(_s)
{
	sSize = s.size();
}

void RemoveWhitespace::operator= (const std::wstring _s)
{
	s = _s;
	sSize = s.size();
}

std::wstring RemoveWhitespace::operator() (Mode mode)
{
	pChar = &s[0];
	std::wstring s1;
	s1.resize(sSize);

	int lastChar = 0;	// last char in copy string
	bool bNewWhiteSpaceBlock = true;
	bool bLeadingWhitespace = true;
	while (*pChar)
	{
		if (_istspace((unsigned char)*pChar))
		{
			// Whitespace

			if (mode & Mode::All)
			{
				// skip
			}
			else if ((mode & Mode::Leading) && bLeadingWhitespace)
			{
				// skip
			}
			else if ((mode & Mode::Condense) && !bNewWhiteSpaceBlock)
			{
				// skip if not 1st ws char in contiguous series
			}
			else
				s1[lastChar++] = ' ';

			bNewWhiteSpaceBlock = false;
		}
		else
		{
			// NOT whitespace, so copy
			bLeadingWhitespace = false;
			bNewWhiteSpaceBlock = true;
			s1[lastChar++] = *pChar;
		}

		pChar++;
	}

	if (mode & Trailing)
	{
		// strip trailing spaces
		while (s1[lastChar - 1] == ' ')
			lastChar--;
	}

	s1.resize(lastChar);

	return s1;
}

std::wstring RemoveWhitespace::operator() (const std::wstring _s, Mode mode)
{
	operator=(_s);
	return operator() (mode);
}

//---------------------------------------------------------------------------

std::wstring MyGetFilename(std::wstring filename)
{
	const size_t last_slash_idx = filename.find_last_of (L"\\/");
	if (std::wstring::npos != last_slash_idx)
	{
		filename.erase (0, last_slash_idx + 1);
	}

	// Remove extension if present.
	const size_t period_idx = filename.rfind ('.');
	if (std::wstring::npos != period_idx)
	{
		filename.erase(period_idx);
	}

	return filename;
}
