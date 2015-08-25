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
 * ARM specific barriers.
 * @warning do not include directly. @see hogl/detail/barrier.hpp
 */

// Memory barriers
// This version requires ARM >= V7 capable CPU.
// For more info see: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka14041.html

static hogl_force_inline void dmb() { asm volatile("dmb" : : : "memory"); }
static hogl_force_inline void dsb() { asm volatile("dsb" : : : "memory"); }
static hogl_force_inline void isb() { asm volatile("isb" : : : "memory"); }

static hogl_force_inline void memrw() { dmb(); }
static hogl_force_inline void memr()  { dmb(); }
static hogl_force_inline void memw()  { dmb(); }

// These might be useful for synchronization primitives.
// Unused in HOGL at this point.
static hogl_force_inline void sev() { asm volatile("sev" : : : "memory"); }
static hogl_force_inline void wfe() { asm volatile("wfe" : : : "memory"); }
static hogl_force_inline void wfi() { asm volatile("wfi" : : : "memory"); }
