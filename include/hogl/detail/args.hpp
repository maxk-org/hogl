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
 * @file hogl/detail/args.h
 * Details of the argument handling.
 */
#ifndef HOGL_DETAIL_ARGS_HPP
#define HOGL_DETAIL_ARGS_HPP

#include <stdint.h>
#include <limits.h>
#include <string>

#include <hogl/detail/compiler.hpp>

namespace hogl {

/**
 * Global string.
 * Special argument that tells HOGL that it's safe to use 
 * string pointer as is. No string copy is done in this case.
 */
struct arg_gstr {
	const char *str;
	hogl_force_inline arg_gstr(const char *s): str(s) {}
};

/**
 * Hexdump.
 * Special argument that tells HOGL to store the data as is and 
 * to generate a hexdump on the output.
 */
struct arg_hexdump {
	const void  *ptr;
	unsigned int len;
	hogl_force_inline arg_hexdump(const void *p, unsigned int n): ptr(p), len(n) {}
};

/**
 * Raw data.
 * Special argument that tells HOGL to store the data as is.
 * Default HOGL format handlers ignore RAW arguments. 
 * This type is designed for custom format handlers. Argument value in the final 
 * record is set to the offset within the tail buffer ORed with the length
 *		(uint64_t) offset << 32 | (length & 0xffffffff)
 */
struct arg_raw {
	const void  *ptr;
	unsigned int len;
	hogl_force_inline arg_raw(const void *p, unsigned int n): ptr(p), len(n) {}
};

/**
 * Log record argument
 */
struct arg {
	/**
	 * Argument types
	 */
	enum {
		NONE,   /// No arg
		UINT32, /// 32bit unsigned int
		INT32,  /// 32bit signed int
		UINT64, /// 64bit unsigned int
		INT64,  /// 64bit signed int
		POINTER,/// Pointer
		DOUBLE, /// Floating point number
		CSTR,   /// Regular C string
		GSTR,   /// Global string (see below)
		HEXDUMP,/// Hexdump
		RAW,    /// Raw data
	};

	// This layout generates the most optimal code.
	// Do not change it without making sure that the compiler is 
	// able to optimize out all unessesary code.
	unsigned int type;
	uint64_t     val;
	unsigned int len;

#if ULONG_MAX == UINT_MAX
	// 32bit arch
	static bool is_32bit(unsigned int type)
	{
		return type != UINT64 && type != INT64 && type != DOUBLE;
	}
#else
	// 64bit arch
	static bool is_32bit(unsigned int type)
	{
		return type == UINT32 || type == INT32;
	}
#endif

	bool is_32bit() const { return is_32bit(type); }

	bool is_simple() const 
	{
		return (type != CSTR && type != HEXDUMP && type != RAW); 
	}

	// Various constructors for autodetecting argument types.
	// Force inlining them to avoid unnecessary code when optimizations
	// are disabled.
	hogl_force_inline arg()                : type(NONE) {}
	hogl_force_inline arg(void *ptr)       : type(POINTER), val((unsigned long) ptr) {}
	hogl_force_inline arg(volatile void *ptr) : type(POINTER), val((unsigned long) ptr) {}
	hogl_force_inline arg(unsigned int i)  : type(UINT32), val(i) {}
	hogl_force_inline arg(int i)           : type(INT32),  val(i) {}
	hogl_force_inline arg(unsigned long long u) : type(UINT64), val(u) {}
	hogl_force_inline arg(long long i)     : type(INT64),  val(i) {}

#if ULONG_MAX == UINT_MAX
	// 32bit arch
	hogl_force_inline arg(unsigned long u) : type(UINT32), val(u) {}
	hogl_force_inline arg(long i)          : type(INT32),  val(i) {}
#else
	// 64bit arch
	hogl_force_inline arg(unsigned long u) : type(UINT64), val(u) {}
	hogl_force_inline arg(long i)          : type(INT64),  val(i) {}
#endif

	hogl_force_inline arg(double d) : type(DOUBLE)
	{
		union { double d; uint64_t u; } u;
		u.d = d;
		val = u.u;
	}

	// For static strings the call to strlen() is resolved at compile time
	hogl_force_inline arg(const char *str) :
		type(CSTR), val((uint64_t) str), len(str ? strlen(str) : 0) {}

	hogl_force_inline arg(const std::string &str) :
		type(CSTR), val((uint64_t) str.data()), len(str.length()) {}

	hogl_force_inline arg(const arg_gstr &s) : type(GSTR), val((uint64_t) s.str) {}

	hogl_force_inline arg(const arg_hexdump &hd) :
		type(HEXDUMP), val((uint64_t) hd.ptr), len(hd.len) {}

	hogl_force_inline arg(const arg_raw &raw) :
		type(RAW), val((uint64_t) raw.ptr), len(raw.len) {}
};

} // namespace hogl

#endif // HOGL_DETAIL_ARGS_HPP
