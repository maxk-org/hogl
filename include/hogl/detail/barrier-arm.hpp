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
 * @file hogl/detail/barrier-arm.hpp
 * ARM v7 and v8 specific barriers.
 * @warning do not include directly. @see hogl/detail/barrier.hpp
 */

// Memory barriers
// This version requires ARM >= V7 capable CPU.
// For more info see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka14041.html

static hogl_force_inline void memrw() { asm volatile("dmb sy" : : : "memory"); }
static hogl_force_inline void memw()  { asm volatile("dmb st" : : : "memory"); }

#if (defined(__aarch64__))
static hogl_force_inline void memr()  { asm volatile("dmb ld" : : : "memory"); }
#else
// Unfortunately 'dmb ld' is not supported on ARM V7
static hogl_force_inline void memr()  { asm volatile("dmb sy" : : : "memory"); }
#endif
