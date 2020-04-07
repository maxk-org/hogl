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

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include <sstream>
#include <boost/format.hpp>

#include "hogl/detail/refcount.hpp"
#include "hogl/output-null.hpp"
#include "hogl/post.hpp"
#include "hogl/area.hpp"
#include "hogl/mask.hpp"
#include "hogl/platform.hpp"
#include "hogl/fmt/format.h"

#define SKIP 77

__HOGL_PRIV_NS_USING__;

// Test output that verifies seq numbers
class test_format : public hogl::format {
private:
	uint64_t _last;
	uint64_t _ooo;

public:
	test_format() : _last(0), _ooo(0)
	{ }

	void process(hogl::ostrbuf&, const hogl::format::data& d)
	{
		uint64_t ts = d.record->timestamp;
		if (ts < _last)
			_ooo++;
		else
			_last = ts;
	}

	unsigned int ooo() const { return _ooo; }
};

// Sequence count
class sequence {
private:
	uint64_t _count;

public:
	sequence() : _count((uint64_t)0 - 1) {}
	uint64_t next() { return __sync_add_and_fetch(&_count, 1); }
};

sequence global_sequence; 

// Test engine
class test_thread {
private:
	const hogl::area   *_log_area;

	std::string     _name;
	pthread_t       _thread;
	volatile bool   _running;
	volatile bool   _killed;
	bool            _failed;

	unsigned int    _ring_capacity;
	unsigned int    _burst_size;
	unsigned int    _interval_usec;
	unsigned int    _nloops;

	static void *entry(void *_self);
	void loop();

	enum log_sections {
		INFO
	};

	static const char *_log_sections[];

public:
	test_thread(const std::string& name, unsigned int ring_capacity, unsigned int burst_size,
				unsigned int interval_usec, unsigned int nloops);
	~test_thread();

	/**
	 * Check if the engine is running
	 */
	bool running() const { return _running; }

	bool failed() const { return _failed; }
};

const char *test_thread::_log_sections[] = {
	"INFO",
	0,
};

test_thread::test_thread(const std::string& name, unsigned int ring_capacity, unsigned int burst_size, 
	unsigned int interval_usec, unsigned int nloops) :
	_name(name),
	_running(true),
	_killed(false),
	_failed(false),
	_ring_capacity(ring_capacity),
	_burst_size(burst_size),
	_interval_usec(interval_usec),
	_nloops(nloops)
{
	int err;

	_log_area = hogl::add_area("SORT", _log_sections);
	if (!_log_area) {
		fmt::fprintf(stderr, "failed to allocate log area.\n");
		abort();
	}

	err = pthread_create(&_thread, NULL, entry, (void *) this);
	if (err) {
		fmt::fprintf(stderr, "failed to create test_thread thread. %d\n", err);
		abort();
	}
}

test_thread::~test_thread()
{
	_killed = true;
	pthread_join(_thread, NULL);
}

void *test_thread::entry(void *_self)
{
	test_thread *self = (test_thread *) _self;

	hogl::platform::set_thread_title(self->_name.c_str());

	// Run the loop
	self->loop();
	return 0;
}

void test_thread::loop()
{
	_running = true;

	// Create private thread ring
	hogl::ringbuf::options ring_opts = { .capacity = _ring_capacity, .prio = 0, .flags = 0, .record_tailroom = 128 };
	hogl::tls tls(_name.c_str(), ring_opts);

	while (!_killed) {
		unsigned int i;
		for (i=0; i < _burst_size; i++)
			hogl::post_unlocked(_log_area, INFO, global_sequence.next());

		_nloops--;
		if (!_nloops)
			break;

		usleep(_interval_usec);
	}

	if (tls.ring()->dropcnt()) 
		fmt::printf("%s drop count %lu\n", _name.c_str(), (unsigned long) tls.ring()->dropcnt());

	_running = false;
}

// -------

static unsigned int nthreads      = 8;
static unsigned int ring_capacity = 1024;
static unsigned int burst_size    = 10;
static unsigned int interval_usec = 1000;
static unsigned int nloops        = 20000;

static hogl::engine::options log_eng_opts = hogl::engine::default_options;

static std::string log_output("null");
static std::string log_format("basic");

int doTest()
{
	test_thread *thread[nthreads];

	unsigned int i;
	for (i=0; i < nthreads; i++) {
		std::string name = fmt::sprintf("THREAD%u", i);
		thread[i] = new test_thread(name, ring_capacity, burst_size, interval_usec, nloops);
	}

	while (1) {
		bool all_dead = true;
		for (i=0; i < nthreads; i++) {
			if (thread[i]->running())
				all_dead = false;
		}

		if (all_dead)
			break;

		usleep(10000);
	}

	bool failed = false;
	for (i=0; i < nthreads; i++) {
		failed |= thread[i]->failed();
		delete thread[i];
	}

	return failed ? -1 : 0;
}

// Command line args {
static struct option main_lopts[] = {
   {"help",    0, 0, 'h'},
   {"format",      1, 0, 'f'},
   {"output",      1, 0, 'o'},
   {"nthreads",    1, 0, 'n'},
   {"ring-size",   1, 0, 'r'},
   {"burst-size",  1, 0, 'b'},
   {"interval",    1, 0, 'i'},
   {"nloops",      1, 0, 'l'},
   {"notso",       0, 0, 'N'},
   {"tso-buffer",  1, 0, 'T'},
   {"poll-interval", 1, 0, 'p'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:o:n:r:b:i:l:p:N:T:";

static char main_help[] =
   "FurLog stress test 0.1 \n"
   "Usage:\n"
      "\tstress_test [options]\n"
   "Options:\n"
      "\t--help -h              Display help text\n"
      "\t--format -f <name>     Log format (null, basic)\n"
      "\t--output -o <name>     Log output (file name, stderr, or null)\n"
      "\t--poll-interval -p <N> Engine polling interval (in usec)\n"
      "\t--nthreads -n <N>      Number of threads to start\n"
      "\t--ring -r <N>          Ring size (number of records)\n"
      "\t--burst -b <N>         Burst size (number of records)\n"
      "\t--interval -i <N>      Interval between bursts (in usec)\n"
      "\t--notso -N             Disable timestamp ordering (TSO)\n"
      "\t--tso-buffer -T <N>    TSO buffer size (number of records)\n"
      "\t--nloops -l <N>        Number of loops in each thread\n";
// }

int main(int argc, char *argv[])
{
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

		case 'n':
			nthreads = atoi(optarg);
			break;

		case 'r':
			ring_capacity = atoi(optarg);
			break;

		case 'b':
			burst_size = atoi(optarg);
			break;

		case 'i':
			interval_usec = atoi(optarg);
			break;

		case 'l':
			nloops = atoi(optarg);
			break;

		case 'p':
			log_eng_opts.polling_interval_usec = atoi(optarg);
			break;

		case 'N':
			log_eng_opts.features &= ~hogl::engine::DISABLE_TSO;
			break;

		case 'T':
			log_eng_opts.tso_buffer_capacity = atoi(optarg);
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
	
	test_format fmt;
	hogl::output_null lo(fmt);

	hogl::activate(lo, log_eng_opts);
	hogl::platform::enable_verbose_coredump();

	// Disable everything but our own output to avoid noise
	hogl::mask m("!.*", "SORT:INFO", 0);
	hogl::apply_mask(m);

	int err = doTest();

	std::cout << "Engine stats: " << std::endl;
	std::cout << hogl::default_engine->get_stats();

	fmt::printf("Number of OOO records %u\n", fmt.ooo());

	hogl::deactivate();

	if (err < 0 || fmt.ooo()) {
		fmt::printf("Failed\n");
		return SKIP;
	}

	fmt::printf("Passed\n");
	return 0;
}
