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

#include "raw-parser.hpp"
#include "rdbuf.hpp"

unsigned int max_record_size = 10 * 1024 * 1024; // 10MB should be plenty for all cases
unsigned int output_buf_size = 1 * 1024 * 1024;

static void process(const std::string& infile, unsigned int flags, hogl::format &fmt)
{
	// Open input
	hogl::file_rdbuf in(infile, flags);
	if (!in.valid()) {
		fprintf(stderr, "failed to open input file %s, %s(%d)\n",
				infile.c_str(), strerror(in.error()), in.error());
		exit(1);
	}

	// Allocate raw parser
	hogl::raw_parser parser(in, max_record_size);
	if (parser.failed()) {
		fprintf(stderr, "failed to allocate raw parser. %s\n", parser.error().c_str());
		exit(1);
	}

	// Allocate the output buffer
	hogl::ostrbuf_stdio out(stdout, 0, output_buf_size);

	// Fetch records from the parser and feed them to the formatter
	const hogl::format::data *fd;
	while ((fd = parser.next())) {
		fmt.process(out, *fd);
		out.flush();
	}


	if (parser.failed()) {
		fprintf(stderr, "failed to parse input. %s\n", parser.error().c_str());
		exit(1);
	}
}

// Command line args {
static struct option main_lopts[] = {
   {"help",    0, 0, 'h'},
   {"format",  1, 0, 'f'},
   {"plugin",  1, 0, 'p'},
   {"tailf",   0, 0, 't'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:p:t:";

static char main_help[] =
   "hogl-cook - HOGL raw log processor\n"
   "Usage:\n"
      "\thogl-cook [options] <raw-log.file or - for stdin>\n"
   "Options:\n"
      "\t--help -h              Display help text\n"
      "\t--tailf -t             Follow the growth of a log file\n"
      "\t--format -f <fmt>      Format of the output log records\n"
      "\t--plugin -p <fmt.so>   Plugin used for formating records\n";
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
		case 'f':
			log_format = strdup(optarg);
			break;

		case 'p':
			fmt_plugin = strdup(optarg);
			break;

		case 't':
			flags |= hogl::file_rdbuf::TAILF;
			break;

		case 'h':
		default:
			printf("%s", main_help);
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		printf("%s", main_help);
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
			fprintf(stderr, "Failed to load formater plugin: %s\n", dlerror());
			exit(1);
    		}
    		// Reset errors
    		dlerror();
 
    		// Load the symbols
    		plugin_ops = (hogl::plugin::format *) dlsym(plugin_handle, "__hogl_plugin_format");
    		const char* dlsym_error = dlerror();
    		if (dlsym_error) {
			fprintf(stderr, "Failed to load plugin symbols: %s\n", dlsym_error);
			exit(1);
    		}

		fmt = plugin_ops->create(log_format);
		if (!fmt) {
			fprintf(stderr, "Failed to initialize format plugin\n");
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
