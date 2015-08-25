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

/**
 * @file hogl/timesource.hpp
 * Default timesource.
 */
#ifndef HOGL_TIMESOURCE_HPP
#define HOGL_TIMESOURCE_HPP

#include <hogl/detail/ringbuf.hpp>
#include <hogl/detail/timesource.hpp>

namespace hogl {

/**
 * Default timesource.
 * Weak symbol. Can be overriden by user app.
 */
extern timesource default_timesource;

/**
 * Chage timesource using a specified ring. The timesource will be changed on the engine 
 * the ring is registered with.
 * @param r pointer to a ringbuf
 * @param ts pointer to the timesource object
 * @param to_usec timeout in microseconds. Timesource change is asynchronous. This timeout 
 * sets the upper bound of how long calling thread will wait for the engine to ack the change.
 * @return true on success, false otherwise (due to timeout)
 * @warn timesource object must be valid at all times while the default engine is running.
 */
bool change_timesource(ringbuf *r, timesource *ts, unsigned int to_usec = 10000000);

/**
 * Chage timesource of the default engine
 * @param ts pointer to the timesource object
 * @param to_usec timeout in microseconds. Timesource change is asynchronous. This timeout 
 * sets the upper bound of how long calling thread will wait for the engine to ack the change.
 * @return true on success, false otherwise (due to timeout)
 * @warn timesource object must be valid at all times while the default engine is running.
 */
bool change_timesource(timesource *ts, unsigned int to_usec = 10000000);

} // namespace hogl

#endif // HOGL_TIMESOURCE_HPP
