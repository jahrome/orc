/*
 * ORC - Library of Optimized Inner Loops
 * Copyright (c) 2007 David A. Schleef <ds@schleef.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ORC_UTILS_H_
#define _ORC_UTILS_H_

#ifndef _ORC_INTEGER_TYPEDEFS_
#define _ORC_INTEGER_TYPEDEFS_
#if defined(_x_STDC__) && __STDC__ && __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef int8_t orc_int8;
typedef int16_t orc_int16;
typedef int32_t orc_int32;
typedef int64_t orc_int64;
typedef uint8_t orc_uint8;
typedef uint16_t orc_uint16;
typedef uint32_t orc_uint32;
typedef uint64_t orc_uint64;
#elif defined(_MSC_VER)
typedef signed __int8 orc_int8;
typedef signed __int16 orc_int16;
typedef signed __int32 orc_int32;
typedef signed __int64 orc_int64;
typedef unsigned __int8 orc_uint8;
typedef unsigned __int16 orc_uint16;
typedef unsigned __int32 orc_uint32;
typedef unsigned __int64 orc_uint64;
#else
#include <limits.h>
typedef signed char orc_int8;
typedef short orc_int16;
typedef int orc_int32;
typedef unsigned char orc_uint8;
typedef unsigned short orc_uint16;
typedef unsigned int orc_uint32;
#if INT_MAX == LONG_MAX
typedef long long orc_int64;
typedef unsigned long long orc_uint64;
#else
typedef long orc_int64;
typedef unsigned long orc_uint64;
#endif
#endif
typedef union { orc_int16 i; orc_int8 x2[2]; } orc_union16;
typedef union { orc_int32 i; float f; orc_int16 x2[2]; orc_int8 x4[4]; } orc_union32;
typedef union { orc_int64 i; double f; orc_int32 x2[2]; orc_int16 x4[4]; } orc_union64;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef unsigned int orc_bool;

#ifdef ORC_ENABLE_UNSTABLE_API

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#endif
#ifndef ORC_CLAMP
#define ORC_CLAMP(x,a,b) ((x)<(a) ? (a) : ((x)>(b) ? (b) : (x)))
#endif
#define ORC_PTR_TO_INT(x) ((int)(long)(x))
#define ORC_PTR_OFFSET(ptr,offset) ((void *)(((unsigned char *)(ptr)) + (offset)))

#define ORC_READ_UINT32_LE(ptr) \
  ((orc_uint32)( \
    ((orc_uint8 *)(ptr))[0] | \
    (((orc_uint8 *)(ptr))[1]<<8) | \
    (((orc_uint8 *)(ptr))[2]<<16) | \
    (((orc_uint8 *)(ptr))[3]<<24)))

#define ORC_WRITE_UINT32_LE(ptr,val) \
  do { \
    ((orc_uint8 *)ptr)[0] = ((val)>>0)&0xff; \
    ((orc_uint8 *)ptr)[1] = ((val)>>8)&0xff; \
    ((orc_uint8 *)ptr)[2] = ((val)>>16)&0xff; \
    ((orc_uint8 *)ptr)[3] = ((val)>>24)&0xff; \
  } while(0)

#endif

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define ORC_GNUC_PREREQ(maj, min) \
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define ORC_GNUC_PREREQ(maj, min) 0
#endif
  
#if ORC_GNUC_PREREQ(3,3) && defined(__ELF__)
#define ORC_INTERNAL __attribute__ ((visibility ("internal")))
#else
#define ORC_INTERNAL
#endif

#if ORC_GNUC_PREREQ(3,3) /* guess */
#define ORC_GNU_PRINTF(a,b) __attribute__((__format__ (__printf__, a, b)))
#else
#define ORC_GNU_PRINTF(a,b)
#endif

#ifdef __cplusplus
#define ORC_BEGIN_DECLS extern "C" {
#define ORC_END_DECLS }
#else
#define ORC_BEGIN_DECLS
#define ORC_END_DECLS
#endif

#ifdef _MSC_VER
#ifdef ORC_EXPORTS
#define ORC_EXPORT __declspec(dllexport) extern
#else
#define ORC_EXPORT __declspec(dllimport) extern
#endif
#else /* not _MSC_VER */
#define ORC_EXPORT extern
#endif

ORC_BEGIN_DECLS

#ifdef ORC_ENABLE_UNSTABLE_API

int get_file_int (const char *file, int *value);
char * get_file (const char *file);
char * get_cpuinfo_line (char *cpuinfo, const char *tag);
char * _strndup (const char *s, int n);
char ** strsplit (const char *s, char delimiter);
char * get_tag_value (char *s, const char *tag);

#endif

ORC_END_DECLS

#endif

