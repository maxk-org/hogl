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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <sstream>

#include "hogl/detail/format.hpp"
#include "hogl/detail/output.hpp"
#include "hogl/detail/ntos.hpp"
#include "hogl/detail/internal.hpp"
#include "hogl/format-basic.hpp"
#include "hogl/format-raw.hpp"
#include "hogl/format-null.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/output-stdout.hpp"
#include "hogl/output-plainfile.hpp"
#include "hogl/output-file.hpp"
#include "hogl/output-pipe.hpp"
#include "hogl/output-null.hpp"
#include "hogl/output-tee.hpp"
#include "hogl/post.hpp"
#include "hogl/flush.hpp"
#include "hogl/area.hpp"
#include "hogl/timesource.hpp"
#include "hogl/platform.hpp"

__HOGL_PRIV_NS_USING__;

// Burst data struct.
// Used for storing burst data in the stress threads, and with RAW records.
struct burst_data {
	unsigned int sample[4];
};

// Custom format with header and footer
class custom_format : public hogl::format_basic {
public:
	custom_format() :
		hogl::format_basic("fast1")
	{}

	void header(hogl::ostrbuf &sb, const char *n, bool first)
	{
		sb.printf("------- %s stress test header (this output %s) -------\n", first ? "first" : "", n);
	}

	void footer(hogl::ostrbuf &sb, const char *n)
	{
		sb.printf("------- %s stress test footer (next output %s) -------\n", !n ? "last" : "", n);
	}

	void raw_one_line(hogl::ostrbuf &sb, unsigned int i, const burst_data &bd, unsigned int last = 0) const
	{
		uint8_t str[256];
        	unsigned int n = 0;

		// The code below is functionally equivalent to the following:
		//     sb.printf(" %x:%x,%x,%x,%x\n", i,
		//		bd[i].sample[0], bd[i].sample[1],
		//		bd[i].sample[2], bd[i].sample[3]);
		str[n++] = ' ';
		hogl::u64tox(i, str, n); str[n++] = ':';
		hogl::u64tox(bd.sample[0], str, n); str[n++] = ',';
		hogl::u64tox(bd.sample[1], str, n); str[n++] = ',';
		hogl::u64tox(bd.sample[2], str, n); str[n++] = ',';
		hogl::u64tox(bd.sample[3], str, n);
		str[n++] = '\n';
		sb.put(str, n - last);
	}

	void output_raw(hogl::ostrbuf &sb, const hogl::record &r) const
	{
		unsigned int len; const burst_data *bd = (const burst_data *) r.get_arg_data(0, len);
		unsigned int count = len / sizeof(burst_data);

		sb.cat('\n');
		unsigned int i;
		for (i=0; i < count - 1; i++)
			raw_one_line(sb, i, bd[i]);
		raw_one_line(sb, i, bd[i], 1);
	}
};

// Format that generates record stats
class stats_format : public hogl::format {
private:
	uint64_t        _tmax_nsec;
	uint64_t        _max;
	uint64_t        _min;

	hogl::timestamp _last;

public:
	stats_format(uint64_t tmax_nsec=0) : _tmax_nsec(tmax_nsec), _max(0), _min(~0ULL), _last(0)
	{}

	void process(hogl::ostrbuf&, const hogl::format::data &d)
	{
		const hogl::record &r = *d.record;

		hogl::timestamp delta = r.timestamp - _last;
		if (delta.to_nsec() < _tmax_nsec) {
			if (delta.to_nsec() < _min) _min = delta.to_nsec();
			if (delta.to_nsec() > _max) _max = delta.to_nsec();
		}

		_last = r.timestamp;
	}

	void dump()
	{
		std::cout << "Timestamp delta stats:" << std::endl;
		std::cout << "\tMax:  " << _max << std::endl;
		std::cout << "\tMin:  " << _min << std::endl;
	}
};

// Bad bad timesource 
class bad_timesource : public hogl::timesource {
private:
	struct sequence {
		uint64_t _count;
		sequence() : _count((uint64_t)0 - 1) {}
		uint64_t next() { return __sync_add_and_fetch(&_count, 1); }
	};

	sequence _seq;

	int _badness;

	static hogl::timestamp get_timestamp(bad_timesource *self)
	{
		hogl::timestamp now = hogl::default_timesource.timestamp();

		uint64_t s = self->_seq.next();
		if (s % 20 == 0)
			now += self->_badness;
		return now;
	}

public:
	bad_timesource(int badness) :
		hogl::timesource("bad", reinterpret_cast<hogl::timesource::callback>(get_timestamp))
	{
		_badness = badness;
	}
};

// Custom schedparam, just for fun
class strict_schedparam : public hogl::schedparam {
public:
	bool strict; // enable extra checks

	strict_schedparam() :
		schedparam(), strict(false)
	{ }

	virtual bool thread_enter(const char *title)
	{
		hogl::platform::post_early(hogl::internal::INFO, "applying schedparam: %s", title);

		bool good = schedparam::thread_enter(title);
		if (!good && strict) {
			hogl::platform::post_early(hogl::internal::ERROR, "strict schedparam: %s failed", title);
			abort();
		}
		return true;
	}
};

// Output filename switcher
class output_switcher {
private:
	hogl::output_file *_out;
	std::string        _dir;
	std::string        _file;
	unsigned int       _idx;

public:
	output_switcher(const std::string& dir, const std::string& file) :
		_out(0), _dir(dir), _file(file), _idx(0)
	{
		mkdir();
	}

	std::string dirname()
	{
		std::ostringstream ss;
		ss << _dir << '/' << _idx << '/';
		return ss.str();
	}

	std::string filename()
	{
		return dirname() + _file;
	}

	bool mkdir()
	{
		int err = ::mkdir(dirname().c_str(), 0777);
		if (err < 0 && errno != EEXIST) {
			fprintf(stderr, "failed to created directory %s : %s", dirname().c_str(), strerror(errno));
			return false;
		}
		return true;
	}

	bool init(hogl::output_file* out)
	{
		_out = out;
		return mkdir();
	}

	void next()
	{
		_idx++;
		mkdir();
		_out->switch_name(filename().c_str());
	}
};

// Main thread log sections
enum main_logsect_ids {
	MAIN_DEBUG,
	MAIN_INFO,
	MAIN_ERROR
};

const char *main_logsect_names[] = {
	"DEBUG",
	"INFO",
	"ERROR",
	0
};

const hogl::area *main_logarea;

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
	bool            _flush;
	bool            _use_raw;
	bool            _use_blocking;
	bool            _use_cstr;

	struct burst_data *_burst_data;

	unsigned int    _stat_flush_timeout;

	static void *entry(void *_self);
	void loop();

	enum log_sections {
		DEBUG,
		INFO,
		WARN,
		ERROR,
	};

	static const char *_log_sections[];

public:
	test_thread(const char *name, unsigned int ring_capacity, unsigned int burst_size,
				bool use_raw, bool use_blocking, bool use_cstr,
				unsigned int interval_usec, unsigned int nloops, bool flush);
	~test_thread();

	/**
	 * Check if the engine is running
	 */
	bool running() const { return _running; }

	bool failed() const { return _failed; }

	unsigned int stat_flush_timeout() const { return _stat_flush_timeout; }
};

const char *test_thread::_log_sections[] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	0,
};

test_thread::test_thread(const char *name, unsigned int ring_capacity, unsigned int burst_size,
	bool use_raw, bool use_blocking, bool use_cstr,
	unsigned int interval_usec, unsigned int nloops, bool flush) :
	_name(name),
	_running(true),
	_killed(false),
	_failed(false),
	_ring_capacity(ring_capacity),
	_burst_size(burst_size),
	_interval_usec(interval_usec),
	_nloops(nloops),
	_flush(flush),
	_use_raw(use_raw),
	_use_blocking(use_blocking),
	_use_cstr(use_cstr),
	_burst_data(0)
{
	int err;

	_log_area = hogl::add_area("STRESS", _log_sections);
	if (!_log_area)
		abort();

	err = pthread_create(&_thread, NULL, entry, (void *) this);
	if (err) {
		hogl::post(_log_area, INFO, "failed to create test_thread thread. %d\n", err);
		exit(1);
	}

	hogl::post(_log_area, INFO, "created test_thread %p(%s)", this, _name);

	_burst_data = new burst_data[_burst_size]();

	_stat_flush_timeout = 0;
}

test_thread::~test_thread()
{
	_killed = true;
	pthread_join(_thread, NULL);

	delete [] _burst_data;

	hogl::post(_log_area, INFO, "destroyed test_thread %p(%s)", this, _name);
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
	hogl::ringbuf::options ring_opts = {};
	ring_opts.capacity = _ring_capacity;

	if (_use_raw)
		ring_opts.record_tailroom = std::max((unsigned int) 256, 
				(unsigned int) sizeof(burst_data) * _burst_size);
	else
		ring_opts.record_tailroom = 0;

	if (_use_blocking)
		ring_opts.flags |= hogl::ringbuf::BLOCKING;

	hogl::tls tls(_name.c_str(), ring_opts);

	hogl::post(_log_area, INFO, "test_thread %p(%s) running", this, _name);

	while (!_killed) {
		unsigned int i;

		// Generate new data for this burst
		for (i=0; i < _burst_size; i++) {
			_burst_data[i].sample[0] = _nloops * (i + 1) + 0;
			_burst_data[i].sample[1] = _nloops * (i + 1) + 1;
			_burst_data[i].sample[2] = _nloops * (i + 1) + 2;
			_burst_data[i].sample[3] = _nloops * (i + 1) + 3;
		}

		if (_use_raw) {
			// Raw mode.
			// Log entire burst as one record
			hogl::post_unlocked(_log_area, INFO,
					hogl::arg_raw(_burst_data, sizeof(burst_data) * _burst_size));
		} else if (_use_cstr) {
			// Log each entry separately (cstr format)
			for (i=0; i < _burst_size; i++) {
				hogl::post_unlocked(_log_area, INFO, "%x:%x,%x,%x,%x", i,
						_burst_data[i].sample[0],
						_burst_data[i].sample[1],
						_burst_data[i].sample[2],
						_burst_data[i].sample[3]);
			}
		} else {
			// Default mode.
			// Log each entry separately (gstr format)
			for (i=0; i < _burst_size; i++) {
				hogl::post_unlocked(_log_area, INFO,
					hogl::arg_gstr("%x:%x,%x,%x,%x"), i,
						_burst_data[i].sample[0],
						_burst_data[i].sample[1],
						_burst_data[i].sample[2],
						_burst_data[i].sample[3]);
			}
		}

		if (_flush) {
			if (!hogl::flush())
				_stat_flush_timeout++;
		}

		_nloops--;
		if (!_nloops)
			break;

		usleep(_interval_usec);
	}

	hogl::post(_log_area, INFO, "test_thread %p(%s) stopped", this, _name);
	hogl::flush();

	hogl::ringbuf *r = hogl::tls::ring();
	if (r->dropcnt()) {
		std::cout << *r;
		_failed = true;
	}

	_running = false;
}

// -------

static unsigned int nthreads      = 8;
static unsigned int ring_capacity = 1024;
static unsigned int burst_size    = 10;
static unsigned int interval_usec = 1000;
static unsigned int nloops        = 20000;
static bool         flush         = false;
static bool         use_raw       = false;
static bool         use_blocking  = false;
static bool         use_cstr      = false;
static int64_t      ts_badness    = 0;

static output_switcher* out_switcher;

static int doTest()
{
	test_thread *thread[nthreads];

	unsigned int i;
	for (i=0; i < nthreads; i++) {
		char name[100];
		sprintf(name, "THREAD%u", i);
		hogl::post(main_logarea, MAIN_INFO, "starting thread #%u (%s)", i, name);
		thread[i] = new test_thread(name, ring_capacity, burst_size, use_raw, use_blocking, use_cstr,
						interval_usec, nloops, i == 0 ? flush : false);
	}

	while (1) {
		bool all_dead = true;
		for (i=0; i < nthreads; i++) {
			if (thread[i]->running())
				all_dead = false;
		}

		if (all_dead)
			break;

		usleep(200000);

		if (out_switcher)
			out_switcher->next();
	}

	bool failed = false;
	for (i=0; i < nthreads; i++) {
		hogl::post(main_logarea, MAIN_INFO, "killing thread #%u", i);
		failed |= thread[i]->failed();

		if (thread[i]->stat_flush_timeout()) {
			std::cout << "thread #" << i << " had " <<  thread[i]->stat_flush_timeout() << " flush timeouts" << std::endl;
			failed = true;
		}

		delete thread[i];
	}

	return failed ? -1 : 0;
}

hogl::output* create_output(std::string& out, std::string& dir_switch, hogl::format& fmt, size_t bufsize, size_t filesize)
{
	if (out == "null")
		return new hogl::output_null(fmt, bufsize);

	if (out == "stderr")
		return new hogl::output_stderr(fmt, bufsize);

	if (out == "stdout")
		return new hogl::output_stdout(fmt, bufsize);

	if (out[0] == '|')
		return new hogl::output_pipe(out.substr(1).c_str(), fmt, bufsize);

	if (out.find('#') != out.npos) {
		// Enable output directory switching
		if (!dir_switch.empty()) {
			out_switcher = new output_switcher(dir_switch, out);

			out = out_switcher->filename();
		}

		hogl::output_file::options opts = {
			.perms     = 0666,
			.max_size  = filesize,
			.max_count = 20,
			.buffer_capacity = bufsize,
			.schedparam = 0
		};

		hogl::output_file* o = new hogl::output_file(out.c_str(), fmt, opts);
		if (!dir_switch.empty())
			out_switcher->init(o);
		return o;
	}

	return new hogl::output_plainfile(out.c_str(), fmt, bufsize);
}

// Command line args {
static struct option main_lopts[] = {
   {"help",        0, 0, 'h'},
   {"format",      1, 0, 'f'},
   {"output",      1, 0, 'o'},
   {"out-buff-size", 1, 0, 'O'},
   {"out-file-size", 1, 0, 'R'},
   {"out-tee",       1, 0, 't'},
   {"out-switch-dir",1, 0, 'D'},
   {"nthreads",    1, 0, 'n'},
   {"ring-size",   1, 0, 'r'},
   {"burst-size",  1, 0, 'b'},
   {"raw",         0, 0, 'W'},
   {"blocking",    0, 0, 'w'},
   {"cstr",        0, 0, 'C'},
   {"interval",    1, 0, 'i'},
   {"nloops",      1, 0, 'l'},
   {"notso",       0, 0, 'N'},
   {"tso-buffer",  1, 0, 'T'},
   {"poll-interval",  1, 0, 'p'},
   {"eng-cpu-affi",   1, 0, 'A'},
   {"eng-prio",    1, 0, 'P'},
   {"eng-ssched",  1, 0, 'S'},
   {"flush",       0, 0, 'F'},
   {"ts-badness",  1, 0, 'B'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:o:O:t:D:R:n:r:b:wi:l:p:N:T:A:P:S:FB:WC";

static char main_help[] =
   "HOGL stress test\n"
   "Usage:\n"
      "\tstress_test [options]\n"
   "Options:\n"
      "\t--help -h               Display help text\n"
      "\t--format -f <name>      Log format (null, basic)\n"
      "\t--output -o <name>      Log output (file name, stderr, or null)\n"
      "\t--out-bufsize -O <N>    Output buffer size (number of bytes)\n"
      "\t--out-file-size -R <S>  Maximum size of each output file chunk (in megabytes)\n"
      "\t--out-tee -t <name>     Tee the main output into: file name, stderr, stderror, pipe\n"
      "\t--out-switch-dir -D <name> Enable output directory switching\n"
      "\t--poll-interval -p <N>  Engine polling interval (in usec)\n"
      "\t--nthreads -n <N>       Number of threads to start\n"
      "\t--ring -r <N>           Ring size (number of records)\n"
      "\t--burst-size -b <N>     Burst size (number of records)\n"
      "\t--raw -W                Use RAW records for bursting. Each record contains 'burst-size' number of entries\n"
      "\t--blocking -w           Use Blocking mode for per-thread rings\n"
      "\t--cstr -C               Use CSTR instead of GSTR\n"
      "\t--interval -i <N>       Interval between bursts (in usec)\n"
      "\t--nloops -l <N>         Number of loops in each thread\n"
      "\t--notso -N              Disable timestamp ordering (TSO)\n"
      "\t--tso-buffer -T <N>     TSO buffer size (number of records)\n"
      "\t--flush -F              Make one of the threads call flush every iteration\n"
      "\t--eng-cpu-affi -A <A>   Set hogl::engine CPU affinity\n"
      "\t--eng-prio -P <N>       Set hogl::engine scheduling priority (with SCHED_FIFO)\n"
      "\t--eng-ssched -S <N>     Use strict (extra checks) schedparam for the hogl::engine\n"
      "\t--ts-badness -B <N>     Timesource badness N (0 - perfect, +/- badness delta)\n";
// }

int main(int argc, char *argv[])
{
	int opt;

	hogl::engine::options log_eng_opts = hogl::engine::default_options;
	std::string log_output("null");
	std::string log_format("custom");
	size_t      out_file_size = 1024 * 1024 * 1024;
	size_t      out_buff_size = 64 * 1024;
	std::string out_tee;
	std::string out_switch_dir;

	// Enable our custom schedparam
	strict_schedparam ssched;
	log_eng_opts.schedparam = static_cast<hogl::schedparam*>(&ssched);

	// Parse command line args
	while ((opt = getopt_long(argc, argv, main_sopts, main_lopts, NULL)) != -1) {
		switch (opt) {
		case 'f':
			log_format = optarg;
			break;

		case 'o':
			log_output = optarg;
			break;

		case 'D':
			out_switch_dir = optarg;
			break;

		case 't':
			out_tee = optarg;
			break;

		case 'R':
			out_file_size = atoi(optarg) * 1024 * 1024;
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

		case 'W':
			use_raw = true;
			break;

		case 'w':
			use_blocking = true;
			break;

		case 'C':
			use_cstr = true;
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
			log_eng_opts.features |= hogl::engine::DISABLE_TSO;
			break;

		case 'T':
			log_eng_opts.tso_buffer_capacity = atoi(optarg);
			break;

		case 'O':
			out_buff_size = atoi(optarg);
			break;

		case 'F':
			flush = true;
			break;

		case 'B':
			ts_badness = strtol(optarg, 0, 0);
			break;

		case 'P':
			ssched.priority = strtol(optarg, 0, 0);
			if (ssched.priority)
				ssched.policy = SCHED_FIFO;
			break;

		case 'A':
			ssched.cpu_affinity = optarg;
			break;

		case 'S':
			ssched.strict = strtol(optarg, 0, 0);
			break;

		case 'h':
		default:
			std::cout << main_help;
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;
	
	if (argc < 0) {
		std::cout << main_help;
		exit(1);
	}

	hogl::format *lf;
	hogl::output *lo[3] = { 0, 0, 0 };

	if (log_format == "null")
		lf = new hogl::format_null();
	else if (log_format == "raw")
		lf = new hogl::format_raw();
	else if (log_format == "custom")
		lf = new custom_format();
	else if (log_format == "stats")
		lf = new stats_format(interval_usec * 1000 /* max threshold in nsec */);
	else
		lf = new hogl::format_basic(log_format.c_str());

	if (out_tee.empty()) {
		lo[0] = create_output(log_output, out_switch_dir, *lf, out_buff_size, out_file_size);
	} else {
		lo[1] = create_output(log_output, out_switch_dir, *lf, 0, out_file_size);
		lo[2] = create_output(out_tee, out_switch_dir, *lf, 0, out_file_size);
		lo[0] = new hogl::output_tee(lo[1], lo[2], out_buff_size);
	}

	hogl::activate(*lo[0], log_eng_opts);
	hogl::platform::enable_verbose_coredump();

	main_logarea = hogl::add_area("MAIN", main_logsect_names);

	// Replace default TLS for the main thread
	hogl::ringbuf::options ringopts = { .capacity = 1024, .prio = 10, .flags = 0,  .record_tailroom = 128};
	hogl::tls *tls = new hogl::tls("MAIN", ringopts);

	// Replace timesource if needed
	bad_timesource *bad_ts = 0;
	if (ts_badness) {
		bad_ts = new bad_timesource(ts_badness);
		if (!hogl::change_timesource(bad_ts)) {
			hogl::post(main_logarea, MAIN_ERROR, "failed to switch to bad timesource");
			hogl::flush();
			abort();
		}
	}

	int err = doTest();

	std::cout << "Engine stats: " << std::endl;
	std::cout << hogl::default_engine->get_stats();

	delete tls;
	hogl::deactivate();

	if (log_format == "stats")
		static_cast<stats_format *>(lf)->dump();

	delete out_switcher;

	delete bad_ts;
	for (unsigned i=0; i<3; i++) delete lo[i];
	delete lf;

	if (err < 0) {
		std::cout << "Failed\n";
		return 1;
	}

	std::cout << "Passed\n";
	return 0;
}
