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
 * @file hogl/detail/barrier.hpp
 * Compiler and memory barriers.
 */

#ifndef HOGL_DETAIL_BARRIER_HPP
#define HOGL_DETAIL_BARRIER_HPP

#include <hogl/detail/compiler.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {
namespace barrier {

// We're using inline function here instead of #defines to avoid 
// name space clashes.

/**
 * Compiler barrier
 */
static hogl_force_inline void compiler() { asm volatile("": : :"memory"); }

#if (defined(__i386__) || defined(__x86_64__))
#include <hogl/detail/barrier-x86.hpp>
#elif (defined(__ARM_ARCH_7A__)) || (defined(__aarch64__))
#include <hogl/detail/barrier-arm.hpp>
#elif (defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) || \
      defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__))
#include <hogl/detail/barrier-powerpc.hpp>
#else
#error Unsupported CPU
#endif

} // namespace barrier
} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_BARRIER_HPP
