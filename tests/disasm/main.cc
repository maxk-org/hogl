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

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/format-basic.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/engine.hpp"
#include "hogl/area.hpp"
#include "hogl/post.hpp"
#include "hogl/c-api/area.h"

__HOGL_PRIV_NS_USING__;

enum test_sect_id {
	TEST_INFO
};

static const char *test_sect_names[] = {
	"INFO",
	0,
};

hogl::area *test_area;
hogl_area_t c_api_area;

extern void init_record(hogl::record *r);
extern void init_post(hogl::record *r);
extern void init_tls();
extern void init_bitmap();
extern "C" void init_c_api_post();

int main(int, char**)
{
	hogl::format_basic  logfmt;
	hogl::output_stderr logout(logfmt);
	hogl::activate(logout);

	test_area = hogl::add_area("TEST-AREA", test_sect_names);
	test_area->set();

	c_api_area = (void *) test_area;

	hogl::record r;

	init_record(&r);
	init_post(&r);
	init_tls();
	init_bitmap();
	init_c_api_post();

	hogl::deactivate();
	return 0;
}
