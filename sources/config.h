#ifndef _config_h_
#define _config_h_

#include "stdint.h"
#include "stddef.h"
#include <stdio.h>

#include <vector>
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
//#include <bits/stdc++.h>

#define STATIC_ASSERT(_cond, _name) typedef char STATIC_ASSERT_FAILED_##_name[(_cond) ? 1 : -1] SPH_ATTR_UNUSED
#define STATIC_SIZE_ASSERT(_type, _size) STATIC_ASSERT(sizeof(_type) == _size, _type##_MUST_BE_##_size##_BYTES)

//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// PORTABILITY
/////////////////////////////////////////////////////////////////////////////

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <intrin.h> // for bsr
#pragma intrinsic(_BitScanReverse)

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#else

typedef unsigned short		WORD;
typedef unsigned char		BYTE;

#endif // _WIN32

/////////////////////////////////////////////////////////////////////////////
// 64-BIT INTEGER TYPES AND MACROS
/////////////////////////////////////////////////////////////////////////////

#if defined(U64C) || defined(I64C)
#error "Internal 64-bit integer macros already defined."
#endif

#if !HAVE_STDINT_H

#if defined(_MSC_VER)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#define U64C(v) v ## UI64
#define I64C(v) v ## I64

#else // !defined(_MSC_VER)
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif // !defined(_MSC_VER)

#endif // no stdint.h

// if platform-specific macros were not supplied, use common defaults
#ifndef U64C
#define U64C(v) v ## ULL
#endif

#ifndef I64C
#define I64C(v) v ## LL
#endif

#define UINT64_FMT "%" PRIu64
#define INT64_FMT "%" PRIi64

#ifndef UINT64_MAX
#define UINT64_MAX U64C(0xffffffffffffffff)
#endif

#ifndef INT64_MIN
#define INT64_MIN I64C(0x8000000000000000)
#endif

#ifndef INT64_MAX
#define INT64_MAX I64C(0x7fffffffffffffff)
#endif


// conversion macros that suppress %lld format warnings vs printf
// problem is, on 64-bit Linux systems with gcc and stdint.h, int64_t is long int
// and despite sizeof(long int)==sizeof(long long int)==8, gcc bitches about that
// using PRIi64 instead of %lld is of course The Right Way,
// so lets wrap them args in INT64() instead
#define INT64(_v) ((long long int)(_v))
#define UINT64(_v) ((unsigned long long int)(_v))
///////////////////////////////////////////////////////////////////////////////////////
#if _WIN32
#include <winsock2.h>

#define realpath(N, R) _fullpath((R), (N), _MAX_PATH)

/* Type to represent block size.  */
typedef long int __blksize_t;

#define popen _popen
#define MY_READ_MODE "rb"
#define MY_WRITE_MODE "wb"
/////

/////
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
/////////////////////////////////////////////////////////////////////////////
// DEBUGGING
/////////////////////////////////////////////////////////////////////////////

#if USE_WINDOWS
#ifndef NDEBUG

#undef assert
#define assert(_expr) (void)( (_expr) || ( sphAssert ( #_expr, __FILE__, __LINE__ ), 0 ) )

#endif // !NDEBUG
#endif // USE_WINDOWS


/// some function arguments only need to have a name in debug builds
#ifndef NDEBUG
#define DEBUGARG(_arg) _arg
#else
#define DEBUGARG(_arg)
#endif

// to avoid disappearing of _expr in release builds
#ifndef NDEBUG
#define Verify(_expr) assert(_expr)
#else
#define Verify(_expr) _expr
#endif

/////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
	#define USE_MYSQL 1		 /// whether to compile MySQL support
	#define USE_WINDOWS 1	 /// whether to compile for Windows
	#define USE_SYSLOG 0	 /// whether to use syslog for logging
	#define HAVE_STRNLEN 1
	#define UNALIGNED_RAM_ACCESS 1
	#define USE_LITTLE_ENDIAN 1
#else
#define USE_WINDOWS 0 /// whether to compile for Windows
#endif

/////////////////////////////////////////////

#define MAX_RESULT_LIMIT 1000000
#define ENABLE_PREFIX 0
#define ENABLE_SUFFIX 0
#define ENABLE_INFIX 0

#define DEBUG_MODE 1
#define DEBUG_LEVEL 2


namespace mynodes
{
 //typedef
	typedef  UINT32 did_t;
	typedef  UINT32 tid_t;
 namespace globals{
	//global verbose flag
	static bool g_bVerbose = false;
	//global print progress flag
	static bool g_bPrintProgress = true;
	//global flag for rotating indexes
	static bool g_bRotate = true;
	//global flag for search results
	static bool g_bShowResultDocs = false;

	static u_int64 g_iMaxLeafSize = 1000000;
	static size_t g_iMemLimit = 128 * 1024 * 1024;
	static u_int64 g_iWriteBuffer = 1024 * 1024;
	static int g_iMinKeywordSize = 3;
	static char *g_sExecName;

	static const char* g_sDataInitDir = "./";
 }
} // namespace mynodes
#endif
