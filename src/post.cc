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

#include "hogl/detail/post.hpp"

namespace hogl {
namespace post_impl {

// Clang does not support 'hot' attribute
#ifdef __clang__
#define __hogl_hot_path 
#else
#define __hogl_hot_path hot
#endif // clang

#define __hogl_post_impl_attrs __attribute__((__hogl_hot_path))

/**
 * Begin posting new record. Unlocked version that uses TLS ring.
 * This function gets a new record form the ring and populate common fields
 * @return log record being pushed.
 */
__hogl_post_impl_attrs record* begin_unlocked(ringbuf *ring, const area *a, unsigned int s)
{
	record *r = ring->push_begin();
	r->seqnum    = ring->inc_seqnum();
	r->timestamp = ring->timestamp();
	r->area      = a;
	r->section   = s;
	return r;
}

/**
 * Finish record posting. Unlocked version.
 * This function gets a new record form the ring and populate common fields
 */
__hogl_post_impl_attrs void finish_unlocked(ringbuf *ring)
{
	ring->push_commit();
}

/**
 * Begin posting new record. Locked version.
 * This function gets a new record form the ring and populate common fields
 * @return log record being pushed.
 */
__hogl_post_impl_attrs record* begin_locked(ringbuf *ring, const area *a, unsigned int s)
{
	ring->lock();
	return begin_unlocked(ring, a, s);
}

/**
 * Finish record posting. Locked version.
 * This function gets a new record form the ring and populate common fields
 */
__hogl_post_impl_attrs void finish_locked(ringbuf *ring)
{
	finish_unlocked(ring);
	ring->unlock();
}

/**
 * Begin posting new record. Unlocked version that uses TLS ring.
 * This function gets a new record form the ring and populate common fields
 * @return log record being pushed.
 */
__hogl_post_impl_attrs void unlocked(ringbuf *ring, const area *a, unsigned int s, const argpack &ap)
{
	record *r = begin_unlocked(ring, a, s);
	r->set_args(ring->record_tailroom(), ap);
	finish_unlocked(ring);
}

/**
 * Begin posting new record. Locked version.
 * This function gets a new record form the ring and populate common fields
 * @return log record being pushed.
 */
__hogl_post_impl_attrs void locked(ringbuf *ring, const area *a, unsigned int s, const argpack &ap)
{
	record *r = begin_locked(ring, a, s);
	r->set_args(ring->record_tailroom(), ap);
	finish_locked(ring);
}

} // namespace post_impl
} // namespace hogl
