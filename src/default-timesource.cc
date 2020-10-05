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

#include <time.h>

#if defined(__QNXNTO__)
#include <sys/neutrino.h>
#include <sys/syspage.h>
#endif

#include "hogl/detail/timesource.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

static hogl::timestamp clock_realtime(const hogl::timesource *)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return hogl::timestamp(ts);
}

timesource realtime_timesource("clock_realtime", clock_realtime);

#if !defined(__QNXNTO__)
static hogl::timestamp clock_monotonic(const hogl::timesource *)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return hogl::timestamp(ts);
}
#else
static hogl::timestamp clock_monotonic(const hogl::timesource *)
{
	static uint64_t cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	static uint64_t nspc = 1000000000ULL / cps;
	uint64_t c = ClockCycles();
	return hogl::timestamp( timespec{ (long int)(c / cps), (long int)((c % cps) * nspc) });
}
#endif

timesource monotonic_timesource("clock_monotonic", clock_monotonic);

/**
 * Default timesource instance
 */
timesource hogl_weak_symbol default_timesource("clock_realtime", clock_realtime);

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__
