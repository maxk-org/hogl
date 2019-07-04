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
#include "hogl/format-raw.hpp"
#include "hogl/output-stdout.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/output-textfile.hpp"
#include "hogl/output-pipe.hpp"
#include "hogl/engine.hpp"
#include "hogl/area.hpp"
#include "hogl/mask.hpp"
#include "hogl/post.hpp"
#include "hogl/flush.hpp"
#include "hogl/ring.hpp"

__HOGL_PRIV_NS_USING__;

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

#define dbglog(level, fmt, args...) hogl::post(test_area, TEST_##level, fmt, ##args)

// Dummy class to print sizes of the internal ringbuf parts
class myring : public hogl::ringbuf {
public:
	myring() :
		ringbuf("dummy")
	{
		uint8_t *start = (uint8_t *) this;
		uint8_t *head  = (uint8_t *) &this->_head;
		uint8_t *tail  = (uint8_t *) &this->_tail;

		printf("ring: tail offset %lu head offset %lu\n",
			(unsigned long) (tail - start), (unsigned long) (head - start)); 
	}
};

static char really_long_string[8192];
static char insanely_long_string[64*1024];

void *run_thread_locked(void *unused)
{
	hogl::ringbuf::options ring_opts = { .capacity = 1024, .prio = 100, .flags = 0, .record_tailroom = 128 };
	hogl::tls tls("LOCKED", ring_opts);

	hogl::post(test_area, TEST_INFO, "tls::ring %p", hogl::tls::ring());

	// Post some test messages.
	// This will use TLS ring.

	hogl::post(test_area, TEST_DEBUG, "debug message %u %llu %u", 1, (uint64_t) 2, 3);
	hogl::post(test_area, TEST_ERROR, "error message %u %u %u %u %010d",
			10, 20, 30, 40, -50);
	hogl::post(test_area, TEST_WARN,  "warning message (floating point) %.3f", 1234.567);
	hogl::post(test_area, TEST_EXTRA_INFO, "extra:info message");

	hogl::post(test_area, TEST_INFO, "string args test [%s]", "this is my string");
	hogl::post(test_area, TEST_INFO, "null string arg test [%s]", (const char *)0);

	hogl::post(test_area, TEST_INFO, "function %s", __PRETTY_FUNCTION__);

	hogl::post(test_area, TEST_INFO, hogl::arg_gstr("gstring test"));
	hogl::post(test_area, TEST_INFO, 
			hogl::arg_gstr("gstring test with arg [%s]"), hogl::arg_gstr("gstr as argument"));

	hogl::post(test_area, TEST_TRACE, 1111111, 2.2, "str", &tls, 5, 6, 7, 8);

	uint8_t u8[4] = { 1,2,3,4 };
	hogl::post(test_area, TEST_TRACE, "char array %u %u %u %u", u8[0], u8[1], u8[2], u8[3]);

	float fa[4] = { 1.0, 10.5, 33.6, 40.6 };
	hogl::post(test_area, TEST_INFO, "float array %f %f %f %f", fa[0], fa[1], fa[2], fa[3]);

	double da[4] = { 1.0, 10.5, 33.6, 40.6 };
	hogl::post(test_area, TEST_INFO, "double array %f %f %f %f", da[0], da[1], da[2], da[3]);

	dbglog(INFO, "info message via wrapper macro");
	dbglog(EXTRA_INFO, "extra info message via wrapper macro, %u", 1000);

	// Populate data pattern for hexdump
	uint8_t data[120];
	for (unsigned int i = 0; i < sizeof(data); i++)
		data[i] = i;

	hogl::post(test_area, TEST_HEXDUMP, hogl::arg_hexdump(data, sizeof(data)));
	hogl::post(test_area, TEST_INFO, "%s", hogl::arg_hexdump(data, sizeof(data)));

	// Post raw data for kicks
	hogl::post(test_area, TEST_INFO, hogl::arg_raw(data, sizeof(data)));
	hogl::post(test_area, TEST_INFO, "%s", hogl::arg_raw(data, sizeof(data)));

	// Post records with >8 args
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %u %u"), 1, 2, 3, 4, 5, 6, 7, 8);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %s %u %u"), 1, 2, 3, 4, 5, "str6", 7, 8);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %s %u"), 1, 2, 3, 4, 5, 6, hogl::arg_gstr("gstr7"), 8);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %s %u"), 1, 2, 3, 4, 5, 6, "str7", 8);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %s %u %u"), 1, 2, 3, 4, 5, 6, "str7", 8, 9);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %s %u %u %u %u %u %u %u"), 1, 2, 3, 4, 5, 6, "str7", 8, 9, 10, 11, 12, 13, 14);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %u %s"), 1, 2, 3, 4, 5, 6, 7, hogl::arg_gstr("gstr8"));
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %u %u %u %u %u %u %u %s"), 1, 2, 3, 4, 5, 6, 7, "str8");
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %f %f %f %f %f %f %f %f %f"), 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %f %f %f %f %f %f %f %s %f"), 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, "str8.8", 9.9);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %f %f %f %f %f %f %f %f %f %f"), 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %f %f %f %f %f %f %f %f %f %f %f"), 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.10, 11.11);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %s %s %s %s %s %s %s %s %s %s %s %s"), "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12");
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %d %d %d %d %s %s %s %d %d %d %d %d %d"), -1, -2, -3, -4, "s5", "s6", "s7", -8, -9, -10, -11, -12, -13);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %d %d %d %d %s %s %s %d %d %d %d %d %d %f"), -1, -2, -3, -4, "s5", "s6", "s7", -8, -9, -10, -11, -12, -13, 14.14);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %d %d %d %d %d %d %d %d %d %d %d %d %d %f"), -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 14.14);
	hogl::post(test_area, TEST_INFO, hogl::arg_gstr(">eight %d %d %d %d %d %d %d %d %d %d %d %d %d %f %u"), -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, 14.14, 15);

	int  t_i = 1;
	long t_l = 2;
	long long t_ll = 3;
	unsigned int  t_ui = 4;
	unsigned long t_ul = 5;
	unsigned long long t_ull = 6;

	hogl::post(test_area, TEST_INFO, "%d",   t_i);
	hogl::post(test_area, TEST_INFO, "%ld",  t_l);
	hogl::post(test_area, TEST_INFO, "%lld", t_ll);
	hogl::post(test_area, TEST_INFO, "%u",   t_ui);
	hogl::post(test_area, TEST_INFO, "%lu",  t_ul);
	hogl::post(test_area, TEST_INFO, "%llu", t_ull);

	int16_t  t_i16 = 1;
	int32_t  t_i32 = 2;
	int64_t  t_i64 = 3;
	uint16_t t_u16 = 1;
	uint32_t t_u32 = 2;
	uint64_t t_u64 = 3;

	hogl::post(test_area, TEST_INFO, "%d",   t_i16);
	hogl::post(test_area, TEST_INFO, "%ld",  t_i32);
	hogl::post(test_area, TEST_INFO, "%lld", t_i64);
	hogl::post(test_area, TEST_INFO, "%u",   t_u16);
	hogl::post(test_area, TEST_INFO, "%lu",  t_u32);
	hogl::post(test_area, TEST_INFO, "%llu", t_u64);

	hogl::flush();
	return 0;
}

void *run_thread_unlocked(void *unused)
{
	hogl::ringbuf::options ring_opts = { .capacity = 1024, .prio = 100 };
	hogl::tls tls("UNLOCKED", ring_opts);

	hogl::post_unlocked(test_area, TEST_INFO, "tls::ring %p", hogl::tls::ring());

	// Post some test messages.
	// This will use TLS ring.
	hogl::post_unlocked(test_area, TEST_DEBUG, "debug message %u %u %u", 1, 2, 3);
	hogl::post_unlocked(test_area, TEST_ERROR, "error message %u %u %u %u %010d", 10, 20, 30, 40, -50);
	hogl::post_unlocked(test_area, TEST_WARN,  "warning message (floating point) %.3f", 1234.567);
	hogl::post_unlocked(test_area, TEST_EXTRA_INFO, "extra:info message");
	hogl::post_unlocked(test_area, TEST_INFO, "string args test [%s]", "this is my string");
	hogl::post_unlocked(test_area, TEST_INFO, "function %s", __PRETTY_FUNCTION__);
	hogl::post_unlocked(test_area, TEST_INFO, hogl::arg_gstr("gstring test"));
	hogl::post_unlocked(test_area, TEST_INFO, hogl::arg_gstr("gstring test with arg [%s]"), hogl::arg_gstr("gstr as argument"));
	hogl::post_unlocked(test_area, TEST_TRACE, 1111111, 2.2, "str", &tls, 5, 6, 7, 8);

	uint8_t u8[4] = { 1,2,3,4 };
	hogl::post_unlocked(test_area, TEST_TRACE, "char array %u %u %u %u", u8[0], u8[1], u8[2], u8[3]);

	float fa[4] = { 1.0, 10.5, 33.6, 40.6 };
	hogl::post_unlocked(test_area, TEST_INFO, "float array %f %f %f %f", fa[0], fa[1], fa[2], fa[3]);

	dbglog(INFO, "info message via wrapper macro");
	dbglog(EXTRA_INFO, "extra info message via wrapper macro, %u", 1000);

	// Populate data pattern for hexdump
	uint8_t data[120];
	for (unsigned int i = 0; i < sizeof(data); i++)
		data[i] = i;

	hogl::post_unlocked(test_area, TEST_HEXDUMP, hogl::arg_hexdump(data, sizeof(data)));
	hogl::post_unlocked(test_area, TEST_INFO, "%s", hogl::arg_hexdump(data, sizeof(data)));

	// Post raw data for kicks
	hogl::post_unlocked(test_area, TEST_INFO, hogl::arg_raw(data, sizeof(data)));
	hogl::post_unlocked(test_area, TEST_INFO, "%s", hogl::arg_raw(data, sizeof(data)));

	hogl::flush();
	return 0;
}

void __attribute__((noinline)) do_post(const hogl::area *a, unsigned int section, 
						uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
{
	hogl::post(a, section, a0, a1, a2, a3);
}

void __attribute__((noinline)) do_call()
{
	do_post(test_area, TEST_TRACE, 0x11111111, 0x22222222, 0x33333333, 0x44444444);
}

int doTest()
{
	do_call();

	{
		pthread_t t;
		if (pthread_create(&t, 0, run_thread_locked, 0) < 0) {
			perror("Failed to create test thread");
			exit(1);
		}
		pthread_join(t, NULL);
	}
	{
		pthread_t t;
		if (pthread_create(&t, 0, run_thread_unlocked, 0) < 0) {
			perror("Failed to create test thread");
			exit(1);
		}
		pthread_join(t, NULL);
	}

	// Post some test messages.
	// This will use the default ring.
	for (unsigned int i = 0; i < sizeof(really_long_string)-1; i++)
		really_long_string[i] = 'A' + i % 26;
	really_long_string[sizeof(really_long_string)-1] = '\0';
	for (unsigned int i = 0; i < sizeof(insanely_long_string)-1; i++)
		insanely_long_string[i] = 'A' + i % 26;
	insanely_long_string[sizeof(insanely_long_string)-1] = '\0';

	volatile uint32_t *u32_ptr = 0;
	hogl::post(test_area, TEST_INFO, "volatile u32 pointer %p", u32_ptr);
	
	hogl::post(test_area, TEST_INFO, "really long string\n%s", really_long_string);
	hogl::post(test_area, TEST_INFO, "insanely long string\n%s", insanely_long_string);
	hogl::post(test_area, TEST_INFO, "really long string\n%s", hogl::arg_gstr(really_long_string));
	hogl::post(test_area, TEST_INFO, "insanely long string\n%s", hogl::arg_gstr(insanely_long_string));
	hogl::post(test_area, TEST_INFO, "insanely long cstring\n%s", insanely_long_string);

	hogl::post(test_area, TEST_DEBUG, "simple debug message %u %u %u", 1, 2, 3);
	hogl::post(test_area, TEST_ERROR, "simple error message %u %u %u %u %u",
			10, 20, 30, 40, 50);
	hogl::post(test_area, TEST_EXTRA_INFO, "simple extra:info message");

	hogl::timestamp t0(2000);
	hogl::timestamp t1(1000);
	uint64_t ts_delta = t1 - t0;
	hogl::post(test_area, TEST_INFO, "timestamp delta %llu", ts_delta);

	return 0;
}

// Command line args {
static struct option main_lopts[] = {
   {"help",    0, 0, 'h'},
   {"format",  1, 0, 'f'},
   {"output",  1, 0, 'o'},
   {"output-bufsize", 1, 0, 'O'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:o:O:";

static char main_help[] =
   "HOGL basic test 0.1 \n"
   "Usage:\n"
      "\tbasic_test [options]\n"
   "Options:\n"
      "\t--help -h            Display help text\n"
      "\t--format -f <name>   Log format (basic, raw)\n"
      "\t--output -o <name>        Log output (file name, stderr, pipe)\n"
      "\t--output-bufsize -O <N>   Output buffer size (in bytes)\n";
// }

int main(int argc, char *argv[])
{
	std::string log_output("stdout");
	std::string log_format("fast1");
	unsigned int output_bufsize = 10 * 1024 * 1024;
	int opt;

	// Parse command line args
	while ((opt = getopt_long(argc, argv, main_sopts, main_lopts, NULL)) != -1) {
		switch (opt) {
		case 'f':
			log_format = optarg;
			break;

		case 'o':
			log_output = optarg;
			break;
	
		case 'O':
			output_bufsize = atoi(optarg);
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

	hogl::format *lf;
	hogl::output *lo;

	if (log_format == "raw")
		lf = new hogl::format_raw();
	else
		lf = new hogl::format_basic(log_format.c_str());

	if (log_output == "stderr")
		lo = new hogl::output_stderr(*lf, output_bufsize);
	else if (log_output == "stdout")
		lo = new hogl::output_stdout(*lf, output_bufsize);
	else if (log_output[0] == '|')
		lo = new hogl::output_pipe(log_output.substr(1).c_str(), *lf, output_bufsize);
	else
		lo = new hogl::output_textfile(log_output.c_str(), *lf, output_bufsize);

	hogl::mask logmask(".*", ".*:DEBUG", 0);

	hogl::activate(*lo);

	test_area = hogl::add_area("TEST-AREA", test_sect_names);

	// Test double add (with reuse)
	test_area = hogl::add_area("TEST-AREA", test_sect_names);

	// Test double add (failed)
	const char *a0_sections[] = { "X", 0 };
	const hogl::area *a0 = hogl::add_area("TEST-AREA", a0_sections);
	assert(a0 == 0);

	hogl::apply_mask(logmask);

	printf("sizeof(hogl::record)  = %lu\n", (unsigned long) sizeof(hogl::record));
	printf("sizeof(hogl::ringbuf) = %lu\n", (unsigned long) sizeof(hogl::ringbuf));
	//printf("sizeof(hogl::post_context) = %lu\n", (unsigned long) sizeof(hogl::post_context));
	myring mring;

	if (doTest() < 0) {
		printf("Failed\n");
		return 1;
	}

	fflush(stdout);
	fflush(stderr);

	std::cerr << *hogl::default_engine;

	hogl::deactivate();

	delete lo;
	delete lf;

	printf("Passed\n");
	return 0;
}
