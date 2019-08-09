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
#include <time.h>
#include <pthread.h>
#include <stdlib.h>

#include "hogl/c-api/area.h"
#include "hogl/c-api/mask.h"
#include "hogl/c-api/post.h"
#include "hogl/c-api/format.h"
#include "hogl/c-api/output.h"
#include "hogl/c-api/engine.h"
#include "hogl/c-api/tls.h"

enum capi_sect_ids {
	CAPI_DEBUG,
	CAPI_INFO,
	CAPI_WARN,
	CAPI_ERROR
};

const char *capi_sect_names[] = {
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	0
};

static hogl_area_t area;

// Testing simple macro wrapper
#define dbglog(level, str, ...) hogl_post(area, CAPI_##level, str, __VA_ARGS__)

static __attribute__((noinline)) void do_post()
{
	hogl_post(area, CAPI_DEBUG, 0xfffffffff, 0xfefefefefefe);
}

void *run_thread(void *unused)
{
	struct hogl_tls_options topts = { .ring_capacity = 1024, .ring_priority = 100 };
	hogl_tls_t tls = hogl_new_tls("THREAD", &topts);

	const char *str = "str";
	char lstr[] = "lstr";

	uint8_t buffer[128] = { 0,1,2,3,4,5,6 };
	unsigned int x = 2;

	hogl_post(area, CAPI_DEBUG, "test");
	hogl_post(area, CAPI_DEBUG, "arg0", "arg1");
	hogl_post(area, CAPI_DEBUG, 1);
	hogl_post(area, CAPI_DEBUG, -10);
	hogl_post(area, CAPI_DEBUG, 7.77);
	hogl_post(area, CAPI_DEBUG, "%f %f %f %f", 22 + 1.1, 23 + 1.2, 24 + 1.3, 25 + 1.4);
	hogl_post(area, CAPI_DEBUG, str);
	hogl_post(area, CAPI_DEBUG, "format string %u %d", 23, -75);
	hogl_post(area, CAPI_DEBUG, "string %s", hogl_string(lstr));
	hogl_post(area, CAPI_DEBUG, "buffer addr %p", hogl_pointer(buffer));
	hogl_post(area, CAPI_DEBUG, "buffer dump %x %x %x %x %x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[x]);
	hogl_post(area, CAPI_DEBUG, "pointer %p", tls);
	hogl_post(area, CAPI_DEBUG, "function_name %s", hogl_string(__func__));

	dbglog(DEBUG, "debug message: %u %f %d", 100, 25.33, -21); 
	dbglog(INFO,  "info message: %u %f %d", 100, 25.33, -21); 
	dbglog(ERROR, "error message: %u %f %d", 100, 25.33, -21); 

	hogl_delete_tls(tls);

	return NULL;
}

enum output_type {
	STDOUT_OUTPUT_TYPE,
	FILE_OUTPUT_TYPE
};

hogl_output_t get_output_file(hogl_format_t fmt, int output) {
	static const char filename[] = "basic-c-api-test.log";
	hogl_output_t out;

	switch (output)
	{
		default:
			printf("unknown output type\n");
			abort();
		case STDOUT_OUTPUT_TYPE:
			out = hogl_new_output_stdout(fmt);
			break;
		case FILE_OUTPUT_TYPE:
			out = hogl_new_output_file(filename, fmt, 0);
			break;
	}
	return out;
}

void setup_and_run(int output) {
	pthread_t thread;

	hogl_format_t fmt = hogl_new_format_basic("timestamp|timedelta|ring|seqnum|area|section");
	hogl_output_t out = get_output_file(fmt, output);

	hogl_mask_t nodbg_mask = hogl_new_mask(".*:.*", "!.*:DEBUG", 0);
	hogl_mask_t allon_mask = hogl_new_mask(".*:.*", 0);

	struct hogl_engine_options opts = { 0 };
	opts.polling_interval_usec = 2000;
	opts.default_mask = nodbg_mask;
	hogl_activate(out, &opts);

	area = hogl_add_area("CAPI", capi_sect_names);

	hogl_apply_mask(allon_mask);

	if (pthread_create(&thread, 0, run_thread, 0) < 0) {
		perror("Failed to create test thread");
		exit(1);
	}

	const char *str = "str";

	hogl_post(area, CAPI_DEBUG, "test");
	hogl_post(area, CAPI_DEBUG, "arg0", "arg1");
	hogl_post(area, CAPI_DEBUG, 1);
	hogl_post(area, CAPI_DEBUG, -10);
	hogl_post(area, CAPI_DEBUG, 7.77);
	hogl_post(area, CAPI_DEBUG, "%f %f %f %f", 22 + 1.1, 23 + 1.2, 24 + 1.3, 25 + 1.4);
	hogl_post(area, CAPI_DEBUG, str);
	hogl_post(area, CAPI_DEBUG, "format string %u %d", 23, -75);
	hogl_post(area, CAPI_DEBUG, "pointer %p", out);
	hogl_post(area, CAPI_DEBUG, "16args %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
				1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

	enum {
		XYZ_0 = 12345
	};
	hogl_post(area, CAPI_DEBUG, "enum %u", XYZ_0);

	dbglog(DEBUG, "debug message: %u %f %d", 100, 25.33, -21); 
	dbglog(INFO,  "info message: %u %f %d", 100, 25.33, -21); 
	dbglog(ERROR, "error message: %u %f %d", 100, 25.33, -21); 

	unsigned int i;
	for (i=0; i<4; i++)
		hogl_post(area, CAPI_DEBUG, "i=%u", i);

	do_post();

	pthread_join(thread, NULL);

	hogl_deactivate();
	hogl_delete_output(out);
	hogl_delete_format(fmt);

	hogl_delete_mask(nodbg_mask);
	hogl_delete_mask(allon_mask);
}

int main(void)
{
	setup_and_run(STDOUT_OUTPUT_TYPE);
	setup_and_run(FILE_OUTPUT_TYPE);
	return 0;
}
