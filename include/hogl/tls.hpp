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
 * @file hogl/tls.h
 * Top-level TLS API.
 */
#ifndef HOGL_TLS_HPP
#define HOGL_TLS_HPP

#include <pthread.h>
#include <stdint.h>

#include <hogl/detail/ringbuf.hpp>
#include <hogl/engine.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Per thread state.
 * Each thread should allocate its own TLS for optimal performance.
 */
class tls {
private:
	/**
	 * Per thread ring pointer
	 */
	static thread_local ringbuf *_ring;

	/**
	 * Pointer to the engine serving this TLS
	 */ 
	engine *_engine;

	/**
	 * Pointer to the previous ring
	 */
	ringbuf *_previous_ring;

	/**
	 * Pointer to the current ring
	 */
	ringbuf *_current_ring;

	// No copies
	tls(const tls&);
	tls& operator=( const tls& );

	// Update ring pointers (called from constructors)
	void update(ringbuf *r);

public:
	/**
	 * Setup hogl per thread settings.
	 * This constructor is going to create and register new ring buffer using provided opts.
	 * If ringbuf::SHARED and ringbuf::REUSABLE flags are not set the name must be unique for this
	 * engine. Otherwise the allocation will fail, the engine will log an error, and this tls will
	 * reuse current ringbuf (usually default_ring).
 	 */
	tls(const char *name,
		ringbuf::options &opts = ringbuf::default_options,
		engine *engine = default_engine);
	/**
	 * Setup hogl per thread settings.
	 * This constructor will use an existing ring.
 	 */
	tls(ringbuf *r, engine *engine = default_engine);

	/**
	 * Cleanup per thread settings.
	 */
	~tls();

	/**
	 * Check if tls (ring, etc) was allocated correctly
	 */
	const bool valid() const { return _current_ring != nullptr; }

	/**
	 * Get pointer to the ring of the current thread
	 */
#if !defined(__QNXNTO__)
	static ringbuf *ring() { return _ring; }
#else
	static ringbuf *ring();
#endif
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

#endif // HOGL_TLS_HPP
