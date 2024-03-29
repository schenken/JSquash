// JSquash - Javascript Squasher

#include "pch.h"
#include <iostream>
#include <vector>
#include <map>
#include "Common.h"
#include <locale>
#include <codecvt>
#include <sstream>

// Globals

const std::wstring validNameChars = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";
const std::wstring validNameLetters = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";

// List of symbol substitution candidates found in the JS
std::map<std::wstring, int> mSymbols;

void PrintHelp();
void ParseCommandLine (int argc, WCHAR* argv[]);
void Parse (int flags = 0);
void RemoveComments (std::vector<uint8_t>& js1, bool removeWhitespace = false);
void RemoveWS (std::vector<uint8_t>& js1);
bool NextCharIsSymbol (int pos, const std::vector<uint8_t>& js1, int& posNextNonWsChar, WCHAR& ch);
std::wstring CheckForComment (int& pos, const std::vector<uint8_t>& js);
std::wstring EncodeJsVarName (int n);
std::map<int, bool> MapQuotedString (const std::vector<uint8_t>& js);
void CheckForQuotedString (const WCHAR& c, int pos, int& posStart, int& posEnd, WCHAR& quoteMark);
void InitReservedWords();

std::vector<uint8_t> js, jsNew;
std::map<std::wstring, int> mIgnoreWords;
std::map<std::wstring, int> mReservedWords;

// Registers instances when a word was ignored. Thus we can compare this
// with mIgnoreWords list: After the first Parse() if mIgnoreWords contains
// symbols that are NOT in mIgnored, such words are redundant (I may have
// changed JS code function names) so we don't want unnecessary clutter.
// Therefore, mIgnored supercedes mIgnoreWords and this is reflected in
// an updated version of [xxx]_js_ignore.txt.
std::map<std::wstring, int> mIgnored;

std::wstring jsFileReserved (L"js_reserved.txt");
std::wstring jsFileIn;
std::wstring jsFileOut;
std::wstring jsFileSymbols;		// Output: List of my symbols.
std::wstring jsFileIgnore;		// Input: List of symbols to ignore, ie. not change.

int modeFlags = 0;

std::wstring_convert<std::codecvt_utf8<wchar_t>> convWS;

int wmain (int argc, WCHAR* argv[], WCHAR* envp[])
{	
	ParseCommandLine (argc, argv);
	if (jsFileIn.size() == 0)
		return 0;

	// Init file names.
	std::wstring jsFilename = MyGetFilename (jsFileIn);
	jsFileSymbols = jsFilename + L"_js_symbols.txt";
	jsFileIgnore = jsFilename + L"_js_ignore.txt";

	LoadTextFileIntoMap (jsFileIgnore, mIgnoreWords);
	InitReservedWords();

	// Update (a) reserved word list and (b) my symbols we don't want changed.
	std::vector<std::wstring> vSymbols;
	LoadTextFileIntoVector (jsFileSymbols, vSymbols);
	for (auto v : vSymbols)
	{
		const size_t pos = v.find (L" ");
		if (std::wstring::npos != pos)
			v.erase (pos);

		if (v[0] == '*')
		{
			v.erase (0, 1);
			mReservedWords[v] = 0;
		}
		else if (v[0] == '+')
		{
			v.erase (0, 1);
			mIgnoreWords[v] = 0;
		}
	}
	// Save the updated reserved word list to disk.
	// (We save the ignore list further down.)
	std::vector<std::wstring> vW;
	for (auto const& v : mReservedWords)
		vW.push_back (v.first);
	WriteVectorToTextFile (jsFileReserved, vW);

	// Read the js file.
	std::ifstream jsIn (jsFileIn, std::ios::in | std::ios::binary);
	js.assign ((std::istreambuf_iterator<char>(jsIn)), std::istreambuf_iterator<char>());

	// Main process of digging out all symbols, identifying comments, quoted strings.
	Parse();


	// After the parse which identifies all symbols and instances of
	// ignored words, we now refresh the ignore-words list from the
	// list of words that were actually ignored (and save it to disk).
	// Thus an automatic purge of unsed ignore words occurs.
	mIgnoreWords.clear();
	for (auto m : mIgnored)
		mIgnoreWords[m.first] = m.second;
	vW.clear();
	for (auto const& v : mIgnoreWords)
		vW.push_back (v.first);
	WriteVectorToTextFile (jsFileIgnore, vW);


	// Consolidate the list of found symbols and save to file.
	int count = 0;
	vSymbols.clear();
	for (auto& v : mSymbols)
	{
		// We generate a symbol from a number. However, the generated symbol must not clash
		// with any symbol in the reserved word list or my ignore list. If that happens,
		// increment number and retry.
		std::wstring sym;
		for (;;)
		{
			++count;
			sym = EncodeJsVarName (count);
			if (mReservedWords.find (sym) == mReservedWords.end() 
				&& mIgnoreWords.find (sym) == mIgnoreWords.end())
				break;
		}

		v.second = count;
		std::wstring s = v.first + L" (" + sym + L")";
		vSymbols.push_back (s);
	}
	WriteVectorToTextFile (jsFileSymbols, vSymbols);

	// mSymbols is our comprehensive list of symbols that must be substituted in the js.
	// Parse again to do the critical bizz.
	Parse (modeFlags);

	std::ofstream jsOut (jsFileOut, std::ios::out | std::ofstream::binary);
	std::copy (jsNew.begin(), jsNew.end(), std::ostreambuf_iterator<char>(jsOut));

	//-------------------------------------------------------------------------
	// Output some stats.
	std::wcout << jsFileIn << L" size: " << js.size() << L" bytes.\n";
	std::wcout << jsFileOut << L" size: " << jsNew.size() << L" bytes.\n";
	float diff = ((float)jsNew.size() / (float)js.size()) * 100.0f;

	float pcent = 0.0f;
	std::wstring lblDiff;
	if (diff > 100.0f)
	{
		pcent = diff - 100.0f;
		lblDiff = L"% larger.";
	}
	else if (diff < 100.0f)
	{
		pcent = 100.0f - diff;
		lblDiff = L"% smaller.";
	}

	if (pcent != 0.0f)
	{
		std::wostringstream out;
		out.precision (2);
		out << std::fixed << pcent;
		out << lblDiff;
	    std::wcout << L"Output is " << out.str() << '\n';
	}
}

void ParseCommandLine (int argc, WCHAR* argv[])
{
	if (argc < 3)
	{
		PrintHelp();
		return;
	}

	std::vector<std::wstring> vArgs;
	if (argc > 3)
	{
		for (int i = 3; i < argc; ++i)
		vArgs.push_back (argv[i]);
	}

	// Inspect the entered args.
	bool invalidOption = false;
	for (auto v : vArgs)
	{
		if (v == L"-v")					// verify only
		{
			modeFlags |= 1UL << 2;
		}
		else if (v == L"-s")			// substitute mode
		{
			modeFlags |= 1UL << 0;
		}
		else if (v == L"-rc")			// remove comments
		{
			modeFlags |= 1UL << 1;
		}
		else if (v == L"-rcw")			// remove comment and whitespace
		{
			modeFlags |= 1UL << 1;
			modeFlags |= 1UL << 4;
		}
		else
		{
			std::wcout << L"Invalid option: " << v << '\n';
			invalidOption = true;
		}
	}
	
	if (invalidOption)
	{
		PrintHelp();
		return;
	}

	jsFileIn = argv[1];
	jsFileOut = argv[2];
}

void PrintHelp()
{
	const WCHAR* text =

		L"\nUsage:\n\n"

		L"    jsquash.exe <infile> <outfile> [options]\n\n"

		L"where:\n\n"
		
		L"    <infile>\n\n"
		
		L"        Name of input javascript file.\n\n"
		
		L"    <outfile>\n\n"
		
		L"        Name of output javascript file.\n\n"
		
		L"    options\n\n"
		
		L"        -s\n\n"
		
		L"            Substitute mode. Output file contains auto-generated.\n"
		L"            symbols that replace your original symbols\n\n"
		
		L"        -rc\n\n"
		
		L"            Remove comments.\n\n"
		
		L"        -rcw\n\n"
		
		L"            Remove comments and all unecessary whitespace.\n\n"
		
		L"        -v\n\n"
		
		L"            Verify only. Prefixes symbols with \"A$\" to indicate where\n"
		L"            substitutions will occur.\n\n"
		
		L"If no options are specified the output file is a copy of the input file.\n\n"

		;

	wprintf (text);

	return;
}

void Parse (int flags)
{
	// Parse the js. We're looking for any alphanumeric (plus '_' and '$') symbol that
	// is not reserved. Ignore comments and quoted strings.

	bool substitute = flags & (1 << 0);
	bool stripComments = flags & (1 << 1);
	bool verifyOnly = flags & (1 << 2);
	bool removeWhitespace = flags & (1 << 4);

	int _pos = 0;
	int jsSize = js.size();

	int posStartSymbol = -1;
	
	std::vector<uint8_t> js1;

	WCHAR quoteMark = 0;
	int posQuotedStringStart = -1;
	int posQuotedStringEnd = -1;

	while (_pos < jsSize)
	{
		int posEnd = jsSize - _pos > 100 ? _pos + 100 : jsSize - 1;
		std::wstring checkStr (js.begin() + _pos, js.begin() + posEnd);

		std::wstring comment = CheckForComment (_pos, js);
		if (comment.size())
		{
			if (stripComments)
			{
				// Neat spot of code to read backwards a few characters in output
				// and remove unnecessary whitespace. This just tidies things a bit.
				// Don't mess with this - it works!
				int _pos2 = js1.size() - 1;
				while (_pos2 >= 0)
				{
					if (!(js1[_pos2] == ' ' || js1[_pos2] == '\t'))
					{
						if (js1[_pos2] != '\n' && comment[1] == '/')
						{
							js1.push_back ('\r');
							js1.push_back ('\n');
						}
						break;
					}
					js1.pop_back();
					_pos2--;
				}
			}
			else
			{
				std::string sym = convWS.to_bytes (comment);
				auto vbSym = reinterpret_cast<const byte*>(sym.data());
				js1.insert (js1.end(), vbSym, vbSym + sym.size());
			}
			_pos++;
			continue;
		}

		//---------------------------------------------------------------------------------

		CheckForQuotedString (js[_pos], _pos, posQuotedStringStart, posQuotedStringEnd, quoteMark);
		if (posQuotedStringStart >= 0)
		{
			js1.push_back (js[_pos]);
			if (posQuotedStringEnd >= 0)
				posQuotedStringStart = -1;
			_pos++;
			continue;
		}

		//---------------------------------------------------------------------------------
		// Look for valid js symbols.
		int pos1 = validNameChars.find (js[_pos], 0);
		if (pos1 != std::wstring::npos)
		{
			if (posStartSymbol == -1)
			{
				if (pos1 > 9)
					// Start of symbol (it does not begin with a number)
					posStartSymbol = _pos;
				else
					js1.push_back (js[_pos]);
			}
		}
		else
		{
			if (posStartSymbol >= 0)
			{
				// Completed symbol.

				// Put symbol string into byte vector.
				std::string sym (js.begin() + posStartSymbol, js.begin() + _pos);
				auto vbSym = reinterpret_cast<const byte*>(sym.data());

				std::wstring symbol (js.begin() + posStartSymbol, js.begin() + _pos);

				// See if it's a reserved word and not in our ignore list.
				if (mReservedWords.find (symbol) == mReservedWords.end() && mIgnoreWords.find (symbol) == mIgnoreWords.end())
				{
					if (substitute)
					{
						// We're in substitute mode, so generate a new symbol.
						std::wstring replaceSymbol = EncodeJsVarName (mSymbols[symbol]);

						// Convert to UTF-8 (multibyte) so we can treat it as a char array
						// for appending to the output byte vector.
						sym = convWS.to_bytes (replaceSymbol);
						vbSym = reinterpret_cast<const byte*>(sym.data());
					}
					else
					{
						// A unique symbol - save to list.
						mSymbols[symbol] = 0;

						if (verifyOnly)
						{
							// Verify mode just prefixes the original symbol with "A$" so you can
							// see what *would* be changed.
							js1.push_back ('A');
							js1.push_back ('$');
						}
					}

					js1.insert (js1.end(), vbSym, vbSym + sym.size());
				}
				else
				{
					// Since it's a reserved word, write symbol unmodified to output.
					js1.insert (js1.end(), vbSym, vbSym + sym.size());
				}

				// Record when it's ignored
				if (mIgnoreWords.find(symbol) != mIgnoreWords.end())
					mIgnored[symbol] = 0;


				posStartSymbol = -1;
			}
			js1.push_back (js[_pos]);
		}

		_pos++;
	}

	if (stripComments)
		RemoveComments (js1, removeWhitespace);

	jsNew.assign (js1.begin(), js1.end());
}

void RemoveComments (std::vector<uint8_t>& js1, bool removeWhitespace)
{
	std::vector<uint8_t> js2;

	// This loop effectively reads in one line at a time, ie. string
	// of chars ending in "\r\n". Then we see if it's a blank line.
	// We ensure that only ever one contiguous blank line is output.
	int pos = 0;
	int js1Size = js1.size();
	int lineStart = 0;
	bool blankLine = false;
	while (pos < js1Size)
	{
		if (js1[pos] == '\r' && js1[pos + 1] == '\n')
		{
			// end of line
			std::wstring line (js1.begin() + lineStart, js1.begin() + pos + 2);

			RemoveWhitespace rmws (line);
			std::wstring line2 = rmws(static_cast<RemoveWhitespace::Mode>(RemoveWhitespace::All));

			if (line2.size() > 0)
			{
				std::string mbLine = convWS.to_bytes (line);
				auto vb = reinterpret_cast<const byte*>(mbLine.data());
				js2.insert (js2.end(), vb, vb + mbLine.size());
				blankLine = false;
			}
			else
			{
				if (!blankLine)
				{
					js2.push_back ('\r');
					js2.push_back ('\n');
					blankLine = true;
				}
			}

			pos += 2;
			lineStart = pos;
			continue;
		}

		pos++;
	}

	if (removeWhitespace)
		RemoveWS (js2);

	js1.assign (js2.begin(), js2.end());
}

void RemoveWS (std::vector<uint8_t>& js1)
{
	// (1) Remove all whitespace (unless it's embedded in a quoted string)
	std::map<int, bool> mQ = MapQuotedString (js1);
	std::vector<uint8_t> js2;

	bool prevNonWsCharIsSymbolChar = false;
	WCHAR prevNonWsChar = 0;
	bool nextNonWsCharIsSymbolChar = false;
	WCHAR nextNonWsChar = 0;
	int posNextNonWsChar = -1;
	int posNextNonWsChar1 = -1;

	int pos = 0;
	int js1Size = js1.size();
	while (pos < js1Size)
	{
		char c = js1[pos];
		if (isspace (c))
		{
			// Always know if the following non-ws-char is symbol.
			if (posNextNonWsChar <= pos)
				nextNonWsCharIsSymbolChar = NextCharIsSymbol (pos, js1, posNextNonWsChar, nextNonWsChar);

			if (mQ.find (pos) != mQ.end() && mQ[pos])	// embedded in quoted string, so output
				js2.push_back (c);

			else if (prevNonWsCharIsSymbolChar && nextNonWsCharIsSymbolChar)
			{
				// The crux: Prevent whitespace between two symbols (regardless of whether
				// they are keywords or your own symbol names) from disappearing. We will
				// retain a single space. This prevents, for example, "var eric;" becoming
				// "vareric;".
				if (posNextNonWsChar != posNextNonWsChar1)
					js2.push_back (c);
				posNextNonWsChar1 = posNextNonWsChar;
			}

		}
		else
		{
			prevNonWsChar = c;
			prevNonWsCharIsSymbolChar = validNameChars.find (c, 0) != std::wstring::npos;
			js2.push_back (c);
		}

		pos++;
	}

	js1.assign (js2.begin(), js2.end());
}

bool NextCharIsSymbol (int pos, const std::vector<uint8_t>& js1, int& posNextNonWsChar, WCHAR& ch)
{
	int pos1 = pos + 1;
	bool got = false;
	while (pos1 < (int)js1.size())
	{
		if (!isspace (js1[pos1]))
		{
			got = true;
			break;
		}
		pos1++;
	}

	bool nextNonWsCharIsSymbolChar = false;
	if (got)
	{
		posNextNonWsChar = pos1;
		ch = js1[posNextNonWsChar];
		nextNonWsCharIsSymbolChar = validNameChars.find (ch, 0) != std::wstring::npos;
	}

	return nextNonWsCharIsSymbolChar;
}

std::wstring CheckForComment (int& pos, const std::vector<uint8_t>& js)
{
	std::wstring comment;
	WCHAR c = js[pos];
	int jsSize = js.size();

	if (c == '/' && pos + 1 < jsSize && js[pos + 1] == '/')
	{
		comment += c;
		pos++;
		while (pos < jsSize)
		{
			c = js[pos];
			comment += c;
			if (c == '\r' && pos + 1 < jsSize && js[pos + 1] == '\n')
			{
				comment += '\n';
				pos++;
				return comment;
			}
			pos++;
		}
	}

	if (c == '/' && pos + 1 < jsSize && js[pos + 1] == '*')
	{
		comment += c;
		pos++;
		while (pos < jsSize)
		{
			c = js[pos];
			comment += c;
			if (c == '*' && pos + 1 < jsSize && js[pos + 1] == '/')
			{
				comment += '/';
				pos++;
				return comment;
			}
			pos++;
		}
	}

	return comment;
}

std::wstring EncodeJsVarName (int n)
{
	int base = validNameLetters.size();
	int bp = 1;
	std::wstring res = L"";

	while (n > 0)
	{
		n--;		// <------------------------------------- CRITICAL !!!!!!!
		int a = n % (bp * base);
		res = validNameLetters[a / bp] + res;
		n -= a;
		bp *= base;
	}

	return res;
}


std::map<int, bool> MapQuotedString (const std::vector<uint8_t>& js)
{
	// Sets booleans in a map to indicate where quoted strings occur.
	int _pos = 0;
	int jsSize = js.size();
	WCHAR quoteMark = 0;
	int posStart = -1;
	int posEnd = -1;
	std::map<int, bool> mQ;

	while (_pos < jsSize)
	{
		CheckForQuotedString (js[_pos], _pos, posStart, posEnd, quoteMark);
		if (quoteMark != 0)
			mQ[_pos] = true;
		if (posStart >= 0 && posEnd >= 0)
			posStart = -1;
		_pos++;
	}

	return mQ;
}

void CheckForQuotedString (const WCHAR& c, int pos, int& posStart, int& posEnd,  WCHAR& quoteMark)
{
	static bool escapedChar;

	if (posStart == -1)
		posEnd = -1;

	if (c == '"' || c == '\'')
	{
		if (quoteMark == 0 || c == quoteMark)
		{ 
			if (posStart == -1)
			{
				posStart = pos;
				posEnd = -1;
				quoteMark = c;
				escapedChar = false;
			}
			else
			{
				if (!escapedChar)
				{
					posEnd = pos;
					quoteMark = 0;
				}
				else
					escapedChar = false;
			}
		}
	}
	else if (posStart >= 0)
	{
		escapedChar = c == '\\' ? true : false;
		posEnd = -1;
	}
}

//-----------------------------------------------------------------------------

void InitReservedWords()
{
	LoadTextFileIntoMap (jsFileReserved, mReservedWords);

	// Automatically reserve all single-char symbols,
	// since they're hardly worth obfuscating.
	for (int i = 1; i <= 54; ++i)
	{
		std::wstring sym = EncodeJsVarName (i);
		mReservedWords[sym] = 0;
	}

	// May as well bung in some obvious keywords
	const WCHAR* const reservedWords[] = {
		L"addEventListener", L"appendChild", L"break", L"charAt", L"className", L"clientHeight", 
		L"clientWidth", L"color", L"createElement", L"display", L"documentElement", L"elements", 
		L"firstChild", L"form", L"forms", L"getElementById", L"getElementsByClassName", 
		L"id", L"innerHTML", L"innerText", L"innerWidth", L"length", L"name", L"offsetHeight", 
		L"onclick", L"options", L"parentNode", L"parse", L"previousSibling", L"prototype", L"removeChild", 
		L"scrollHeight", L"selectedIndex", L"stringify", L"style", L"target", L"test", L"textContent", 
		L"trim", L"type", L"userAgent", L"value", L"Edge", L"JSON", L"MSIE", L"XMLHttpRequest", L"case", 
		L"decodeURIComponent", L"document", L"else", L"false", L"for", L"function", L"if", L"navigator", 
		L"new", L"null", L"return", L"switch", L"this", L"true", L"var", L"window"
	};

	for (auto s : reservedWords)
		mReservedWords[s] = 0;
}
