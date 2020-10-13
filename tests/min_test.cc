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
#include <time.h>

#include "hogl/format-basic.hpp"
#include "hogl/output-stdout.hpp"
#include "hogl/engine.hpp"
#include "hogl/area.hpp"
#include "hogl/post.hpp"
#include "hogl/mask.hpp"
#include "hogl/fmt/printf.h"

__HOGL_PRIV_NS_USING__;

static bool run_tests()
{
	hogl::area* area = hogl::add_area("TEST");

	hogl::post(area, area->ERROR, "error %s", "just kidding");
	hogl::post(area, area->WARN,  "warn %llu", 123456789ULL);
	hogl::post(area, area->INFO,  "info %u",  12345);
	hogl::post(area, area->DEBUG, "debug %f", 1234.5);

	hogl::post(area, area->TRACE, hogl::arg_gstr("cat event %s # ph:X # arg0: %u"), "TEST", 12345);

	return true;
} 

int main(int argc, char *argv[])
{
	// min init to log to stdout
	hogl::format_basic  format("fast1");
	hogl::output_stdout output(format);
	hogl::activate(output);

	// enable DEBUG and TRACE
	hogl::mask mask(".*:.*(INFO|WARN|ERROR|FATAL|DEBUG|TRACE).*", 0);
	hogl::apply_mask(mask);

	bool r = run_tests();

	hogl::deactivate();

	if (r) {
		fmt::printf("Passed\n");
		return 0;
	}

	return 1;
}
