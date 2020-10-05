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
 * @file hogl/detail/timesource.hpp
 * Timesources
 */
#ifndef HOGL_DETAIL_TIMESOURCE_HPP
#define HOGL_DETAIL_TIMESOURCE_HPP

#include <stdint.h>

#include <hogl/detail/compiler.hpp>
#include <hogl/detail/preproc.hpp>
#include <hogl/detail/timestamp.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Timesource used by the engine and its rings.
 */
class timesource {
public:
	// Callback function.
	// Note:: This logic must be as lean and as fast as possible because
	// it's one of the top contributors to the client side overhead.
	// While something like boost::function or std::function would've been
	// more flexible it has much more overhead and is an overkill for the
	// default timesource which simply calls clock_gettime() and does not
	// even need any context.
	typedef hogl::timestamp (*callback)(const timesource *self);

	/// Get timestamp.
	/// Calls the callback function to get the timestamp.
	hogl::timestamp timestamp() const { return _callback(this); }

	/// Get the name of this timesource
	const char *name() const { return _name; }

	/// Initialize callback object.
	/// @param name timesource name
	/// @param cb callback function pointer
	timesource(const char *name, callback cb);
	~timesource();

	timesource(const timesource& ts);

protected:
	callback _callback;
	char    *_name;
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_TIMESOURCE_HPP
