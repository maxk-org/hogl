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
 * @file hogl/detail/ntos.h
 * Simple number to string conversion helpers
 */
#ifndef HOGL_DETAIL_NTOS_HPP
#define HOGL_DETAIL_NTOS_HPP

#include <hogl/detail/compiler.hpp>

#include <stdint.h>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// Simple "uint64_t to hex string" converter
// The caller must ensure that the destination string has enough room
static inline void u64tox(uint64_t v, uint8_t *str, unsigned int &i, unsigned int width = 0, char pad = '0')
{
	const char hexmap[] = "0123456789abcdef";
	unsigned int d = 0;
	uint8_t  digit[32];

	// Generate all digits (reverse order)
	do { digit[d++] = hexmap[v & 0xf]; } while ((v >>= 4));

	// Pad if needed
	while (d < width) digit[d++] = pad;

	// Copy into the strbuf (correct order)
	do str[i++] = digit[--d]; while (d);
}

// Simple "uint64_t to decimal string" converter
// The caller must ensure that the destination string has enough room
static inline void u64tod(uint64_t v, uint8_t *str, unsigned int &i, unsigned int width = 0, char pad = '0')
{
	unsigned int d = 0;
	uint8_t digit[32];

	if (width > sizeof(digit))
		width = sizeof(digit);

	// Generate all digits (reverse order)
	do digit[d++] = (v % 10) + '0'; while ((v /= 10));

	// Pad if needed
	while (d < width) digit[d++] = pad;

	// Copy into the strbuf (correct order)
	do str[i++] = digit[--d]; while (d);
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_NTOS_HPP
