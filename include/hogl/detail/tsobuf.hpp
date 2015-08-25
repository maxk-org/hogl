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
 * @file hogl/detail/tsobuf.h
 * Time stamp ordering buffer
 */
#ifndef HOGL_DETAIL_TSOBUF_HPP
#define HOGL_DETAIL_TSOBUF_HPP

#include <stdint.h>
#include <algorithm>

#include <hogl/detail/timestamp.hpp>
#include <hogl/detail/ringbuf.hpp>

namespace hogl {

/**
 * Timestamp ordering buffer.
 * Used for reordering log records in the engine.
 */
class tsobuf {
public:
	// TSO buffer entry
	struct entry {
		hogl::timestamp timestamp;
		record      *rec; // Pointer to the record 
		unsigned int tag; // Tag
	};

private:
	unsigned int _head;
	unsigned int _tail;
	unsigned int _capacity;
	entry       *_entry;

	// Less operator for sorting
	struct less {
		bool operator() (const entry &i, const entry &j)
		{
			return i.timestamp < j.timestamp;
		}
	};
	static less _less;

public:
	void resize(unsigned int n)
	{
		delete [] _entry;
		_capacity = n;
		_entry = new entry[n];
		_head  = 0;
		_tail  = 0;
	}

	void clear()
	{
		_head = 0;
		_tail = 0;
	}

	void reset() { clear(); }

	unsigned int capacity() const { return _capacity; }
	unsigned int size() const { return _tail - _head; }
	bool full() const { return _tail == _capacity; }
	bool empty() const { return _head >= _tail; }

	/**
	 * Push entry
	 * @warn Does not check for available room. The assumption is that 
	 * the caller checked for !full() condition first. This is done
	 * for performance reasons.
	 */
	void push(entry &e)
	{
		_entry[_tail++] = e;
	}

	/**
	 * Pop entry
	 */
	bool pop(entry &e)
	{
		if (empty())
			return false;
		e = _entry[_head++];
		return true;
	}

	/**
	 * Get top entry without removing it
	 */
	bool top(entry &e)
	{
		if (empty())
			return false;
		e = _entry[_head];
		return true;
	}

	/**
	 * Flip TSO buffer.
	 * All records between head and tail are shifted up and head is reset
	 * to zero.
	 */
	void flip()
	{
		unsigned int s = size();
		memmove(_entry, _entry + _head, s * sizeof(entry));
		_head = 0;
		_tail = s;
	}

	void sort()
	{
		std::sort(_entry + _head, _entry + _tail, _less);
	}

	tsobuf(unsigned int n = 1) : _head(0), _tail(0), _entry(0)
	{
		resize(n);
	}

	tsobuf(const tsobuf &t) : _head(0), _tail(0), _entry(0)
	{
		resize(t._capacity);
		memcpy(_entry, t._entry, _capacity * sizeof(entry));
	}

	~tsobuf()
	{
		delete [] _entry;
	}
};

} // namespace hogl

#endif // HOGL_DETAIL_TSOBUF_HPP
