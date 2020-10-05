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
 * @file hogl/ring.hpp
 * Ring management.
 */

#ifndef HOGL_RING_HPP
#define HOGL_RING_HPP

#include <hogl/detail/ringbuf.hpp>
#include <hogl/detail/engine.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

extern engine *default_engine;

/**
 * Add new ringbuf to the default engine.
 * If ringbuf::SHARED and ringbuf::REUSABLE flags are not set the name must be unique for this
 * engine. Otherwise the allocation will fail, the engine will log an error, and return nullptr.
 * @param name ringbuf name
 * @param opts ringbuf options
 * @return pointer to the new ringbuf or nullptr of allocation failed
 */ 
static inline ringbuf *add_ring(const char *name, const ringbuf::options &opts)
{
	return default_engine->add_ring(name, opts);
}

static inline ringbuf *add_ring(const std::string &name, const ringbuf::options &opts)
{
	return default_engine->add_ring(name.c_str(), opts);
}

/**
 * Find ringbuf in the default engine.
 * @param name ringbuf name
 * @return pointer to the ringbuf or zero if names was not found.
 */
static inline ringbuf *find_ring(const char *name)
{
	return default_engine->find_ring(name);
}

static inline ringbuf *find_ring(const std::string &name)
{
	return default_engine->find_ring(name.c_str());
}

/**
 * Get a list of ringbufs from the default engine
 * @param l reference to a stringlist
 */
static inline void list_rings(string_list &l)
{
	default_engine->list_rings(l);
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

#endif // HOGL_RING_HPP
