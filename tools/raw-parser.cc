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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>

#include "hogl/detail/ringbuf.hpp"
#include "hogl/detail/magic.hpp"
#include "hogl/plugin/format.hpp"
#include "hogl/format-basic.hpp"
#include "hogl/area.hpp"

#include "raw-parser.hpp"
#include "rdbuf.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

raw_area::raw_area() : hogl::area("dummy")
{
	// Realocate name and section strings to max size.
	// The raw parser reads those strings directly into
	// the buffers allocated here.
	// Regular hogl::area destructor handles this properly.
	// Note that we don't need to delete the _section array
	// because it's pointing to static default_section.
	free(_name);
	_name = (char *) malloc(MAX_NAME_LEN);
	_section_name = (char *) malloc(MAX_NAME_LEN);

	_bitmap.resize(1);
	_section = new const char* [1];
	_section[0] = _section_name;
}

void raw_parser::read_args(hogl::record &r)
{
	unsigned long offset = 16 * 8;

	r.argtype = read_uint<uint64_t>("argtype");

	for (unsigned int i=0; i < record::NARGS; i++) {
		if (_failed) return;

		unsigned int type = r.get_arg_type(i);
		unsigned long n;

		switch (type) {
		case arg::NONE:
			return;

		case arg::GSTR:
			// Remap GSTR to CSTR.
			// Since the length is known it might be useful for the formatter.
			r.clear_arg_type(i);
			r.set_arg_type(i, hogl::arg::CSTR);
			// fall through
		case arg::CSTR:
			n = read_str<uint16_t>((char *) r.argval + offset);
			r.set_arg_data(i, offset, n - 1);
			offset += n;
			break;

		case arg::HEXDUMP:
		case arg::RAW:
			if (_ver == V1)
				n = read_blob<uint16_t>((char *) r.argval + offset);
			else
				n = read_blob<uint32_t>((char *) r.argval + offset);

			r.set_arg_data(i, offset, n);
			offset += n;
			break;
		case arg::INT32:
		case arg::UINT32:
			r.set_arg_val32(i, read_uint<uint32_t>());
			break;
		default:
			r.set_arg_val64(i, read_uint<uint64_t>());
			break;
		}
	}
}

raw_parser::raw_parser(hogl::rdbuf &in, unsigned int ver, unsigned int max_record_size) :
	_in(in), _ver(ver)
{
	reset();

	// Allocate nicely aligned buffer for the dummy record 
	// we're going to populate with data read from the file.
	int err = posix_memalign((void **)&_record, 64, max_record_size);
	if (err) {
		_error << "failed to allocate record buffer. "
			<< strerror(errno) << "(" <<  errno << ")";
		_failed = true;
		return;
	}

	_ring_name = (char *) malloc(_area.MAX_NAME_LEN);
	if (!_ring_name) {
		_error << "failed to allocate ring name. "
			<< strerror(errno) << "(" <<  errno << ")";
		_failed = true;
		return;
	}

	_format_data.ring_name = _ring_name;
	_format_data.record    = (hogl::record *) _record;
}

raw_parser::~raw_parser()
{
	free(_record);
	free(_ring_name);
}

void raw_parser::reset()
{
	_error.clear();
	_failed = false;
}

// Read and return next record from the input
const hogl::format::data* raw_parser::next()
{
	hogl::record &r = *(hogl::record *)_record;

	r.clear();
	r.area = &_area;

	r.timestamp = read_uint<uint64_t>("timestamp");
	if (_failed) {
		// Clear the error status since we most likely simply hit the EOF
		reset();
		return 0;
	}

	r.seqnum = read_uint<uint64_t>("seqnum");
	read_str<uint8_t>(_ring_name,     "ring-name");
	read_str<uint8_t>(_area.name(),   "area-name");
	read_str<uint8_t>(_area.section(),"section-name");

	if (_failed)
		return 0;

	read_args(r);

	if (_failed)
		return 0;

	return &_format_data;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

