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

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <string>

#include "hogl/detail/internal.hpp"
#include "hogl/engine.hpp"
#include "hogl/tls.hpp"
#include "hogl/post.hpp"

namespace hogl {
/**
 * Per thread ring pointer
 */
__thread ringbuf *tls::_ring = &default_ring;

/**
 * TLS constructor
 */
tls::tls(const char *name, ringbuf::options &opts, engine *engine) :
	_engine(engine)
{
	ringbuf *r = engine->add_ring(name, opts);

	hogl::post(engine->internal_area(), internal::TLS_DEBUG,
		"created tls %p. ring %s(%p)", this, name, r);

	// Set TLS ring pointer
	_ring = r;

}

tls::tls(ringbuf *r, engine *engine) :
	_engine(engine)
{
	hogl::post(engine->internal_area(), internal::TLS_DEBUG,
		"created tls %p. name %s ring %p", this, r->name(), r);

	// Set TLS ring pointer
	_ring = r->hold();
}

tls::~tls()
{
	// Set TLS ring pointer back to default
	ringbuf *r = _ring;
	_ring = &default_ring;

	hogl::post(_engine->internal_area(), internal::TLS_DEBUG,
		"destroyed tls %p ring %p (empty %u)", this, r, r->empty());

	r->release();
}

} // namespace hogl
