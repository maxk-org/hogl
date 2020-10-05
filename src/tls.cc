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

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include "hogl/detail/internal.hpp"
#include "hogl/engine.hpp"
#include "hogl/tls.hpp"
#include "hogl/post.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {
/**
 * Per thread ring pointer
 */
thread_local ringbuf *tls::_ring = &default_ring;

/**
 * Update TLS ring pointer.
 * Private helper used from constructors.
 */
void tls::update(ringbuf *r)
{
	// hold a reference to the previous ring (ie current tls pointer)
	_previous_ring = _ring->hold();

	hogl::post(_engine->internal_area(), internal::TLS_DEBUG,
		"created tls %p. ring %s(%p)", this, r->name(), r);

	// Set TLS ring pointer
	_ring = _current_ring = r;
}

/**
 * TLS constructor
 */
tls::tls(const char *name, ringbuf::options &opts, engine *engine) :
	_engine(engine), _previous_ring(nullptr), _current_ring(nullptr)
{
	ringbuf *r = engine->add_ring(name, opts);
	if (!r) {
		// Ring allocation failed: out of mem, not unique name & not reuseable, etc.
		// Instead of setting tls pointer to null, and usually causing segfaults downstream,
		// simply do nothing and the thread will use the current tls pointer.
		// The engine logs an error in this case, and if needed the caller can simply check
		// tls.valid() if strict error checking is required.
		return;
	}
	update(r);
}

tls::tls(ringbuf *r, engine *engine) :
	_engine(engine), _previous_ring(nullptr), _current_ring(nullptr)
{
	update(r);
}

tls::~tls()
{
	// Looks like allocation failed in the constructor
	if (!valid()) {
		hogl::post(_engine->internal_area(), internal::WARN, "destroyed tls %p null-ring", this);
		return;
	}

	// Set TLS ring pointer back to the previous ring
	_ring = _previous_ring;

	// release the reference to the previous ring
	_previous_ring->release();

	ringbuf *r = _current_ring;
	hogl::post(_engine->internal_area(), internal::TLS_DEBUG,
		"destroyed tls %p ring %p (empty %u)", this, r, r->empty());

	r->release();
}

#if defined(__QNXNTO__)
// QNX TLS implementation is broken for shared libraries.
// Out-of-line version here is a workaround.
ringbuf* tls::ring() { return _ring; }
#endif

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

