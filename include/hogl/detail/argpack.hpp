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
 * @file hogl/detail/argpack.hpp
 * Argument packing and preprocessing.
 */
#ifndef HOGL_DETAIL_ARGPACK_HPP
#define HOGL_DETAIL_ARGPACK_HPP

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <hogl/detail/args.hpp>
#include <hogl/detail/preproc.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Packed arguments
 */
struct argpack {
	uint64_t argtype;
	union {
		uint64_t u64;
		uint32_t u32;
	} argval[16];
	uint32_t arglen[16];

	/**
	 * Get number of arguments in the pack
	 */
	unsigned int nargs() const
	{
		unsigned int n = 0;
		uint64_t t = argtype;
		while (t) { t = t >> 4; ++n; }
		return n;
	}

	/**
 	 * Set argument type.
 	 * @param i index (position) of the argument (zero based)
 	 * @param type argument type
 	 */
	void set_arg_type(unsigned int i, unsigned int type)
	{
		argtype |= (uint64_t) (type & 0xf) << (i * 4);
	}

	/**
 	 * Get argument type.
 	 * @param i index (position) of the argument (zero based)
 	 * @return a argument reference
 	 */
	unsigned int get_arg_type(unsigned int i) const
	{
		return (argtype >> (i * 4)) & 0xf;
	}

	/**
 	 * Get argument 32-bit value 
 	 * @param i index (position) of the argument (zero based)
 	 */
	uint32_t get_arg_val32(unsigned int i) const
	{
		return argval[i].u32;
	}

	/**
 	 * Get argument 32-bit value 
 	 * @param i index (position) of the argument (zero based)
 	 */
	uint64_t get_arg_val64(unsigned int i) const
	{
		return argval[i].u64;
	}

	/**
 	 * Get argument data
 	 * @param i index (position) of the argument (zero based)
	 * @param [out] len size of the data
	 * @return pointer to the data
 	 */
	const uint8_t *get_arg_data(unsigned int i, unsigned int &len) const
	{
		len = arglen[i];

		#if ULONG_MAX == UINT_MAX
		// 32-bit arch
		return (const uint8_t *) argval[i].u32;
		#else
		// 64-bit arch
		return (const uint8_t *) argval[i].u64;
		#endif
	}

	/**
 	 * Set argument.
 	 * @param i index (position) of the argument (zero based)
 	 * @param a argument itself
	 * @param 0 for simple argument and 1 otherwise
 	 */
	hogl_force_inline unsigned int set_arg(unsigned int i, const hogl::arg a)
	{
		if (a.type == a.NONE)
			return 0;

		__hogl_check_arg(a);

		set_arg_type(i, a.type);

		if (a.is_32bit())
			argval[i].u32 = a.val;
		else
			argval[i].u64 = a.val;

		if (a.is_simple())
			return 0;

		arglen[i] = a.len;
		return 1;
	}

	/**
 	 * Populate argpack arguments
 	 * @param arg0 ... arg1 argument list
	 * @return number of non-simple arguments
 	 */
	hogl_force_inline unsigned int populate(__hogl_long_arg_list(16))
	{
		unsigned int n = 0;
		argtype = 0;
		#define __hogl_argpack_set_arg(i) n += set_arg(i, a##i)
		HOGL_PP_REPS(16, __hogl_argpack_set_arg, ;);
		return n;
	}

	/**
 	 * Clear the record.
 	 */
	void clear()
	{
		argtype = 0;
	}

	/**
 	 * Construct new argpack
 	 */
	argpack() { clear(); }
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_ARGPACK_HPP
