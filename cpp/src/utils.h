#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <assert.h>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utility>
#include <map>
#include <list>
#include <array>
#include <cstdarg>
#include <chrono>
#include <algorithm>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <mysql.h>

#define STD_NPOS std::string::npos
#define STD_NOW std::chrono::high_resolution_clock::now()

#define DEBUG_MODE 0
#if DEBUG_MODE
#define debugPrintf(_args ...) std::fprintf(stdout, _args)
#else
#define debugPrintf(_args ...)
#endif
#define myprintf(_args ...) std::fprintf(stdout, _args)
#define print_err(_args ...) std::fprintf(stderr, _args)

namespace mynodes {
typedef std::string string_t;
typedef std::chrono::high_resolution_clock::time_point time_point_t;
typedef uint64_t size64_t;

struct serverConnectionConfig {
	const char *server;
	const char *user;
	const char *password;
	const char *database;
};
MYSQL* mysql_connection_setup(struct serverConnectionConfig mysql_details);
MYSQL_RES* mysql_perform_query(MYSQL *connection, char *sql_query);

void normalize_word(string_t &word);
void print_time(const char* mess, time_point_t t_s);
void strToLower(char *s);
void scan_stdin(const char* fmt, const int co, ...);
std::vector<string_t> scan_exp(const string_t& exp);
bool fixBufferSize(std::FILE* fh, const __blksize_t newSize = 8192);
void writeStringToBinFile(const int fdes, const string_t& st);
void writeStringToBinFile(const string_t& st, std::FILE* fh);
string_t readStringFromBinFile(std::FILE* fh);
string_t readStringFromBinFile(const int fdes);
size64_t getFilesize(const char* filename);
bool hasSuffix(const string_t& in, const string_t & suf);
std::vector<string_t> explode(string_t const & s, char delim);
string_t implode(std::vector<string_t> & elements, char delim) ;


}
#endif
