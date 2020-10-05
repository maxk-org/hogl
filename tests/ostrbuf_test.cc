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

#define _XOPEN_SOURCE 700

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/detail/ostrbuf.hpp"

#define BOOST_TEST_MODULE ostrbuf_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

class my_ostrbuf : public hogl::ostrbuf {
private:
	void dump(const char *how, const uint8_t* data, size_t len)
	{
		fmt::fprintf(stdout, "\t%s: len %u [", how, (unsigned int) len);
		if (len > 60) {
			fwrite(data,  1, 57, stdout);
			fwrite("...", 1, 3, stdout);
		} else
			fwrite(data, 1, len, stdout);
		fmt::fprintf(stdout, "]\n");
	}

	void do_flush(const uint8_t* data, size_t len)
	{
		fmt::fprintf(stdout, "my-ostrbuf::flush\n");
		if (_size) {
			dump("flush-buffered", _data, _size);
			reset();
		}

		if (len > room()) {
			dump("skip-buffering", data, len);
			return;
		}

		if (len)
			do_copy(data, len);
	}

public:
	my_ostrbuf(unsigned int capacity) :
		hogl::ostrbuf(capacity) 
	{}
};

static void generate_output(hogl::ostrbuf &ob)
{
	ob.push_back("0123");
	ob.push_back("4567");
	ob.push_back("abcd");	
	ob.push_back("0123456789abcdef");
	ob.printf("arg0=%d arg1=%x arg2=%f arg3=%s", -125, ~0U, 125.67, "just-a-string");

	ob.push_back("efgh");
	ob.push_back("iklm");
	ob.push_back("xyz");

	char insanely_long_string[64 * 1024];
	for (unsigned int i = 0; i < sizeof(insanely_long_string)-1; i++)
		insanely_long_string[i] = 'A' + i % 26;
	insanely_long_string[sizeof(insanely_long_string)-1] = '\0';

	ob.printf("insanely-long-string=%s", insanely_long_string);
}

BOOST_AUTO_TEST_CASE(basic)
{
	my_ostrbuf ob8(8);
	generate_output(ob8);

	for (unsigned int i = 1; i <= 16; i *= 4) {
		my_ostrbuf *ob = new my_ostrbuf(i * 1024);
		generate_output(*ob);
		delete ob;
	}
}
