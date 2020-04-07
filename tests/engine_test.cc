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

#define _XOPEN_SOURCE 700

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/detail/engine.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/format-basic.hpp"

#define BOOST_TEST_MODULE engine_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

BOOST_AUTO_TEST_CASE(default_opts)
{
	hogl::format_basic  format;
	hogl::output_stderr output(format);
	hogl::engine eng(output);
}

BOOST_AUTO_TEST_CASE(custom_opts)
{
	hogl::format_basic  format;
	hogl::output_stderr output(format);

	hogl::engine eng(output);
}

BOOST_AUTO_TEST_CASE(area_reuse)
{
	hogl::format_basic  format;
	hogl::output_stderr output(format);

	hogl::engine eng(output);

	const char *sect0[] = { "X", "Y", "Z", 0 };
	const char *sect1[] = { "A", "B", "C", 0 };

	// New area
	const hogl::area *a0 = eng.add_area("XYZ", sect0);
	BOOST_REQUIRE(a0 != 0);

	// Good reuse
	const hogl::area *a1 = eng.add_area("XYZ", sect0);
	BOOST_REQUIRE(a1 != 0 && a1 == a0);

	// Bad reuse
	const hogl::area *a2 = eng.add_area("XYZ", sect1);
	BOOST_REQUIRE(a2 == 0);
}

BOOST_AUTO_TEST_CASE(default_mask)
{
	hogl::format_basic  format;
	hogl::output_stderr output(format);

	hogl::engine::options opts = {
		.default_mask = hogl::mask(".*:(INFO|WARN|ERROR|FATAL).*", 0),
		.polling_interval_usec = 10000,           // polling interval usec
		.tso_buffer_capacity =   4096,            // tso buffer size (number of records)
		.features = 0,                            // default feature set
		.schedparam = 0,
		.timesource = 0                           // timesource for this engine (0 means default timesource)
	};

	hogl::engine eng(output, opts);

	// New area
	const hogl::area *a = eng.add_area("XYZ");
	BOOST_REQUIRE(a->test(hogl::area::INFO) == true);
	BOOST_REQUIRE(a->test(hogl::area::WARN) == true);
	BOOST_REQUIRE(a->test(hogl::area::ERROR) == true);
	BOOST_REQUIRE(a->test(hogl::area::FATAL) == true);
	BOOST_REQUIRE(a->test(hogl::area::DEBUG) == false);
	BOOST_REQUIRE(a->test(hogl::area::TRACE) == false);

	std::cout << *a;
	std::cout << eng;
}

BOOST_AUTO_TEST_CASE(cpu_affinity_mask)
{
	hogl::format_basic  format;
	hogl::output_stderr output(format);

	hogl::engine::options opts = {
		.default_mask = hogl::mask(".*:(INFO|WARN|ERROR|FATAL).*", 0),
		.polling_interval_usec = 10000,
		.tso_buffer_capacity =   4096,
		.features = 0,
		.schedparam = 0,
		.timesource = 0
	};

	// Set schedparams with affinity mask
	opts.schedparam = new hogl::schedparam(0, 0, hogl::schedparam::DELETE_ON_EXIT, "0x1");

	hogl::engine eng(output, opts);

	std::cout << eng;
}

BOOST_AUTO_TEST_CASE(cpu_affinity_list)
{
	hogl::format_basic  format;
	hogl::output_stderr output(format);

	hogl::engine::options opts = {
		.default_mask = hogl::mask(".*:(INFO|WARN|ERROR|FATAL).*", 0),
		.polling_interval_usec = 10000,
		.tso_buffer_capacity =   4096,
		.features = 0,
		.schedparam = 0,
		.timesource = 0
	};

	// CPU affinity as a list

	hogl::schedparam sp(0, 0, 0, "list:1,5-6");
	opts.schedparam = &sp;

	hogl::engine eng(output, opts);

	std::cout << eng;
}
