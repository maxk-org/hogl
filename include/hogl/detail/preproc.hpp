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
 * @file hogl/detail/preproc.h
 * C/C++ proprocessor tricks used to generate repeated declarations
 */
#ifndef HOGL_DETAIL_PREPROC_HPP
#define HOGL_DETAIL_PREPROC_HPP

// Repeat 'p' 'n' times separated by comma
#define HOGL_PP_REPC_16(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9), p(10), p(11), p(12), p(13), p(14), p(15)
#define HOGL_PP_REPC_15(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9), p(10), p(11), p(12), p(13), p(14)
#define HOGL_PP_REPC_14(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9), p(10), p(11), p(12), p(13)
#define HOGL_PP_REPC_13(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9), p(10), p(11), p(12)
#define HOGL_PP_REPC_12(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9), p(10), p(11)
#define HOGL_PP_REPC_11(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9), p(10)
#define HOGL_PP_REPC_10(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8), p(9)
#define HOGL_PP_REPC_9(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7), p(8)
#define HOGL_PP_REPC_8(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6), p(7)
#define HOGL_PP_REPC_7(p) p(0), p(1), p(2), p(3), p(4), p(5), p(6)
#define HOGL_PP_REPC_6(p) p(0), p(1), p(2), p(3), p(4), p(5) 
#define HOGL_PP_REPC_5(p) p(0), p(1), p(2), p(3), p(4)
#define HOGL_PP_REPC_4(p) p(0), p(1), p(2), p(3) 
#define HOGL_PP_REPC_3(p) p(0), p(1), p(2)
#define HOGL_PP_REPC_2(p) p(0), p(1) 
#define HOGL_PP_REPC_1(p) p(0)
#define HOGL_PP_REPC_0(p)
#define HOGL_PP_REPC(n, p) HOGL_PP_REPC_##n (p)

// Repeat 'p' 'n' times separated by 's'
#define HOGL_PP_REPS_16(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)s p(10)s p(11)s p(12)s p(13)s p(14)s p(15)
#define HOGL_PP_REPS_15(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)s p(10)s p(11)s p(12)s p(13)s p(14)
#define HOGL_PP_REPS_14(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)s p(10)s p(11)s p(12)s p(13)
#define HOGL_PP_REPS_13(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)s p(10)s p(11)s p(12)
#define HOGL_PP_REPS_12(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)s p(10)s p(11)
#define HOGL_PP_REPS_11(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)s p(10)
#define HOGL_PP_REPS_10(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)s p(9)
#define HOGL_PP_REPS_9(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)s p(8)
#define HOGL_PP_REPS_8(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)s p(7)
#define HOGL_PP_REPS_7(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)s p(6)
#define HOGL_PP_REPS_6(p, s) p(0)s p(1)s p(2)s p(3)s p(4)s p(5)
#define HOGL_PP_REPS_5(p, s) p(0)s p(1)s p(2)s p(3)s p(4)
#define HOGL_PP_REPS_4(p, s) p(0)s p(1)s p(2)s p(3)
#define HOGL_PP_REPS_3(p, s) p(0)s p(1)s p(2)
#define HOGL_PP_REPS_2(p, s) p(0)s p(1)
#define HOGL_PP_REPS_1(p, s) p(0)
#define HOGL_PP_REPS_0(p, s)
#define HOGL_PP_REPS(n, p, s) HOGL_PP_REPS_##n (p, s)

// Repeat 'p' 'n' times separated by whitespace
#define HOGL_PP_REP_16(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10) p(11) p(12) p(13) p(14) p(15) p(16)
#define HOGL_PP_REP_15(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10) p(11) p(12) p(13) p(14) p(15)
#define HOGL_PP_REP_14(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10) p(11) p(12) p(13) p(14)
#define HOGL_PP_REP_13(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10) p(11) p(12) p(13)
#define HOGL_PP_REP_12(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10) p(11) p(12)
#define HOGL_PP_REP_11(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10) p(11)
#define HOGL_PP_REP_10(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9) p(10)
#define HOGL_PP_REP_9(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8) p(9)
#define HOGL_PP_REP_8(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7) p(8)
#define HOGL_PP_REP_7(p) p(1) p(2) p(3) p(4) p(5) p(6) p(7)
#define HOGL_PP_REP_6(p) p(1) p(2) p(3) p(4) p(5) p(6)
#define HOGL_PP_REP_5(p) p(1) p(2) p(3) p(4) p(5)
#define HOGL_PP_REP_4(p) p(1) p(2) p(3) p(4)
#define HOGL_PP_REP_3(p) p(1) p(2) p(3)
#define HOGL_PP_REP_2(p) p(1) p(2) 
#define HOGL_PP_REP_1(p) p(1)
#define HOGL_PP_REP_0(p)
#define HOGL_PP_REP(n, p) HOGL_PP_REP_##n (p)

// Concatenate two arguments
#define HOGL_PP_CONCAT(arg1, arg2)  arg1##arg2

// Compute the number of arguments to a macro (up to 16)
#define HOGL_PP_NARGS(...) HOGL_PP_NARGS_(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define HOGL_PP_NARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, n, ...) n

// Repeat 'w' for each argument
#define HOGL_PP_FOR_EACH_1(w, x) w(x)
#define HOGL_PP_FOR_EACH_2(w, x, ...)  w(x); HOGL_PP_FOR_EACH_1(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_3(w, x, ...)  w(x); HOGL_PP_FOR_EACH_2(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_4(w, x, ...)  w(x); HOGL_PP_FOR_EACH_3(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_5(w, x, ...)  w(x); HOGL_PP_FOR_EACH_4(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_6(w, x, ...)  w(x); HOGL_PP_FOR_EACH_5(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_7(w, x, ...)  w(x); HOGL_PP_FOR_EACH_6(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_8(w, x, ...)  w(x); HOGL_PP_FOR_EACH_7(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_9(w, x, ...)  w(x); HOGL_PP_FOR_EACH_8(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_10(w, x, ...) w(x); HOGL_PP_FOR_EACH_9(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_11(w, x, ...) w(x); HOGL_PP_FOR_EACH_10(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_12(w, x, ...) w(x); HOGL_PP_FOR_EACH_11(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_13(w, x, ...) w(x); HOGL_PP_FOR_EACH_12(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_14(w, x, ...) w(x); HOGL_PP_FOR_EACH_13(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_15(w, x, ...) w(x); HOGL_PP_FOR_EACH_14(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_16(w, x, ...) w(x); HOGL_PP_FOR_EACH_15(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH_N(n, w, ...) HOGL_PP_CONCAT(HOGL_PP_FOR_EACH_, n)(w, __VA_ARGS__)
#define HOGL_PP_FOR_EACH(w, ...) HOGL_PP_FOR_EACH_N(HOGL_PP_NARGS(__VA_ARGS__), w, __VA_ARGS__)

// Generate long and short argument lists for various hogl functions
#define __hogl_long_arg(i)  hogl::arg a##i = hogl::arg()
#define __hogl_short_arg(i) a##i
#define __hogl_long_arg_list(i)  HOGL_PP_REPC(i, __hogl_long_arg)
#define __hogl_short_arg_list(i) HOGL_PP_REPC(i, __hogl_short_arg)

#ifdef HOGL_ENABLE_ARG_CHECK
// This check doesn't really do anything special unless the app is running
// under valgrind, in which case it triggers an early warning about conditional jump
// depending on uninitialized variable.
#define __hogl_check_arg(a) ({ if(a.val) hogl::arg_check(a); })
#else
#define __hogl_check_arg(a)
#endif

#endif // HOGL_DETAIL_PREPROC_HPP
