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

// This test demonstrates how to start an auxiliary HOGL engine for logging
// a different kind of data (binary records, etc).

#include <hogl/format-raw.hpp>
#include <hogl/format-basic.hpp>
#include <hogl/output-file.hpp>
#include <hogl/output-stdout.hpp>
#include <hogl/post.hpp>
#include <hogl/flush.hpp>
#include <hogl/engine.hpp>

const unsigned int ONE_KB = 1024;
const unsigned int ONE_MB = ONE_KB * 1024;
const unsigned int ONE_GB = ONE_MB * 1024;

const unsigned int chunk_size = ONE_KB * 512;
static uint8_t huge_buf[chunk_size * 4];

// Aux engine and related state
hogl::engine   *_aux_engine; 
hogl::area     *_aux_area;
hogl::ringbuf  *_aux_ring; 
hogl::format   *_aux_logfmt; 
hogl::output   *_aux_logout;

int main(void)
{
	// -- Start main/default logging
	hogl::format_basic def_logfmt("fast1");
	hogl::output_stdout def_logout(def_logfmt);
	hogl::activate(def_logout);

	// -- Start aux engine for binary logging
	_aux_logfmt = new hogl::format_raw();

	hogl::output_file::options aux_output_opts = {
		        perms: 0666,
		        max_size:  ONE_GB,
		        max_age: 0,
		        max_count: 5000,
		        buffer_capacity: ONE_MB,
		};
	_aux_logout = new hogl::output_file("bin.log", *_aux_logfmt, aux_output_opts);

	_aux_engine = new hogl::engine(*_aux_logout);

	_aux_area = _aux_engine->add_area("AUX-AREA");

	// -- Create a ring with the aux engine
	hogl::ringbuf::options aux_ring_opts = {
			capacity: 10,
			prio: 100,
			flags: 0,
			record_tailroom: sizeof(huge_buf)
		};
	_aux_ring = _aux_engine->add_ring("AUX-RING", aux_ring_opts);

	// Log some binary records
	unsigned int i;
	for (i=0; i<10; i++) {
		hogl::push(_aux_ring, _aux_area, 0,
				4, // four chunks
				hogl::arg_raw(&huge_buf[chunk_size * 0], 64000),
				hogl::arg_raw(&huge_buf[chunk_size * 1], 64000),
				hogl::arg_raw(&huge_buf[chunk_size * 2], 64000),
				hogl::arg_raw(&huge_buf[chunk_size * 3], 64000));
	}

	// Wait for those records to get processed
	hogl::flush(_aux_ring);

	_aux_ring->release();

	// -- Destroy aux engine
	delete _aux_engine;
	delete _aux_logout;
	delete _aux_logfmt;

	// -- Stop main logging
	hogl::deactivate();

	return 0;
}
