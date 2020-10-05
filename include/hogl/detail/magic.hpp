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
 * @file hogl/detail/magic.h
 * Magic signatures for the HOGL objects.
 */
#ifndef HOGL_DETAIL_MAGIC_HPP
#define HOGL_DETAIL_MAGIC_HPP

#include <hogl/detail/compiler.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {
	/**
 	 * Magic signatures used for locating various HOGL objects in the coredumps.
 	 * These signatures must be updated when the layout of the coresponding
 	 * structure changes.
 	 */
	union magic {
		uint32_t u32[4];
		uint64_t u64[2];
	};

	/**
	 * Engine magic number
	 */
	static const magic engine_magic = {
		{ 0xe10f0f56, 0xba8a7dc8, 0xdd223344, 0x51691388 }
	};

	/**
	 * Ring magic number
	 */
	static const magic ring_magic = { 
		{ 0xe1272734, 0xba8a7dc8, 0xdd223344, 0x51691388 }
	};

	/**
	 * Area magic number
	 */
	static const magic area_magic = { 
		{ 0xe129000f, 0xba8a7dc8, 0xdd223344, 0x51691388 }
	};

	/**
	 * Output magic number
	 */
	static const magic output_magic = {
		{ 0xe121200f, 0xba8a7dc8, 0xdd223344, 0x51691388 }
	};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_MAGIC_HPP
