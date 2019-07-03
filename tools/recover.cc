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
#include <dlfcn.h>

#include <iostream>
#include <list>

#include "hogl/detail/ringbuf.hpp"
#include "hogl/detail/magic.hpp"
#include "hogl/plugin/format.hpp"
#include "hogl/format-basic.hpp"

#include "tools/coredump.hpp"
#include "tools/recovery-engine.hpp"

__HOGL_PRIV_NS_USING__;

// Command line args {
static struct option main_lopts[] = {
   {"help",    0, 0, 'h'},
   {"format",  1, 0, 'f'},
   {"plugin",  1, 0, 'p'},
   {"dump-rings",  0, 0, 'R'},
   {"dump-areas",  0, 0, 'A'},
   {0, 0, 0, 0}
};

static char main_sopts[] = "hf:p:RA";

static char main_help[] =
   "hogl-recover - HOGL log recovery from core dumps\n"
   "Usage:\n"
      "\thogl-recover [options] <core.dump.file> [executable.file]\n"
   "Options:\n"
      "\t--help -h              Display help text\n"
      "\t--format -f <fmt>      Format of the log records\n"
      "\t--plugin -p <fmt.so>   Plugin used for formating records\n"
      "\t--dump-rings -R        Dump recovered ring buffers\n"
      "\t--dump-areas -A        Dump recovered areas\n";
// }

int main(int argc, char *argv[])
{
	char *log_format = 0;
	char *fmt_plugin = 0;
	int opt;

	bool dump_rings = false;
	bool dump_areas = false;

	// Parse command line args
	while ((opt = getopt_long(argc, argv, main_sopts, main_lopts, NULL)) != -1) {
		switch (opt) {
		case 'f':
			log_format = strdup(optarg);
			break;

		case 'p':
			fmt_plugin = strdup(optarg);
			break;

		case 'A':
			dump_areas = true;
			break;

		case 'R':
			dump_rings = true;
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

	const char *corefile = argv[0];
	const char *execfile = 0;
	if (argc == 2)
		execfile = argv[1];

	// Load coredump file
	hogl::coredump  core(corefile, execfile);
	if (core.failed) {
		fprintf(stderr, "Failed to load core and executable files\n");
		exit(1);
	}

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

	// Create recovery engine
	hogl::recovery_engine *engine = new hogl::recovery_engine(core, *fmt);

	if (dump_rings)
		engine->dump_rings();

	if (dump_areas)
		engine->dump_areas();

	if (!dump_rings && !dump_areas) {
		engine->dump_outbufs();
		engine->dump_records();
	}

	// Cleanup
	delete engine;

	if (fmt_plugin) {
		plugin_ops->release(fmt);
		dlclose(plugin_handle);
	} else
		delete fmt;

	return 0;
}
