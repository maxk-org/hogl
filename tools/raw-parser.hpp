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

#ifndef HOGL_RAW_PARSER_HPP
#define HOGL_RAW_PARSER_HPP

#include <stdint.h>

#include <sstream>

#include "hogl/detail/record.hpp"
#include "hogl/detail/format.hpp"
#include "hogl/detail/area.hpp"

#include "rdbuf.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// RAW area.
// Used for reconstructing areas while reading RAW record stream.
class raw_area : public hogl::area
{
private:
	char *_section_name;

public:
	enum { MAX_NAME_LEN = 256 };

	raw_area();

	// Overrite area methods to return non-const pointers
	char *name()    { return _name; }
	char *section() { return _section_name; }
};

// RAW record stream parser
class raw_parser {
private:
	hogl::rdbuf&      _in;     // input stream
	bool              _failed; // failure state
	std::stringstream _error;  // last error string

	hogl::format::data _format_data; // data struct passed to formatter
	void              *_record;      // record buffer
	raw_area           _area;        // raw area
	char              *_ring_name;   // ringbuffer name for last record

	unsigned int       _ver;

	// Read uint from the input stream
	template <typename T>
	T read_uint(const char *stage = "")
	{
		if (_failed) return 0;

		T v = 0;
		_failed = ! _in.read(&v, sizeof(T));
		if (_failed)
			_error << "failed to read uint @ " << stage;
		return v;
	}

	// Read a blob from the input stream
	template <typename T>
	unsigned long read_blob(char *data, const char *stage = "")
	{
		if (_failed) return 0;

		unsigned long len = read_uint<T>(stage);
		if (_failed || !len)
			return 0;

		_failed = ! _in.read(data, len);
		if (_failed) {
			_error << "failed to read blob @ " << stage;
			return 0;
		}
		return len;
	}

	// Read a string from the input stream
	// Strings are guarantied to be null-terminated
	// Returns string length including the null-terminator.
	template <typename T>
	unsigned long read_str(char *str, const char *stage = "")
	{
		if (_failed) return 0;

		unsigned long len = read_blob<T>(str, stage);

		if (_failed) return 0;

		str[len++] = 0;
		return len;
	}

	// Read and reconstruct record arguments from the input stream
	void read_args(hogl::record &r);

public:
	// Compatibility versions
	enum versions { V1, V1_1 };

	// Constructor
	raw_parser(hogl::rdbuf &in, unsigned int ver = V1_1, unsigned int max_record_size = 10 * 1024 * 1024);
	~raw_parser();

	// Check of the last operation failed
	bool failed() const { return _failed; }

	// Get error string for last failed operation
	std::string error() const { return _error.str(); }

	// Reset parser state
	void reset();

	// Get next record extracted from the input stream.
	// Returns null if no more records can be extracted.
	const hogl::format::data* next();
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_RAW_PARSER_HPP
