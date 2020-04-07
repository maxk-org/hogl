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

#include "hogl/detail/ostrbuf-null.hpp"
#include "hogl/format-basic.hpp"

#define BOOST_TEST_MODULE area_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

BOOST_AUTO_TEST_CASE(basic)
{
	hogl::format_basic fmt0(
		hogl::format_basic::TIMESTAMP | 
		hogl::format_basic::AREA |
		hogl::format_basic::SECTION);

	hogl::format_basic fmt1("timestamp|area|section");
}

// Test to make sure tscache does proper formatting of
// all possible timestamp values.
BOOST_AUTO_TEST_CASE(tscache)
{
	hogl::format_basic::tscache tscache;

	hogl::ostrbuf_null rsb(128);

	// Iterate through a bunch of timestamp values.
	// Pass them through the formatter and normal printf(), and
	// make sure that generated strings match.
	struct { uint64_t begin; uint64_t end; } range[] = {
		{ 0, 10000 },
		//{ 99999999, 1000000000 },
		//{              0,     2999999999 },
		//{   100000000000,   200999999999 },
		//{ 10000000000000, 10001999999999 },
		{ ~0ULL - 1000,      ~0ULL },
		{ 0, 0 },
	};

	for (unsigned int r=0; range[r].end; r++) {
		for (uint64_t i=range[r].begin; i < range[r].end; i++) {
			hogl::timestamp t = i;

			tscache.update(t);

			struct timespec ts;
			t.to_timespec(ts);

			rsb.reset();
			rsb.printf("%011lu.%09lu", ts.tv_sec, ts.tv_nsec);
			rsb.push_back('\0');

			BOOST_REQUIRE_MESSAGE(memcmp(tscache.str(), rsb.head(), tscache.len()) == 0,
				"[" << std::string(tscache.str(), tscache.len()) << "] vs [" << (const char *) rsb.head() << "]" );
		}
	}
}
