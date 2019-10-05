/*
   Copyright (c) 2015, Max Krasnyansky <max.krasnyansky@gmail.com> 
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:
   
   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
   
   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file hogl/c-api/post.h
 * Top-level C-API for posting log records.
 */
#ifndef HOGL_CAPI_POST_H
#define HOGL_CAPI_POST_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * Explicit string argument specifier.
 * Use this if argument type is ambigious to the compiler
 * @param str pointer to the string
 */
#define hogl_string(str)  ((const char *) (str))

/**
 * Explicit pointer argument specifier.
 * Use this if argument type is ambigious to the compiler
 * @param ptr pointer value
 */
#define hogl_pointer(ptr) ((void *) (ptr))

#ifndef __cplusplus

#include <hogl/detail/preproc.hpp>
#include <hogl/c-api/args.h>
#include <hogl/c-api/area.h>

/* String argument handler */
static inline void __hogl_strarg(struct hogl_arg *arg, const char *str)
{
	arg->type = HOGL_ARG_CSTR;
	arg->len  = strlen(str);
	arg->val  = (unsigned long) str;
}

/* Autodetects argument type and populate hogl_arg structure. */
#define hogl_setarg(args, i, a)	\
do {									\
	if (__builtin_types_compatible_p (typeof((a)), char []) ||	\
	    __builtin_types_compatible_p (typeof((a)), const char *) ||	\
	    __builtin_types_compatible_p (typeof((a)), char *)) {	\
		__hogl_strarg(args + i, (const char *) (unsigned long) (a));	\
		break;								\
	} else {								\
		union {								\
			uint64_t u64;						\
			int64_t  s64;						\
			uint32_t u32;						\
			int32_t  s32;						\
			uint16_t u16;						\
			int16_t  s16;						\
			double   dbl;						\
			float    flt;						\
		} un;								\
		typeof (a) __hogl_x = (a);					\
		const uint8_t *__hogl_p = (const uint8_t *) &__hogl_x;			\
		if (__builtin_types_compatible_p (typeof(__hogl_x), float)) {	\
			un.dbl = *(float *)__hogl_p;					\
			args[i].type = HOGL_ARG_DOUBLE;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), double)) {	\
			un.dbl = *(double *)__hogl_p;					\
			args[i].type = HOGL_ARG_DOUBLE;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), uint64_t)) {\
			un.u64 = *(uint64_t *)__hogl_p;				\
			args[i].type = HOGL_ARG_UINT64;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), int64_t)) {	\
			un.s64 = *(int64_t *)__hogl_p;					\
			args[i].type = HOGL_ARG_INT64;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), uint32_t)) {\
			un.u64 = *(uint32_t *)__hogl_p;				\
			args[i].type = HOGL_ARG_UINT32;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), int32_t)) {	\
			un.s64 = *(int32_t *)__hogl_p;					\
			args[i].type = HOGL_ARG_INT32;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), uint16_t)) {\
			un.u64 = *(uint16_t *)__hogl_p;				\
			args[i].type = HOGL_ARG_UINT32;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), int16_t)) {	\
			un.s64 = *(int16_t *)__hogl_p;					\
			args[i].type = HOGL_ARG_INT32;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), uint8_t)) {	\
			un.u64 = *(uint8_t *)__hogl_p;					\
			args[i].type = HOGL_ARG_UINT32;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), int8_t)) {	\
			un.s64 = *(int8_t *)__hogl_p;					\
			args[i].type = HOGL_ARG_INT32;				\
		} else								\
		if (__builtin_types_compatible_p (typeof(__hogl_x), void *)) {	\
			un.u64 = (unsigned long) __hogl_x;			\
			args[i].type = HOGL_ARG_POINTER;			\
		} else {							\
			un.u64 = 0;						\
			args[i].type = HOGL_ARG_NONE;				\
		}								\
		args[i].val = un.u64;						\
	}									\
} while(0)

/* Internal post parts */
#define hogl_setarg_(a) do { hogl_setarg(__hogl_args, __hogl_i, a); ++__hogl_i; } while(0)
#define hogl_post_(n, area, sect, args) HOGL_PP_CONCAT(__hogl_post_, n)(area, sect, args)

/**
 * Post a log record.
 * @param area area handle
 * @param sect section id
 * @param ... variable argument list
 */
#define hogl_post(area, sect, ...)						\
do {										\
	if (hogl_area_test(area, sect)) {					\
		struct hogl_arg __hogl_args[HOGL_PP_NARGS(__VA_ARGS__)];	\
		unsigned int __hogl_i = 0;					\
		HOGL_PP_FOR_EACH(hogl_setarg_, __VA_ARGS__);			\
		hogl_post_(HOGL_PP_NARGS(__VA_ARGS__), area, sect, __hogl_args);\
	}									\
} while(0)

// Internal post handlers for various number of arguments
void __hogl_post_1(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_2(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_3(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_4(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_5(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_6(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_7(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_8(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_9(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_10(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_11(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_12(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_13(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_14(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_15(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);
void __hogl_post_16(const hogl_area_t area, unsigned int sect, struct hogl_arg *arg);

/**
 * Flush records in the current ring buffer.
 * This function waits for the records in the current ring buffer to be flushed by the engine.
 */
void hogl_flush();

#else /* __cplusplus */

/**
 * If this header is included from a C++ code (happens for common library headers)
 * we simply reuse the C++ post implementation. C version doesn't work under C++ compiler anyway.
 */
#include <hogl/post.hpp>

#define hogl_post(a, s, args...) hogl::post(static_cast<const hogl::area *>(a), s, ##args)

/**
 * Flush records in the current ring buffer.
 * This function waits for the records in the current ring buffer to be flushed by the engine.
 */
extern "C" void hogl_flush();

#endif /* __cplusplus */

#endif /* HOGL_CAPI_POST_H */
