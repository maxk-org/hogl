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

#include <string.h>
#include <stdlib.h>

#include "hogl/detail/internal.hpp"
#include "hogl/timesource.hpp"
#include "hogl/post.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

timesource::timesource(const char *name, callback cb)
	: _callback(cb), _name(strdup(name))
{ }

timesource::timesource(const timesource &ts)
	: _callback(ts._callback), _name(strdup(ts._name))
{ }

timesource::~timesource()
{
	free(_name);
}

// Comes from flush.cc
extern bool timeout(uint64_t &to, uint64_t usec);

// Note: This implementation is not self contained. 
// It assumes that the engine (or something else) that is poping 
// records out of this ring will ack the ts change record.
static bool do_change_timesource(ringbuf *ring, timesource *ts, uint64_t to_usec)
{
	uint64_t seq = ring->inc_seqnum();
	uint64_t to = 0;

	// Wait for some room in the ringbuf
	while (!ring->room()) {
		if (timeout(to, to_usec))
			return false;
	}

	// Push the timesource change reccord
	record *r = ring->push_begin();
	r->timestamp = ring->timestamp();
	r->seqnum    = seq;
	r->special(internal::SPR_TIMESOURCE_CHANGE, (uint64_t) ts);
	ring->push_commit(r);

	// If timeout is zero return success right away.
	// It's unlikely that the engine will fail to process 
	// special record.
	if (!to_usec)
		return true;

	// Wait for it to get acked
	while (!r->acked()) {
		if (timeout(to, to_usec))
			return false;
	}

	return true;
}

bool change_timesource(ringbuf *ring, timesource *ts, unsigned int to_usec)
{
	bool r;
	ring->lock();
	r = do_change_timesource(ring, ts, to_usec);
	ring->unlock();
	return r;
}

bool change_timesource(timesource *ts, unsigned int to_usec)
{
	if (!ts)
		ts = &default_timesource;
	ringbuf *ring = tls::ring();
	return change_timesource(ring, ts, to_usec);
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

