/*
   Copyright (c) 2015-2019 Max Krasnyansky <max.krasnyansky@gmail.com> 
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

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/inotify.h>

#include <algorithm>

#include "rdbuf.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

file_rdbuf::file_rdbuf(const std::string &filename, unsigned int flags, unsigned int capacity) :
	_flags(flags), _ifd(-1)
{
	_data = (uint8_t *) malloc(capacity);
	if (!_data) {
		_errno = errno;
		return;
	}
	_capacity = capacity;

	if (filename != "-") {
		_fd = open(filename.c_str(), O_RDONLY);
		if (_fd < 0) {
			_errno = errno;
			return;
		}
		_flags |= CLOSE_ON_DELETE;
	} else {
		_fd = fileno(stdin);
		_flags &= ~CLOSE_ON_DELETE;
	}

	if (_flags & TAILF) {
		lseek(_fd, 0, SEEK_END);
		_ifd = inotify_init();
		if (_ifd < 0) {
			_errno = errno;
			return;
		}
		inotify_add_watch(_ifd, filename.c_str(), IN_MODIFY);
	}

	reset();
}

file_rdbuf::~file_rdbuf()
{
	if (_data)
		free(_data);
	if (_flags & CLOSE_ON_DELETE)
		close(_fd);
	if (_ifd != -1)
		close(_ifd);
}

void file_rdbuf::copy_out(uint8_t* &dst, unsigned int &len)
{
	unsigned int n = std::min(_size, len);
	memcpy(dst, head(), n);
	_head += n;
	_size -= n;
	dst += n;
	len -= n;
}

bool file_rdbuf::read_in()
{
retry:
	ssize_t r = ::read(_fd, _data, _capacity);
	if (r < 0) {
		_errno = errno;
		return false;
	}
	if (!r) {
		if (_flags & TAILF) {
			uint8_t buf[sizeof(struct inotify_event) + NAME_MAX + 1];
			r = ::read(_ifd, buf, sizeof(buf));
			if (r < 0) {
				_errno = errno;
				return false;
			}
			goto retry;
		}
		return false;
	}

	_head = 0;
	_size = r;
	return true;
}

bool file_rdbuf::read(void *ptr, unsigned int len)
{
	uint8_t *dst = (uint8_t *) ptr;
	while (len) {
		if (!_size && !read_in())
			return false;
		copy_out(dst, len);
	}
	return true;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

