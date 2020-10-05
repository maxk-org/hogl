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
 * @file hogl/detail/bitmap.hpp
 * Simple, bare minimum version of boost::dynamic_bitset.
 */

#ifndef HOGL_DETAIL_BITMAP_HPP
#define HOGL_DETAIL_BITMAP_HPP

#include <hogl/detail/compiler.hpp>

#include <stdint.h>
#include <string.h>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Simple, bare minimum version of boost::dynamic_bitset.
 * Implemented to avoid dependency on boost.
 */
class bitmap {
protected:
	#if __WORDSIZE == 64
	typedef uint64_t word;
	#else
	typedef uint32_t word;
	#endif

	enum {
		word_size = sizeof(word) * 8,
		byte_size = 8,
	};

	word        *_data;
	unsigned int _size;

	unsigned int nwords(unsigned int s) const
	{
		return (s + word_size - 1) / word_size;
	}
	unsigned int nbytes(unsigned int s) const
	{
		return (s + byte_size - 1) / byte_size;
	}
	unsigned int which_word(unsigned int b) const
	{
		return b / word_size;
	}
	unsigned int which_bit(unsigned int b) const
	{
		return b & (word_size - 1);
	}

public:
	void resize(unsigned int size)
	{
		delete [] _data;
		_size = size;
		_data = new word[nwords(size)];
	}

	void set()
	{
		memset(_data, 0xff, nbytes(_size));
	}

	void set(unsigned int bit, bool val = true)
	{
		if (val)
			_data[which_word(bit)] |= ((word)1 << which_bit(bit));
		else
			_data[which_word(bit)] &= ~((word)1 << which_bit(bit));
	}

	void reset()
	{
		memset(_data, 0, nbytes(_size));
	}

	void reset(unsigned int bit)
	{
		set(bit, 0);
	}

	bool test(unsigned int bit) const
	{
		return _data[which_word(bit)] & (1 << which_bit(bit));
	}

	unsigned int size() const { return _size; }

	bitmap(unsigned int size = 1) : _data(0)
	{
		resize(size);
		reset();
	}

	bitmap(const bitmap &b) : _data(0)
	{
		resize(b._size);
		memcpy(_data, b._data, nbytes(_size));
	}

	~bitmap()
	{
		delete [] _data;
	}
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_BITMAP_HPP
