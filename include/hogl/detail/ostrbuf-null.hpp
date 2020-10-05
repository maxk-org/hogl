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
 * @file hogl/detail/ostrbuf-null.h
 * Stream buffer with null output.
 */

#ifndef HOGL_DETAIL_OSTRBUF_NULL_HPP
#define HOGL_DETAIL_OSTRBUF_NULL_HPP

#include <stdlib.h>
#include <stdexcept>

#include <hogl/detail/ostrbuf.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Stream buffer with null output.
 * In the normal state all writes are discarded.
 * In the failed state all writes throw std::logic_error.
 */
class ostrbuf_null : public ostrbuf
{
private:
	virtual void do_flush(const uint8_t *, size_t)
	{
		if (failed())
			throw std::logic_error("hogl::ostrbuf -- write into failed stream");
		reset();
	}

public:
	// Construct null ostrbuf in a normal state
	ostrbuf_null(unsigned int buffer_capacity = 8192) :
		ostrbuf(buffer_capacity)
	{ }

	// Construct null ostrbuf in a failed state
	ostrbuf_null(const char *err) :
		ostrbuf(0)
	{
		failure(err);
	}
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_OSTRBUF_NULL_HPP
