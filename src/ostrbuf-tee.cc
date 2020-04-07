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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "hogl/detail/compiler.hpp"
#include "hogl/detail/ostrbuf-tee.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

bool ostrbuf_tee::failed() const
{
	return (_o0->failed() || _o1->failed());
}

const char* ostrbuf_tee::error() const 
{
	return _o0->failed() ? _o0->error() : _o1->error();
}

void ostrbuf_tee::do_flush(const uint8_t *data, size_t len)
{
	if (failed())
		return;

	// Flush buffered data
	if (_size) {
		_o0->flush(_data, _size);
		_o1->flush(_data, _size);
	}

	// Flush un-buffered data
	if (len) {
		_o0->flush(data, len);
		_o1->flush(data, len);
	}

	this->reset();
}

ostrbuf_tee::ostrbuf_tee(ostrbuf* o0, ostrbuf* o1, unsigned int buffer_capacity) :
	ostrbuf(buffer_capacity), _o0(o0), _o1(o1)
{ }

ostrbuf_tee::~ostrbuf_tee()
{
	// We don't own underlying ostrbufs
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__
