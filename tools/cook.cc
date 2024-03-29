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

#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "hogl/detail/ostrbuf-stdio.hpp"
#include "hogl/detail/ringbuf.hpp"
#include "hogl/detail/magic.hpp"
#include "hogl/plugin/format.hpp"
#include "hogl/format-basic.hpp"
#include "hogl/area.hpp"
#include "hogl/fmt/printf.h"

#include "raw-parser.hpp"
#include "rdbuf.hpp"

__HOGL_PRIV_NS_USING__;

using hogl::raw_parser;

static unsigned int min_valid_recs  = 4; // 4 valid records seems reasoanble for most cases
static unsigned int max_record_size = 10 * 1024 * 1024; // 10MB should be plenty for all cases
static unsigned int output_buf_size = 1 * 1024 * 1024;
static unsigned int version = raw_parser::V1_1;

// Returns true if processing is complete, and false if we need to retry.
// Calls exit() directly on critical errors.
static bool reprocess(const std::string& infile, unsigned int flags, size_t offset, hogl::format &fmt, hogl::ostrbuf_stdio& out)
{
	// Open input
	hogl::file_rdbuf in(infile, flags);
	if (!in.valid()) {
		fmt::fprintf(stderr, "failed to open file %s, %s(%d)\n", infile, strerror(in.error()), in.error());
		exit(1);
	}

	// Allocate raw parser
	hogl::raw_parser parser(in, version, max_record_size);
	if (parser.failed()) {
		fmt::fprintf(stderr, "failed to allocate raw parser : %s\n", parser.error());
		exit(1);
	}

	// Skip offset bytes
	if (offset) in.read(nullptr, offset);

	// Numer of records processed
	uint64_t nrecs = 0;

	// Fetch records from the parser and feed them to the formatter
	while (1) {
		// Update offset at the start of the record
		offset = in.count();

		auto* fd = parser.next();
		if (!fd) break;

		fmt.process(out, *fd);
		out.flush();

		nrecs++;
	}

	if (parser.failed()) {
		fmt::fprintf(stderr, "%s : offset %u\n", parser.error(), offset);

		if (min_valid_recs && nrecs >= min_valid_recs)
			return true; // done with input

		if (parser.error().find_first_of("format:") == 0)
			return false; // retry @ different offset
	}

	return true; // done with input
}

static void process(const std::string& infile, unsigned int flags, hogl::format &fmt)
{
	// Allocate the output buffer
	hogl::ostrbuf_stdio out(stdout, 0, output_buf_size);

	size_t offset = 0;
	while (!reprocess(infile, flags, offset, fmt, out)) {
		if (++offset > max_record_size) {
			fmt::fprintf(stderr, "all validation attempts failed\n");
			exit(1);
		}
	}
}

static unsigned int get_version(const char *str)
{
	// RAW format version map
	static const struct {
		const char *str;
		unsigned int ver;
	} vm[] = {
		{ "1.0", raw_parser::V1   },
		{ "1.1", raw_parser::V1_1 },
		{ }
	};

	for (unsigned int i = 0; vm[i].str; i++)
		if (!strcmp(str, vm[i].str)) return vm[i].ver;

	fmt::fprintf(stderr, "Unsupported version: %s\n", str);
	fmt::fprintf(stderr,"Supported versions:");
	for (unsigned int i = 0; vm[i].str; i++) fmt::fprintf(stderr, " %s", vm[i].str);
	fmt::fprintf(stderr, "\n");
	exit(1);
}

// Command line args {
static struct option main_lopts[] = {
	{"help",    0, 0, 'h'},
	{"version", 1, 0, 'v'},
	{"format",  1, 0, 'f'},
	{"plugin",  1, 0, 'p'},
	{"tailf",   0, 0, 't'},
	{"max-record-size", 1, 0, 'M'},
	{"min-valid-recs",  1, 0, 'R'},
	{0, 0, 0, 0}
};

static char main_sopts[] = "hv:f:p:tM:R:";

static char main_help[] =
	"hogl-cook - HOGL raw log processor\n"
	"Usage:\n"
		"\thogl-cook [options] <raw-log.file or - for stdin>\n"
	"Options:\n"
		"\t--help -h                  Display help text\n"
		"\t--version -v <ver>         Force a specific RAW version for compatibility\n"
		"\t--format -f <fmt>          Format of the output log records\n"
		"\t--plugin -p <fmt.so>       Plugin used for formating records\n"
		"\t--tailf -t                 Follow the growth of a log file\n"
		"\t--max-record-size -M <N>   Max size of the RAW record (default: 10MB)\n"
		"\t--min-valid-recs  -R <N>   Min number of valid records (default: 4)\n";
// }

int main(int argc, char *argv[])
{
	char *log_format = 0;
	char *fmt_plugin = 0;
	unsigned int flags = 0;
	int opt;

	// Parse command line args
	while ((opt = getopt_long(argc, argv, main_sopts, main_lopts, NULL)) != -1) {
		switch (opt) {
		case 'v':
			version = get_version(optarg);
			break;

		case 'f':
			log_format = strdup(optarg);
			break;

		case 'p':
			fmt_plugin = strdup(optarg);
			break;

		case 't':
			flags |= hogl::file_rdbuf::TAILF;
			break;

		case 'M':
			max_record_size = strtoul(optarg, 0, 0);
			break;

		case 'R':
			min_valid_recs = strtoul(optarg, 0, 0);
			break;

		case 'h':
		default:
			fmt::printf("%s", main_help);
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fmt::printf("%s", main_help);
		exit(1);
	}

	const char *infile = argv[0];

	// Create format handler
	void *plugin_handle = 0;
	hogl::plugin::format *plugin_ops = 0;
	hogl::format *fmt = 0;

	if (fmt_plugin) {
    		// Load plugin library
    		plugin_handle = dlopen(fmt_plugin, RTLD_LAZY);
    		if (!plugin_handle) {
			fmt::fprintf(stderr, "Failed to load formater plugin: %s\n", dlerror());
			exit(1);
    		}
    		// Reset errors
    		dlerror();
 
    		// Load the symbols
    		plugin_ops = (hogl::plugin::format *) dlsym(plugin_handle, "__hogl_plugin_format");
    		const char* dlsym_error = dlerror();
    		if (dlsym_error) {
			fmt::fprintf(stderr, "Failed to load plugin symbols: %s\n", dlsym_error);
			exit(1);
    		}

		fmt = plugin_ops->create(log_format);
		if (!fmt) {
			fmt::fprintf(stderr, "Failed to initialize format plugin\n");
			exit(1);
		}
	} else
		fmt = new hogl::format_basic(log_format);

	// Read input file
	process(infile, flags, *fmt);

	// Cleanup
	if (fmt_plugin) {
		plugin_ops->release(fmt);
		dlclose(plugin_handle);
	} else
		delete fmt;

	return 0;
}
