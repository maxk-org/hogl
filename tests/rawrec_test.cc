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
#include <time.h>

#include "hogl/format-basic.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/output-plainfile.hpp"
#include "hogl/engine.hpp"
#include "hogl/area.hpp"
#include "hogl/mask.hpp"
#include "hogl/post.hpp"
#include "hogl/ring.hpp"
#include "hogl/fmt/printf.h"

__HOGL_PRIV_NS_USING__;

// Replacement default ring options.
// Must be shared and immortal.
hogl::ringbuf::options hogl::default_ring_options = {
        .capacity = 2048,
        .prio = 0,
        .flags = hogl::ringbuf::SHARED | hogl::ringbuf::IMMORTAL,
        .record_tailroom = 2048
};

class myformat : public hogl::format_basic {
public:
	myformat(const char *str) : hogl::format_basic(str) {}

private:
	virtual void output_raw(hogl::ostrbuf &sb, record_data &d);
};

void myformat::output_raw(hogl::ostrbuf &sb, record_data &d)
{
	const hogl::record& r = *d.record;

	// arg0 points to raw data
	// arg1 is data type
	unsigned int len; const uint8_t *data = r.get_arg_data(0, len);
	unsigned int data_type = r.get_arg_val32(1);

	sb.printf("custom raw record: type %u, len %u @ %p\n", data_type, len, data);
}

static const hogl::area *test_area;

static unsigned int custom_data(uint8_t *dst, unsigned int room)
{
	uint8_t data[256];

	unsigned int len = sizeof(data) > room ? room : sizeof(data);
	memcpy(dst, data, len);

	return len;
}

static int doTest()
{
	uint8_t rawbuf[1024];

	hogl::post(test_area, test_area->DEBUG, hogl::arg_raw(rawbuf, sizeof(rawbuf)), 1234);

	hogl::post(test_area, test_area->DEBUG, "simple debug message %u %u %u", 1, 2, 3);
	hogl::post(test_area, test_area->ERROR, "simple error message %u %u %u %u %u",
			10, 20, 30, 40, 50);
	hogl::post(test_area, test_area->INFO, "simple info message");

	// Open-coded post example
	{
		hogl::ringbuf *ring = hogl::tls::ring();
		hogl::record *r = hogl::post_impl::begin_locked(ring, test_area, test_area->INFO);

		// Add two arguments: arg0 is raw data, arg1 is uint32_t
		unsigned int offset = sizeof(uint64_t) * 2;
		unsigned int room   = ring->record_tailroom() - offset;

		r->set_arg_type(0, hogl::arg::RAW);
		r->set_arg_data(0, offset, custom_data((uint8_t *) r->argval + offset, room));
		r->set_arg_type(1, hogl::arg::UINT32);
		r->set_arg_val32(1, 9895 /* dummy */);

		hogl::post_impl::finish_locked(ring);
	}

	return 0;
}

// Command line args {
static struct option main_lopts[] = {
   {"help",    0, 0, 'h'},
   {"file",    1, 0, 'f'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:";

static char main_help[] =
   "RAW record test 0.1 \n"
   "Usage:\n"
      "\trawrec_test [options]\n"
   "Options:\n"
      "\t--help -h            Display help text\n"
      "\t--file -f <name>     Log to a file\n";
// }

int main(int argc, char *argv[])
{
	const char *file = NULL;
	int opt;

	// Parse command line args
	while ((opt = getopt_long(argc, argv, main_sopts, main_lopts, NULL)) != -1) {
		switch (opt) {
		case 'f':
			file = strdup(optarg);
			break;

		case 'h':
		default:
			fmt::printf("%s", main_help);
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;
	
	if (argc < 0) {
		fmt::printf("%s", main_help);
		exit(1);
	}

	myformat logfmt("timespec|timedelta|ring|seqnum|area|section");
	hogl::mask     logmask(".*", 0);

	hogl::output *logout;
	if (file)
		logout = new hogl::output_plainfile(file, logfmt, 64 * 1024);
	else
		logout = new hogl::output_stderr(logfmt, 64 * 1024);
	hogl::activate(*logout);

	test_area = hogl::add_area("TEST-AREA");

	hogl::apply_mask(logmask);

	if (doTest() < 0) {
		fmt::printf("Failed\n");
		return 1;
	}

	fflush(stdout);
	fflush(stderr);

	hogl::deactivate();

	delete logout;

	fmt::printf("Passed\n");
	return 0;
}
