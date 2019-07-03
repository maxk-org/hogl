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

#ifndef HOGL_RDBUF_HPP
#define HOGL_RDBUF_HPP

#include <hogl/detail/compiler.hpp>

#include <stdint.h>
#include <string>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// Simple buffered reader interface
class rdbuf
{
public:
	virtual ~rdbuf() {}

	// Check if rdbuf is valid
	virtual bool valid() const = 0;

	// Check if the last operation failed
	virtual bool fail()  const = 0;

	// Get last error code (errno)
	virtual int  error() const = 0;

	// Reset rfbuf
	virtual void reset() = 0;

	// Read n bytes
	// Returns true of all bytes could be read and false otherwise
	virtual bool read(void *ptr, unsigned int n) = 0;
};

// Simple buffered file reader.
// It's faster than stdio and std::iostream but more importantly
// it's better suite to handle 'tailf' and other modes needed for
// things like parsing raw records from files and pipes.
class file_rdbuf : public rdbuf
{
private:
	uint8_t     *_data;     // data buffer
	unsigned int _capacity; // buffer capacity 
	unsigned int _head;     // head of available data
	unsigned int _size;     // size of available data

	unsigned int _flags;    // rdbuf flags
	int          _fd;       // file descriptor
	int          _ifd;      // inotify file descriptor
	int          _errno;    // last errno value

	// Copy buffered data out
	void copy_out(uint8_t* &dst, unsigned int &len);

	// Read next data chunk (up to capacity)
	bool read_in();

public:
	enum flags {
		CLOSE_ON_DELETE = (1<<0), // close file descriptor when rdbuf is deleted
		TAILF		= (1<<1), // follow the tail (don't fail on zero reads)
	};

	bool valid() const { return _fd != -1; }
	bool fail()  const { return _errno != 0; }
	int  error() const { return _errno; }

	uint8_t* head() { return _data + _head; }
	unsigned int  size() { return _size; }

	void reset()
	{
		_head = _size = 0;
		_errno = 0;
	}

	file_rdbuf(const std::string &filename, unsigned int flags = 0, unsigned int capacity = 4096);
	~file_rdbuf();

	bool read(void *ptr, unsigned int len);
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_RDBUF_HPP
