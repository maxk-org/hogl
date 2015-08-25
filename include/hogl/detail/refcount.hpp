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
 * @file hogl/detail/refcount.h
 * Simple atomic refcount.
 */
#ifndef HOGL_DETAIL_REFCOUNT_HPP
#define HOGL_DETAIL_REFCOUNT_HPP

namespace hogl {

/**
 * Simple atomic refcount.
 */
class refcount {
private:
	mutable volatile int _count;

public:
	// FIXME: Uses GCC builtins without checking for GCC

	/**
 	 * Increment reference count.
 	 * @param v value to increment by
 	 * @return new value of the counter
 	 */
	int inc(int v = 1)
	{
		return __sync_add_and_fetch(&_count, v);
	}

	/**
	 * Decrement reference count.
 	 * @param v value to decrement by
 	 * @return new value of the counter
	 */
	int dec(int v = 1)
	{
		return __sync_sub_and_fetch(&_count, v);
	}

	/**
	 * Set reference count value.
 	 * @param v value to decrement by
 	 * @return previous value of the counter
	 */
	int set(int v)
	{
		return _count = v;
	}

	/**
	 * Get reference count value.
 	 * @return counter value
	 */
	int get() const
	{
		return __sync_add_and_fetch(&_count, 0);
	}

	/**
	 * Initialize refcount 
 	 * @param v initial value
 	 */
	refcount(int c = 0) : _count(c) { }

	/**
	 * Initialize refcount 
 	 * @param r initial value
 	 */
	refcount(const refcount &r) { set(r.get()); }
};

} // namespace hogl

#endif // HOGL_DETAIL_REFCOUNT_HPP
