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
 * @file hogl/detail/ostrbuf.h
 * Efficient output stream buffer for record formatting
 */
#ifndef HOGL_DETAIL_OSTRBUF_HPP
#define HOGL_DETAIL_OSTRBUF_HPP

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include <hogl/detail/compiler.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Efficient output stream buffer for record formatting.
 * All put() operations do a flush when the buffer gets full.
 * The underlying streams are expected to be blocking (flush is a blocking operation).
 * The data is never truncated, except for catastrofic failures of the underlying 
 * sream (disk full, closed pipe, etc).
 * Failed streams cannot be reused (ie there is no way to clear failure condition).
 */
class ostrbuf {
protected:
	uint8_t*      _data;     // Data buffer
	unsigned int  _capacity; // Buffer capacity
	unsigned int  _size;     // Current buffer size
	FILE*         _stdio;    // Pointer to a virtual stdio stream for this ostrbuf
	volatile bool _failed;   // Failure flag
	char          _error[64];// Error message

	// Get tail of the data buffer.
	// Used internally for appending data.
	uint8_t *tail() { return _data + _size; }

	// Get amount of available room in the buffer.
	size_t room() const { return _capacity - _size; }

	// Copy data into the buffer.
	// Caller must check for room.
	void do_copy(const uint8_t* data, size_t n)
	{
		memcpy(tail(), data, n);
		_size += n;
	}

	/**
	 * Indicate a catastrofic failure of the underlying stream.
	 * @param err error message that explains failure cause
	 */
	void failure(const char *err);

	/**
	 * Flush buffered and new data to the underlying stream.
	 * Overriden in the derived classes.
	 * This operation is expected to be blocking.
	 * Data is never truncated except for the catastrofic failures (disk full, closed pipe, etc).
	 * This method must call @see failure() method in case of such failures.
	 * @param data pointer to new data that does not fit in the buffer (maybe null)
	 * @param n number of new data bytes (maybe zero)
	 */
	virtual void do_flush(const uint8_t* data, size_t n) = 0;

public:
	/**
	 * Constructor
	 * @param capacity capacity of the data buffer; zero if no buffering is required
	 * @param nostdio do not provide virtual stdio stream (saves memory)
	 */
	ostrbuf(size_t capacity = 1024, bool nostdio = false);

	/**
	 * Destructor
	 */
	virtual ~ostrbuf();

	// Check if the underlying stream has failed
	bool failed() const { return _failed; }

	// Get error message that explains stream failure cause
	const char* error() const { return _error; }

	// Get read-only pointer to the head of the data buffer
	const uint8_t *head() const { return _data; }

	// Get capacity of the data buffer
	size_t capacity() const { return _capacity; }

	// Get current size of the 
	size_t size() const { return _size; }

	// Reset data buffer.
	// All buffered & not yet flushed data will be dropped.
	void reset() { _size = 0; }

	// Flush any buffered data
	void flush() { do_flush(0, 0); }

	// Put (append) data into the buffer
	void put(const uint8_t* data, size_t n)
	{
		if (n > room())
			return do_flush(data, n);
		do_copy(data, n);
	}

	// Put (append) a string into the buffer
	void cat(const char *str, size_t len)
	{
		put((const uint8_t *) str, len);
	}

	// Put (append) a string into the buffer
	void cat(const char *str)
	{
		cat(str, strlen(str));
	}

	// Put (append) a string into the buffer
	void cat(const std::string &str)
	{
		cat(str.data(), str.length());
	}

	// Put (append) a single character 
	void cat(char c)
	{
		cat(&c, 1);
	}

	// Put (append) formated string into the buffer
	void printf(const char* fmt, ...);

	// Get a pointer to the stdio stream handle.
	// The stdio handle can be passed to fprintf() and other functions
	// for putting (append) data into the buffer.
	FILE* stdio() { return _stdio; }

private:
	// No copies
	ostrbuf(const ostrbuf&);
	ostrbuf& operator=( const ostrbuf& );
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_OSTRBUF_HPP
