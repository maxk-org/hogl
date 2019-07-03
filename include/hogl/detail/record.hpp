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
 * @file hogl/detail/record.hpp
 * Log record implementationn details.
 */
#ifndef HOGL_DETAIL_RECORD_HPP
#define HOGL_DETAIL_RECORD_HPP

#include <stdint.h>
#include <string.h>

#include <hogl/detail/timestamp.hpp>
#include <hogl/detail/area.hpp>
#include <hogl/detail/args.hpp>
#include <hogl/detail/argpack.hpp>
#include <hogl/detail/preproc.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

class area;

/**
 * Log record.
 */
struct record {
	enum { NARGS = 16 };

	const hogl::area *area;
	hogl::timestamp   timestamp;
	uint64_t seqnum  :52;
	uint64_t section :12;
	uint64_t argtype;
	union {
		uint64_t u64;
		uint32_t u32;
		struct {
			uint32_t offset;
			uint32_t len;
		} data;
	} argval[NARGS];

	// Note that this layout is important.
	// It ensures that the record with 4 arguments fits nicely into
	// the 64 byte cacheline.

	/**
	 * Get the size of the record header portion.
	 * i.e. excluding the argument storage. 
	 */
	static unsigned int header_size() { return sizeof(record) - sizeof(uint64_t) * NARGS; }

	/**
 	 * Set argument type
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
 	 * Clear argument type
 	 * @param i index (position) of the argument (zero based)
 	 */
	void clear_arg_type(unsigned int i)
        {
		argtype &= ~((uint64_t) 0xf << (i * 4));
        }

       	/**
 	 * Set argument value.
 	 * @param i index (position) of the argument (zero based)
 	 * @param val argument value
 	 */
	void set_arg_val32(unsigned int i, uint32_t val)
	{
		argval[i].u32 = val;
	}

	/**
 	 * Set argument value.
 	 * @param i index (position) of the argument (zero based)
 	 * @param val argument value
 	 */
	void set_arg_val64(unsigned int i, uint64_t val)
	{
		argval[i].u64 = val;
	}

	/**
	 * Set argument data (encoded in value)
 	 * @param i index (position) of the argument (zero based)
 	 * @param offset offset in the record buffer
 	 * @param len size of the data in bytes
	 */
	void set_arg_data(unsigned int i, uint32_t offset, uint32_t len)
	{
		argval[i].data.offset = offset;
		argval[i].data.len = len;
	}

	/**
 	 * Get argument 32bit value 
 	 * @param i index (position) of the argument (zero based)
 	 */
	uint32_t get_arg_val32(unsigned int i) const
	{
		return argval[i].u32;
	}

	/**
 	 * Get argument 64bit value 
 	 * @param i index (position) of the argument (zero based)
 	 */
	uint64_t get_arg_val64(unsigned int i) const
	{
		return argval[i].u64;
	}

	/**
 	 * Get argument data.
 	 * @param i index (position) of the argument (zero based)
 	 * @param len [out] data length.
	 * @return pointer to argument data
	 * @warn This function works only with arguments that have data: CSTR, HEXDUMP, RAW. \
	 *       It does not validate argument type for performance reasons.
 	 */
	const uint8_t *get_arg_data(unsigned int i, unsigned int &len) const
	{
		len = argval[i].data.len;
		return (const uint8_t*) this->argval + argval[i].data.offset;
	}

	/**
	 * Copy cstr into the record.
	 * Adds truncation marker if the string is longer than the available room.
	 * Copied string is guarantied to be null-terminated.
	 * @param i argument index
	 * @param str pointer to the source string
	 * @param len length of the source string
	 * @param tailroom amount of tailroom left in the record buffer
	 * @param offset offset in the record buffer
	 * @return number of bytes used in the record buffer
	 */
	unsigned int copy_cstr(unsigned int i, const uint8_t *str, unsigned int len, unsigned int tailroom, unsigned int offset)
	{
		unsigned int room = tailroom - offset;
		if (!len || !room) {
			set_arg_data(i, 0, 0);
			return 0;
		}

		--room; // Save room for null-terminator

		uint8_t *dst = (uint8_t *) this->argval + offset;
		unsigned int n = len; // Copy len
		if (n > room) {
			// Doesn't fit. Add truncation marker (if possible)
			n = len = room;
			if (n > 3) {
				n -= 3;
				memcpy(dst + n, ">>>", 3);
			}
		}
		memcpy(dst, str, n);
		dst[len] = '\0';

		set_arg_data(i, offset, len);
		return len + 1;
	}

	/**
	 * Copy data into the record.
	 * @param i argument index
	 * @param data pointer to the data buffer
	 * @param len length of the data
	 * @param tailroom amount of tailroom left in the record buffer
	 * @param offset offset in the record buffer
	 * @return number of bytes used in the record buffer
	 */
	unsigned int copy_data(unsigned int i, const uint8_t *data, unsigned int len, unsigned int tailroom, unsigned int offset)
	{
		unsigned int room = tailroom - offset;
		len = len > room ? room : len;

		memcpy((uint8_t *) this->argval + offset, (const void *) data, len);

		set_arg_data(i, offset, len);
		return len;
	}

	/**
 	 * Set record argument
 	 * @param i index (position) of the argument (zero based)
 	 * @param a argument itself
 	 * @param tailroom number of bytes available at the tail of the record (used as generic buffer).
 	 * @param offset offset in the buffer (used for packing strings and complex args)
 	 */
	hogl_force_inline void set_arg(unsigned int i, const hogl::arg a, unsigned int tailroom, unsigned int &offset)
	{
		// In case of the fully inlined version (ie when an entire record posting stack
		// is inlined) a.type is known at compile time, which means all type checks 
		// are resolved at compile time.

		if (a.type == a.NONE)
			return;

		set_arg_type(i, a.type);

		if (a.type == a.HEXDUMP || a.type == a.RAW) {
			offset += copy_data(i, (const uint8_t *) a.val, a.len, tailroom, offset);
			return;
		}

		if (a.type == a.CSTR) {
			offset += copy_cstr(i, (const uint8_t *) a.val, a.len, tailroom, offset);
			return;
		}

		if (a.is_32bit())
			set_arg_val32(i, a.val);
		else
			set_arg_val64(i, a.val);
	}

	/**
 	 * Populate record arguments
 	 * @param tailroom number of bytes available at the tail of the record (used as generic buffer).
 	 * @param arg0 ... arg1 argument list
 	 */
	hogl_force_inline void set_args(unsigned int tailroom, __hogl_long_arg_list(16))
	{
		// Compute the number of valid arguments (resolved at compile time)
		unsigned int n = 0;
		#define __hogl_record_valid_arg(i) n += (a##i.type != arg::NONE)
		HOGL_PP_REPS(16, __hogl_record_valid_arg, ;);

		// Compute offset based on the number of valid arguments
		unsigned int offset = n * sizeof(uint64_t);

		// Populate arguments
		argtype = 0;
		#define __hogl_record_set_arg(i) set_arg(i, a##i, tailroom, offset)
		HOGL_PP_REPS(16, __hogl_record_set_arg, ;);
	}

	/**
 	 * Populate record arguments
 	 * @param tailroom number of bytes available at the tail of the record (used as generic buffer).
 	 * @param ap reference to the pre-processed and packed arguments
 	 */
	hogl_force_inline void set_args(unsigned int tailroom, const argpack &ap)
	{
		unsigned int n = ap.nargs();
		unsigned int offset = n * sizeof(uint64_t);

		argtype = ap.argtype;
		for (unsigned int i=0; i < n; i++) {
			unsigned int type = ap.get_arg_type(i);
			const uint8_t *data; unsigned int len;

			if (type == arg::HEXDUMP || type == arg::RAW) {
				data = ap.get_arg_data(i, len);
				offset += copy_data(i, data, len, tailroom, offset);
			} else if (type == arg::CSTR) {
				data = ap.get_arg_data(i, len);
				offset += copy_cstr(i, data, len, tailroom, offset);
			} else
				set_arg_val64(i, ap.get_arg_val64(i));
		}
	}

	/**
 	 * Clear the record
 	 */
	void clear()
	{
		area      = 0;
		timestamp = 0;
		seqnum    = 0;
		section   = 0;
		argtype   = 0;
	}

	/**
 	 * Populate special record
 	 */
	void special(uint32_t type, uint64_t arg1 = 0, uint64_t arg2 = 0)
	{
		area    = 0;
		section = 0xfff;
		argtype = type;
		argval[0].u64 = 0;
		argval[1].u64 = arg1;
		argval[2].u64 = arg2;
	}

	/**
	 * Check for special record
	 * @return true if the record is special, false otherwise
	 */
	bool special()
	{
		if (hogl_likely(area != 0 || section != 0xfff))
			return false;
		return true;
	}

	/**
	 * Acknowledge special record
	 */
	void ack()
	{
		volatile uint64_t *a0 = &argval[0].u64;
		*a0 = 0x7e7f;
	}

	/**
 	 * Check if the record was acked
 	 * @return true if the record was acked, false otherwise
 	 */
	bool acked() const
	{
		volatile const uint64_t *a0 = &argval[0].u64;
		return *a0 == 0x7e7f;
	}

	/**
 	 * Construct new record. 
 	 */
	record() { clear(); }
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_RECORD_HPP
