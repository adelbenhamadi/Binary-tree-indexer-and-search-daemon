#ifndef _utility_h_
#define _utility_h_

#define HAVE_CONFIG_H true

#if HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stddef.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <cstring>
#include <cstdio>
#include <thread>
#include <sstream>
#include <istream>
#include <iostream>
#include <fcntl.h>

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <utility>



#include <sys/stat.h>

#include <map>
#include <list>
#include <array>
#include <cstdarg>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <inttypes.h>
#include <math.h>

#define __STDC_FORMAT_MACROS

#define STD_NPOS std::string::npos
#define STD_NOW std::chrono::high_resolution_clock::now()
#define TIME_MilS(_t) std::chrono::duration_cast<std::chrono::milliseconds>(_t).count()
#define TIME_MicS(_t) std::chrono::duration_cast<std::chrono::microseconds>(_t).count()
#define TIME_NS(_t) std::chrono::duration_cast<std::chrono::nanoseconds>(_t).count()
//global types
typedef uint64_t u64_t;
typedef std::string string_t;
typedef std::vector<string_t> sVector_t;
typedef std::vector<int32_t> i32Vector_t;
typedef std::vector<u64_t> u64Vector_t;
typedef std::chrono::high_resolution_clock::time_point timePoint_t;

const string_t csKEY_ZERO = "*";


/////////////////////////////////////////////////////////////
//print functions
/////////////////////////////////////////////////////////////
#define diePrintf(...)          \
	{                                \
		std::fprintf(stdout, __VA_ARGS__); \
		exit(1);                     \
	}

#define printInfo(...) std::fprintf(stdout, __VA_ARGS__)
#define printError(...) std::fprintf(stderr, __VA_ARGS__)
#define printWarning(...) std::fprintf(stdout, __VA_ARGS__)

#if DEBUG_MODE
#define debugPrintf(level, ...) \
	if (DEBUG_LEVEL >= level)        \
	std::fprintf(stdout, __VA_ARGS__)
#else
#define debugPrintf(_args...) std::fprintf(stdout, __VA_ARGS__)
#endif

///safe delete stuff
#define safeDelete(_x)      \
	{                       \
		if (_x)             \
		{                   \
			delete (_x);    \
			(_x) = nullptr; \
		}                   \
	}
#define safeDeleteArray(_x) \
	{                       \
		if (_x)             \
		{                   \
			delete[](_x);   \
			(_x) = nullptr; \
		}                   \
	}
#define safeRelease(_x)      \
	{                        \
		if (_x)              \
		{                    \
			(_x)->Release(); \
			(_x) = nullptr;  \
		}                    \
	}
////////////////////////////////////////////////////////
//loopers
////////////////////////////////////////////////////////
#define ARRAY_FOREACH(_index, _array) \
	for (int _index = 0; _index < _array.size(); _index++)

#define ARRAY_FOREACH_COND(_index, _array, _cond) \
	for (int _index = 0; _index < _array.size() && (_cond); _index++)

#define ARRAY_ANY(_res, _array, _cond)                        \
	false;                                                    \
	for (int _any = 0; _any < _array.size() && !_res; _any++) \
		_res |= (_cond);

#define ARRAY_ALL(_res, _array, _cond)                       \
	true;                                                    \
	for (int _all = 0; _all < _array.size() && _res; _all++) \
		_res &= (_cond);

#define ARRAY_CONTAINS(_array, _item) \
	std::find(_array.begin(), _array.end(), _item) != _array.end()

#define ARRAY_UNIQ(_array)                                 \
	auto last = std::unique(_array.begin(), _array.end()); \
	_array.erase(last, _array.end());
#define ARRAY_SORT(_array, _compareF) \
	std::sort(_array.begin(), _array.end(), _compareF);

#define STR_TOLOWER(_s) std::transform(_s.begin(), _s.end(), _s.begin(), std::tolower)
#define STR_TOUPPER(_s) std::transform(_s.begin(), _s.end(), _s.begin(), std::toupper)

namespace mynodes{
	namespace utility{
		
			
			std::string random_string(int seed);
			void print_time(const char* mess, timePoint_t t_s);		
			void scan_stdin(const char* fmt, const int co, ...);
			sVector_t scan_exp(const string_t & exp);
			bool writeVector(std::ofstream & input, char* pVec, size_t sz, size_t co, bool bWriteCount, bool bClose);
			void writeStringToBinFile(const int fdes, const string_t & st);
			string_t readStringFromBinFile(const int fdes);
			void writeStringToBinFile(const string_t & st, std::FILE * fh);
			string_t readStringFromBinFile(std::FILE * fh);
			void writeStringToBinFile(const string_t & st, std::ostream & os);
			string_t readStringFromBinFile(std::istream & is);
			bool readStringFromBinFile(std::istream & is, char* dest);
			u64_t getFilesize(const char* filename);
			void explode(sVector_t& dOut, string_t const& s, char delim);
			string_t implode(sVector_t& elements, char delim);

			/// swap
			template <typename T>
			inline void Swap(T& v1, T& v2)
			{
				T temp = v1;
				v1 = v2;
				v2 = temp;
			}
			inline bool hasSuffix(const string_t & in, const string_t & suf)
			{
				size_t pos = in.find(suf);      // position of suf in in
				if (pos == STD_NPOS) {
					return false;
				}
				string_t f = in.substr(pos);     // get from pos to the end
				return f == suf;
			}
			
			inline bool checkToken(const char* p) {
				const int& c = p[0];
				if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') /*|| ( c>='A' && c<='Z' )*/ || c == '-' || c == '_') return true;
				/*if(c=='.' || c==';' || c==',' || c==':' || c=='/' || c=='\\' || c=='?' || c=='!' || c=='=' || c=='+' || c=='%' || c=='*' || c=='(' || c==')' || c=='[' || c==']') *p++;
				else*/
				if (c == '<') {
					if (p[1] == 'a') {
						while (*p) {
							p++;
							if (*p == '>') break;
						}
						return false;
					}
					if ((p[1] == 'p' || p[1] == 'i') && p[2] == '>') p += 3;
					else if (((p[1] == 'ul' && p[2] == 'l') || (p[1] == 'l' && p[2] == 'i')) && p[3] == '>') p += 4;
					else if (
						((p[1] == 'p' && p[2] == 'r' && p[3] == 'e') || (p[1] == 'i' && p[2] == 'm' && p[3] == 'g'))
						&& p[4] == '>') p += 5;
					return false;
				}
				p++;
				return false;
			}

			static char EMPTY[1];
			static const int SAFETY_GAP = 4;

			inline bool IsEmpty(const char* cIn)
			{
				if (!cIn)
					return true;
				return ((*cIn) == '\0');
			}

			inline bool hasPrefix(const char* cIn, const char* cTest)
			{
				if (!cIn || !cTest)
					return false;
				return strncmp(cIn, cTest, strlen(cTest)) == 0;
			}

			inline bool hasSuffix(const char* cIn, const char* cTest)
			{
				if (!cIn || !cTest)
					return false;

				size_t iVal = strlen(cIn);
				size_t iSuffix = strlen(cTest);
				if (iVal < iSuffix)
					return false;
				return strncmp(cIn + iVal - iSuffix, cTest, iSuffix) == 0;
			}

			inline void Trim(char* cIn)
			{
				if (cIn)
				{
					const char* sStart = cIn;
					const char* sEnd = cIn + strlen(cIn) - 1;
					while (sStart <= sEnd && isspace((unsigned char)*sStart))
						sStart++;
					while (sStart <= sEnd && isspace((unsigned char)*sEnd))
						sEnd--;
					memmove(cIn, sStart, sEnd - sStart + 1);
					cIn[sEnd - sStart + 1] = '\0';
				}
			}

			inline void Unquote(char* cIn)
			{
				auto l = strlen(cIn);
				if (l && cIn[0] == '\'' && cIn[l - 1] == '\'')
				{
					memmove(cIn, cIn + 1, l - 2);
					cIn[l - 2] = '\0';
				}
			}


			inline int IsAlpha(int c)
			{
				return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_';
			}

			inline bool IsSpace(int iCode)
			{
				return iCode == ' ' || iCode == '\t' || iCode == '\n' || iCode == '\r';
			}

			/// check for keyword modifiers
			inline bool IsModifier(int iSymbol)
			{
				return iSymbol == '^' || iSymbol == '$' || iSymbol == '=' || iSymbol == '*';
			}

			template <typename T>
			inline bool IsWildcard(T c)
			{
				return c == '*' || c == '?' || c == '%';
			}


			static char* ltrim(char* sLine)
			{
				while (*sLine && IsSpace(*sLine))
					sLine++;
				return sLine;
			}

			static char* rtrim(char* sLine)
			{
				char* p = sLine + strlen(sLine) - 1;
				while (p >= sLine && IsSpace(*p))
					p--;
				p[1] = '\0';
				return sLine;
			}

			static char* trim(char* sLine)
			{
				return ltrim(rtrim(sLine));
			}

			



		}

} // namespace mynodes
#endif
