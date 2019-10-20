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
 * @file hogl/detail/output.h
 * Output handler implementation details
 */
#ifndef HOGL_DETAIL_OUTPUT_HPP
#define HOGL_DETAIL_OUTPUT_HPP

#include <hogl/detail/ostrbuf.hpp>
#include <hogl/detail/format.hpp>
#include <hogl/detail/magic.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Output handler
 */
class output {
protected:
	magic    _magic;   // Magic signature
	format&  _format;  // Reference to the format handler

	// Pointer to the output buffer.
	// By default points to a null stream.
	// Derived classes must initialize this to point to the actual output.
	// The destructor deletes this object unconditionally.
	ostrbuf* _ostrbuf;

	/**
	 * Initialize output
	 * @param osb pointer to an underlying output stream. null sets the default.
	 */
	void init(ostrbuf *sb);

	/**
	 * Output constructor
	 */
	output(format &format);

public:
	// Check if the ouput failed
	bool failed() const { return _ostrbuf->failed(); }

	// Get error message that explains failure cause
	const char* error() const { return _ostrbuf->error(); }

	/**
	 * Flush output
	 */
	void flush() { _ostrbuf->flush(); }

	/**
	 * Get the reference to the format for this output
	 */
	format& get_format() { return _format; }

	/**
	 * Get the pointer to the ostrbuf for this output
	 */
	ostrbuf* get_ostrbuf() { return _ostrbuf; }

	/**
	 * Generate header.
	 * @param n name of the current output (filename, pipe, etc)
	 * @param first true if this is the first header (first file chunk)
	 */
	void header(const char *n, bool first = true)
	{
		_format.header(*_ostrbuf, n, first);
	}

	/**
	 * Generate footer.
	 * @param n name of the next output chunk (filename, pipe, etc), 
	 * set to null for the last chunk.
	 */
	void footer(const char *n = 0)
	{
		_format.footer(*_ostrbuf, n);
		flush();
	}

	/**
	 * Process and generate output record
	 * @param d format data @see hogl::format::data
	 */
	void process(const format::data &d)
	{
		_format.process(*_ostrbuf, d);
	}

	virtual ~output();
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_OUTPUT_HPP
