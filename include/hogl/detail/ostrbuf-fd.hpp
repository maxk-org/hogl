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
 * @file hogl/detail/ostrbuf-fd.h
 * String buffer with output to a file descriptor
 */

#ifndef HOGL_DETAIL_OSTRBUF_FD_HPP
#define HOGL_DETAIL_OSTRBUF_FD_HPP

#include <hogl/detail/ostrbuf.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * String buffer with output to file descriptor
 */
class ostrbuf_fd : public ostrbuf
{
private:
	unsigned int _flags;
	int _fd;

	virtual void do_flush(const uint8_t *data, size_t len);

public:
	enum Flags {
		CLOSE_ON_DELETE = (1<<0) // Close the file descriptor on delete
	};

	// Get file descriptor value
	int fd() const { return _fd; }

	// Explicitly close the file descriptor
	void close(); 

	/**
	 * Constructor.
	 * @param fd file descriptor
	 * @param flags @see ostrbuf_fd::Flags
	 * @param buffer_capacity capacity of the data buffer
	 */ 
	ostrbuf_fd(int fd, unsigned int flags = 0, unsigned int buffer_capacity = 8192);

	// Destructor
	virtual ~ostrbuf_fd();
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_OSTRBUF_FD_HPP
