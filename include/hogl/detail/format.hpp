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
 * @file hogl/detail/format.h
 * Format handler implementation details.
 */
#ifndef HOGL_DETAIL_FORMAT_HPP
#define HOGL_DETAIL_FORMAT_HPP

#include <hogl/detail/record.hpp>
#include <hogl/detail/ostrbuf.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Abstract format handler
 */
class format {
public:
	/**
	 * Data used for record formatting.
	 * The format handler uses this to generate the final records.
	 */
	struct data {
		const char      *ring_name; /// Ring buffer name
		const hogl::record *record; /// Pointer to the record
	};

	/**
	 * Process the record and save it into the string/byte buffer.
	 * @param d reference to the format data @see hogl::format::data
	 * @param s reference to the string buffer
	 */
	virtual void process(ostrbuf &s, const data &d) = 0;

	/**
	 * Generate format header.
	 * Called everytime new output stream (file, pipe, etc) is opened.
	 * Also called every time output file is rotated (i.e. for each
	 * new file chunk).
	 * @param s reference to the string buffer
	 * @param n name of the current output (filename, pipe, etc)
	 * @param first true if this is the first header (first file chunk)
	 */
	virtual void header(ostrbuf &s, const char *n, bool first) {};

	/**
	 * Generate format footer.
	 * @param s reference to the string buffer
	 * @param n name of the next output (filename, pipe, etc), set to zero for the last chunk.
	 */
	virtual void footer(ostrbuf &s, const char *n) {};

	virtual ~format() {}
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_FORMAT_HPP
