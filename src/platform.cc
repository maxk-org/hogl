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

#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <string>

#include "hogl/detail/internal.hpp"
#include "hogl/detail/engine.hpp"
#include "hogl/platform.hpp"
#include "hogl/post.hpp"
#include "hogl/fmt/printf.h"

__HOGL_PRIV_NS_OPEN__
namespace hogl {
namespace platform {

// Special version of post for the low-level init functions.
// Some of these functions may be called before the default
// engine is running. It uses stderr in that case.
template<typename T0, typename T1>
void __post_early(unsigned int section, const char *fmt, T0 arg0, T1 arg1)
{
	if (default_engine) {
		const hogl::area *area = default_engine->internal_area();
		hogl::post(area, section, fmt, arg0, arg1);
	} else {
		// The engine is not running
		const char *sect_name = "";
		switch (section) {
		case internal::INFO:
			sect_name = "info:";
			break;

		case internal::ERROR:
			sect_name = "error:";
			break;

		case internal::WARN:
			sect_name = "warn:";
			break;
		}

		fmt::fprintf(stderr, "%s", sect_name);
		fmt::fprintf(stderr, fmt, arg0, arg1); 
		fmt::fprintf(stderr, "\n");
	}
}

void post_early(unsigned int section, const char *fmt, const char *arg0, int arg1)
{
	__post_early(section, fmt, arg0, arg1);
}
void post_early(unsigned int section, const char *fmt, const char *arg0, const char* arg1)
{
	__post_early(section, fmt, arg0, arg1);
}

/**
 * Enable file backed mappings in the coredump.
 * FIXME: This is Linux specific implementation. Add support for *BSD and others.
 */
bool enable_verbose_coredump()
{
	FILE *f = fopen("/proc/self/coredump_filter", "w");
	if (f) {
		int r = fprintf(f, "0xffff");
		int err = errno;
		fclose(f);
		if (r < 0) {
			post_early(internal::ERROR, "could not enable verbose coredump. %s(%d)",
						strerror(err), err);
			return false;
		}
		return true;
	}

	post_early(internal::WARN, "verbose coredump is not supported");
	return false;
}

#if defined(__linux__) || defined(__QNXNTO__) // OS

void set_thread_title(const char *str)
{
	pthread_setname_np(pthread_self(), str);
}

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

void set_thread_title(const char *str)
{
	setproctitle("%s", str);
}

#else // OS

void set_thread_title(const char *str)
{ }

#endif // OS

} // namespace platform
} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

