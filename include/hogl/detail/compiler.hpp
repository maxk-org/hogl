/*
   Copyright (c) 2015-2020 Max Krasnyansky <max.krasnyansky@gmail.com> 
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
 * @file hogl/detail/compiler.h
 * Compiler related macros, shortcuts, etc.
 */

#ifndef HOGL_DETAIL_COMPILER_HPP
#define HOGL_DETAIL_COMPILER_HPP

#include <hogl/detail/privns.hpp>

// Note: Currently supported compilers are:
// 	GCC 4.0 and above
// 	Intel C++ compiler
// 	Clang 3.0 and above
// FIXME: switch to C++11 attributes

// Branch likelihood hints
#define hogl_likely(x)    __builtin_expect(!!(x), 1)
#define hogl_unlikely(x)  __builtin_expect(!!(x), 0)

// Forced inlining 
#define hogl_force_inline inline __attribute__ ((always_inline))

// Don't warn about unused function
#define hogl_weak_symbol __attribute__ ((weak))

// Packed structs and unaligned access
#define hogl_packed __attribute__ ((packed))

// Frequently used function
#ifdef __clang__
#define hogl_hot_path
#else 
#define hogl_hot_path __attribute__ ((hot))
#endif // clang

#endif // HOGL_DETAIL_COMPILER_HPP
