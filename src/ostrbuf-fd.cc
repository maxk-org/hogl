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

#define _XOPEN_SOURCE 700

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>

#include <string>

#include "hogl/detail/ostrbuf-fd.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

void ostrbuf_fd::do_flush(const uint8_t *data, size_t len)
{
	if (failed())
		return;

	struct iovec iov[2];
	unsigned int i = 0;

	// Flush buffered data
	if (_size) {
		iov[i].iov_base = (void *) _data;
		iov[i].iov_len  = _size;
		i++;
	}

	// Flush un-buffered data
	if (len) {
		iov[i].iov_base = (void *) data;
		iov[i].iov_len  = len;
		i++;
	}

	if (i) {
		int r = ::writev(_fd, iov, i);
		if (r < 0)
			ostrbuf::failure(strerror(errno));
	}

	this->reset();
}

void ostrbuf_fd::close()
{
	::close(_fd); _fd = -1;
}

ostrbuf_fd::ostrbuf_fd(int fd, unsigned int flags, unsigned int buffer_capacity) :
	ostrbuf(buffer_capacity), _flags(flags), _fd(fd)
{
	// FIXME: Enforce blocking IO on the provided FD
}

ostrbuf_fd::~ostrbuf_fd()
{
	if (_flags & CLOSE_ON_DELETE) {
		::close(_fd);
		_fd = -1;
	}
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

