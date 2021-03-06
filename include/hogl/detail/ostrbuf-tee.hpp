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
 * @file hogl/detail/ostrbuf-tee.h
 * String buffer with output to two outher ostrbufs
 */

#ifndef HOGL_DETAIL_OSTRBUF_TEE_HPP
#define HOGL_DETAIL_OSTRBUF_TEE_HPP

#include <hogl/detail/ostrbuf.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * String buffer with output to two other ostrbufs
 */
class ostrbuf_tee : public ostrbuf
{
private:
	ostrbuf* _o0;
	ostrbuf* _o1;

	virtual void do_flush(const uint8_t *data, size_t len);

public:
	ostrbuf_tee(ostrbuf* o0, ostrbuf* o1, unsigned int buffer_capacity = 8192);

	// Get error message that explains stream failure cause
	virtual const char* error() const;

	// Check if the underlying stream has failed
	virtual bool failed() const;

	virtual ~ostrbuf_tee();
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

#endif // HOGL_DETAIL_OSTRBUF_TEE_HPP
