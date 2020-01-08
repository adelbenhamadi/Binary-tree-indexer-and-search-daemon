#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cstring>

#include <sstream>
#include <iostream>
#include <fcntl.h>

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <utility>

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>

#define strcasecmp			strcmpi
#define strncasecmp			_strnicmp
#define snprintf			_snprintf
#define strtoll				_strtoi64
#define strtoull			_strtoui64
#define realpath(N,R) _fullpath((R),(N),_MAX_PATH)
/* Type to represent block size.  */
typedef long int __blksize_t;

	#define snprintf	_snprintf
	#define popen		_popen
	#define MY_READ_MODE "rb"
	#define MY_WRITE_MODE "wb"
	#include <io.h>
	#include <tlhelp32.h>
#else
	#include <sys/types.h>
    #include <unistd.h>
	#define MY_READ_MODE "r"
	#define MY_WRITE_MODE "wb"
#endif
#ifdef __linux__
#include <sys/mman.h>
#endif



#include <sys/stat.h>

#include <map>
#include <list>
#include <array>
#include <cstdarg>
#include <chrono>
#include <algorithm>
#include <inttypes.h>
#include <math.h>

#define __STDC_FORMAT_MACROS

#include <mysql.h>

#define STD_NPOS std::string::npos
#define STD_NOW std::chrono::high_resolution_clock::now()

#define DEBUG_LEVEL 1
#define DEBUG_MODE 1
#if DEBUG_MODE
#define debugPrintf(level,_args ...) if(DEBUG_LEVEL>=level) std::fprintf(stdout, _args)
#else
#define debugPrintf(_args ...)
#endif

#define myprintf(_args ...) std::fprintf(stdout, _args)
#define print_err(_args ...) std::fprintf(stderr, _args)

namespace mynodes {

typedef uint64_t u64_t;
typedef std::string string_t;
typedef std::vector<string_t> sVector_t;
typedef std::vector<int32_t> i32Vector_t;
typedef std::vector<u64_t> u64Vector_t;
typedef std::chrono::high_resolution_clock::time_point timePoint_t;


struct MyConnectionConfig {
	const char *server;
	const char *user;
	const char *password;
	const char *database;
};
MYSQL* mysql_connection_setup(struct MyConnectionConfig mysql_details);
MYSQL_RES* mysql_perform_query(MYSQL *connection, char *sql_query);

void normalize_word(string_t &word);
std::string random_string(int seed);
void print_time(const char* mess, timePoint_t t_s);
void strToLower(char *s);
void scan_stdin(const char* fmt, const int co, ...);
sVector_t scan_exp(const string_t& exp);
bool fixBufferSize(std::FILE* fh, const __blksize_t newSize = 8192);
void writeStringToBinFile(const int fdes, const string_t& st);
void writeStringToBinFile(const string_t& st, std::FILE* fh);
string_t readStringFromBinFile(std::FILE* fh);
string_t readStringFromBinFile(const int fdes);
u64_t getFilesize(const char* filename);
bool hasSuffix(const string_t& in, const string_t & suf);
sVector_t explode(string_t const & s, char delim);
string_t implode(sVector_t & elements, char delim) ;


}
#endif
