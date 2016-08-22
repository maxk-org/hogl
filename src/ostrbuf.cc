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
 * Efficient output string buffer used for record formatting
 */

#include <stdarg.h>
#include <assert.h>

#include <hogl/detail/ostrbuf.hpp>

namespace hogl {

#if defined(__linux__)

// ssize_t reader (void *cookie, char *buffer, size_t size)
// int seeker (void *cookie, off64_t *position, int whence)
// int stdio_close(void *cookie)

static ssize_t stdio_write(void *cookie, const char *data, size_t size)
{
	ostrbuf *ob = (ostrbuf *) cookie;
	ob->put((const uint8_t *) data, size);
	return size;
}

static cookie_io_functions_t stdio_ops = {
	.read = 0,
	.write = stdio_write,
	.seek = 0,
	.close = 0
};

FILE* stdio_custom_open(void *ctx)
{
	return fopencookie(ctx, "w", stdio_ops);
} 

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

static int stdio_write(void *cookie, const char *data, int size)
{
	ostrbuf *ob = (ostrbuf *) cookie;
	ob->put((const uint8_t *) data, size);
	return size;
}

FILE* stdio_custom_open(void *ctx)
{
	return fwopen(ctx, stdio_write);
}

#else

#error "Unsupported OS"

#endif

void ostrbuf::failure(const char *err)
{
	_error[0] = '\0';
	strncat(_error, err, sizeof(_error) - 1);
	_failed = true;
}

ostrbuf::ostrbuf(size_t capacity, bool nostdio) :
	_data(0), _capacity(0), _size(0), _stdio(0),
	_failed(false)
{
	_error[0] = '\0';

	// If capacity is zero all buffering is disabled but the ostrbuf
	// is still fully usable.

	if (capacity) {
		_data = (uint8_t*) malloc(capacity);
		if (!_data) {
			failure("failed to allocate data buffer - no memory");
			return;
		}
		_capacity = capacity;
	}

	if (!nostdio) {
		_stdio = stdio_custom_open((void *) this);
		if (!_stdio) {
			failure("failed to allocate virtual stdio handle - no memory");
			return;
		}

		// Disable stdio buffering, we have our own
		setvbuf(_stdio, NULL, _IONBF, 0);
	}
}

ostrbuf::~ostrbuf()
{
	_capacity = _size = 0;
	if (_stdio)
		fclose(_stdio);
	free(_data);
}

void ostrbuf::printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(_stdio, fmt, ap);
	va_end(ap);
}

} // namespace hogl
