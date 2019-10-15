/*
   Copyright (c) 2015-2019 Max Krasnyansky <max.krasnyansky@gmail.com> 
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
 * @file hogl/c-api/args.h
 * Log record arguments
 */

#ifndef HOGL_CAPI_ARGS_H
#define HOGL_CAPI_ARGS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Enum values and struct layout must match with detail/args.hpp.
// They are redefined here in order to not mess with the clean C++
// naming (namespace and all). This stuff is not supposed to change
// often (if at all), so a little bit of duplication is ok. 
//
// C-API unit-test ensures that things match between C++ and C.
// So make sure that the unit-test is not broken after making 
// changes here.

/**
 * Support argument types.
 * Used internaly by the library.
 */
enum hogl_arg_type {
	HOGL_ARG_NONE,   // No arg
	HOGL_ARG_UINT32, // 32bit Unsigned int
	HOGL_ARG_INT32,  // 32bit Signed int
	HOGL_ARG_UINT64, // 64bit Unsigned int
	HOGL_ARG_INT64,  // 64bit Signed int
	HOGL_ARG_POINTER,// Pointer
	HOGL_ARG_DOUBLE, // Floating point number
	HOGL_ARG_CSTR,   // Regular C string
	HOGL_ARG_GSTR,   // Global C string
	HOGL_ARG_HEXDUMP,// Hexdump 
	HOGL_ARG_RAW,    // Raw data
} types;

/**
 * Record argument.
 * Used internaly by the library.
 */
struct hogl_arg {
	unsigned int type;
	uint64_t     val;
	unsigned int len;
};

#endif // HOGL_CAPI_ARGS_H
