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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>

#include <string>
#include <map>
#include <algorithm>

#include "hogl/detail/internal.hpp"
#include "hogl/detail/engine.hpp"
#include "hogl/detail/barrier.hpp"
#include "hogl/platform.hpp"
#include "hogl/post.hpp"

#ifdef HOGL_DEBUG
#define dprint(fmt, args...) fprintf(stderr, "hogl: " fmt "\n", ##args)
#else
#define dprint(a...)
#endif

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// Default ringbuf
ringbuf default_ring("DEFAULT", default_ring_options);

// Default engine instance
engine *default_engine;

void activate(output &out, const engine::options &engine_opts)
{
	assert(default_engine == 0);

	// Default ringbuf must be shared and immortal.
	// We check here because user app is allowed to replace
	// default options.
	assert(default_ring.immortal() && default_ring.shared());

	// Initialize default engine
	engine *e = new engine(out, engine_opts);

	// Indicate that default ring is in use
	default_ring.hold();

	// Register default shared ringbuf
	e->add_ring(&default_ring);

	default_engine = e;

	// Call deactivate on exit
	atexit(hogl::deactivate);
}

void deactivate()
{
	if (!default_engine)
		return;

	// Release the default ring.
	// So that it becomes an orphan and the engine could
	// drop it from its list.
	default_ring.release();

	delete default_engine;
	default_engine = 0;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__
