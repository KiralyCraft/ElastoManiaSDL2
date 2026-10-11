#pragma once
// Compatibility declarations for the original Win32 C library surface.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <cctype>
#include <limits>
#include <climits>
#include <new>
static_assert(CHAR_BIT == 8 && sizeof(int) == 4 && sizeof(short) == 2, "Elma requires 8/16/32-bit byte/short/int.");
static_assert(sizeof(float) == 4 && sizeof(double) == 8 && std::numeric_limits<double>::is_iec559, "Elma requires IEEE binary32/binary64.");
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Big-endian builds require field-wise serialization work and are not supported yet."
#endif
int elma_stricmp(const char* a, const char* b);
int elma_strnicmp(const char* a, const char* b, size_t n);
char* elma_strupr(char* text);
char* elma_strlwr(char* text);
FILE* elma_fopen(const char* name, const char* mode);
int elma_access(const char* name, int mode);
int elma_remove(const char* name);
int elma_rename(const char* old_name, const char* new_name);
int elma_mkdir(const char* name);
int32_t elma_filelength(int handle);
char* elma_itoa(int value, char* dest, int base);
#define strcmpi elma_stricmp
#define stricmp elma_stricmp
#define _stricmp elma_stricmp
#define strnicmp elma_strnicmp
#define _strnicmp elma_strnicmp
#define strupr elma_strupr
#define strlwr elma_strlwr
#define fopen elma_fopen
#define access elma_access
#define remove elma_remove
#define rename elma_rename
#define mkdir elma_mkdir
#define filelength elma_filelength
#define itoa elma_itoa
#define _MAX_PATH 260
#ifndef __cdecl
#define __cdecl
#endif
#include "numeric.hpp"
#define sin elma_math_sin
#define cos elma_math_cos
#define sqrt elma_math_sqrt
#define atan2 elma_math_atan2
int elma_rand();
void elma_srand(unsigned seed);
#define rand elma_rand
#define srand elma_srand
