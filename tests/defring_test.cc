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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>

#include "hogl/format-basic.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/output-textfile.hpp"
#include "hogl/engine.hpp"
#include "hogl/area.hpp"
#include "hogl/mask.hpp"
#include "hogl/post.hpp"
#include "hogl/ring.hpp"

enum test_sect_id {
	TEST_DEBUG,
	TEST_INFO,
	TEST_WARN,
	TEST_ERROR,
	TEST_EXTRA_DEBUG,
	TEST_EXTRA_INFO,
	TEST_TRACE,
	TEST_HEXDUMP,
};

static const char *test_sect_names[] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"EXTRA:DEBUG",
	"EXTRA:INFO",
	"TRACE",
	"HEXDUMP",
	0,
};

static const hogl::area *test_area;

int doTest()
{
	// Post some test messages.
	// This will use the default ring.
	hogl::post(test_area, TEST_DEBUG, "simple debug message %u %u %u", 1, 2, 3);
	hogl::post(test_area, TEST_ERROR, "simple error message %u %u %u %u %u",
			10, 20, 30, 40, 50);
	hogl::post(test_area, TEST_EXTRA_INFO, "simple extra:info message");

	return 0;
}

// Replacement default ring options.
// Must be shared and immortal.
hogl::ringbuf::options hogl::default_ring_options = {
	capacity: 2048,
	prio:     0,
	flags:    hogl::ringbuf::SHARED | hogl::ringbuf::IMMORTAL,
	record_tailroom: 2048
};

// Command line args {
static struct option main_lopts[] = {
   {"help",    0, 0, 'h'},
   {"file",    1, 0, 'f'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:";

static char main_help[] =
   "HOGL default ring options test 0.1 \n"
   "Usage:\n"
      "\tdefring_test [options]\n"
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
			printf("%s", main_help);
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;
	
	if (argc < 0) {
		printf("%s", main_help);
		exit(1);
	}

	hogl::format_basic logfmt("timestamp|timedelta|ring|seqnum|area|section");
	hogl::mask         logmask(".*", 0);

	hogl::output *logout;
	if (file)
		logout = new hogl::output_textfile(file, logfmt, 16 * 1024);
	else
		logout = new hogl::output_stderr(logfmt, 16 * 1024);
	hogl::activate(*logout);

	test_area = hogl::add_area("TEST-AREA", test_sect_names);

	// Test double add (with reuse)
	test_area = hogl::add_area("TEST-AREA", test_sect_names);

	hogl::apply_mask(logmask);

	if (doTest() < 0) {
		printf("Failed\n");
		return 1;
	}

	fflush(stdout);
	fflush(stderr);

	hogl::string_list alist, rlist;
	hogl::list_areas(alist);
	hogl::list_rings(rlist);

	hogl::string_list::const_iterator it;

	std::cerr << "Areas:" << std::endl;
	for (it = alist.begin(); it != alist.end(); ++it) {
		const hogl::area *area = hogl::find_area(it->c_str());
		if (area) {
			std::cerr.width(4);
			std::cerr << *area;
		}
	}

	std::cerr << "Rings:" << std::endl;
	for (it = rlist.begin(); it != rlist.end(); ++it) {
		const hogl::ringbuf *ring = hogl::find_ring(it->c_str());
		if (ring) {
			std::cerr.width(4);
			std::cerr << *ring;
			ring->release();
		}
	}

	hogl::deactivate();

	delete logout;

	printf("Passed\n");
	return 0;
}
