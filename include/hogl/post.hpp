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
 * @file hogl/post.hpp
 * Top-level API for posting log records.
 */
#if !defined(HOGL_POST_HPP) && !defined(HOGL_FAST_POST_HPP)
#define HOGL_POST_HPP

#include <stdint.h>

#include <hogl/detail/compiler.hpp>
#include <hogl/detail/preproc.hpp>
#include <hogl/detail/area.hpp>
#include <hogl/detail/argpack.hpp>
#include <hogl/detail/post.hpp>
#include <hogl/tls.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Push a log record without locking
 */
static hogl_force_inline void push_unlocked(ringbuf *ring,
		const hogl::area *area,	unsigned int sect, __hogl_long_arg_list(16))
{
	argpack ap;
	unsigned int n = ap.populate(__hogl_short_arg_list(16));

	// Check whether we have complex arguments (cstr, hexdump, etc).
	// If not then populate the record directly inline, otherwise use 
	// the argpack to move all processing out of line.
	// This is resolved at compile time and does not generate any
	// extra code.
	if (!n) {
		record *r = post_impl::begin_unlocked(ring, area, sect);
		r->set_args(ring->record_tailroom(), __hogl_short_arg_list(16));
		post_impl::finish_unlocked(ring);
	} else
		post_impl::unlocked(ring, area, sect, ap);
}

/**
 * Push a log record into the ring
 */
static hogl_force_inline void push(ringbuf *ring,
		const hogl::area *area,	unsigned int sect, __hogl_long_arg_list(16))
{
	argpack ap;
	unsigned int n = ap.populate(__hogl_short_arg_list(16));

	// Check whether we have complex arguments (cstr, hexdump, etc).
	// If not then populate the record directly inline, otherwise use 
	// the argpack to move all processing out of line.
	// This is resolved at compile time and does not generate any
	// extra code.
	if (!n) {
		record *r = post_impl::begin_locked(ring, area, sect);
		r->set_args(ring->record_tailroom(), __hogl_short_arg_list(16));
		post_impl::finish_locked(ring);
	} else
		post_impl::locked(ring, area, sect, ap);
}

/**
 * Post new log record
 */
static hogl_force_inline void post(ringbuf *ring,
		const hogl::area *area, unsigned int sect, __hogl_long_arg_list(16))
{
	if (area->test(sect))
		push(ring, area, sect, __hogl_short_arg_list(16));
}

/**
 * Post new log record into the TLS ring
 */
static hogl_force_inline void post(
		const hogl::area *area, unsigned int sect, __hogl_long_arg_list(16))
{
	post(tls::ring(), area, sect, __hogl_short_arg_list(16));
}

/**
 * Post new log record without locking.
 * The ring must not be shared.
 */
static hogl_force_inline void post_unlocked(ringbuf *ring,
		const hogl::area *area, unsigned int sect, __hogl_long_arg_list(16))
{  
	if (area->test(sect))
		push_unlocked(ring, area, sect, __hogl_short_arg_list(16));
}

/**
 * Post new log record into the TLS ring without locking. 
 * The TLS ring must not be shared.
 */
static hogl_force_inline void post_unlocked(
		const hogl::area *area, unsigned int sect, __hogl_long_arg_list(16))
{
	post_unlocked(tls::ring(), area, sect, __hogl_short_arg_list(16));
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_POST_HPP
