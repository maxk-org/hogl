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

/**
 * @file hogl/detail/ostrbuf.h
 * Efficient output string buffer used for record formatting
 */

#include <stdarg.h>
#include <assert.h>

#include <hogl/detail/ostrbuf.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

void ostrbuf::failure(const char *err)
{
	_error[0] = '\0';
	strncat(_error, err, sizeof(_error) - 1);
	_failed = true;
}

bool ostrbuf::failed() const { return _failed; }

const char* ostrbuf::error() const { return _error; }

ostrbuf::ostrbuf(size_t capacity) :
	_data(0), _capacity(0), _size(0), _failed(false)
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
}

ostrbuf::~ostrbuf()
{
	_capacity = _size = 0;
	free(_data);
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

