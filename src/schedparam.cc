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

#include <sched.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#include "hogl/detail/internal.hpp"
#include "hogl/platform.hpp"
#include "hogl/post.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// ****
// CPU affinity support

#if defined(__linux__)

// FIXME: add support Linux kernel cpusets

// Convert list format string into cpu mask.
// @param str list string (comma-separated list of decimal numbers and ranges)
// @param mask output mask
// @param nbytes number of bytes in the mask
// returns 0 on success, -1 and errno on invalid input strings.
static int parse_cpulist(const std::string& str, size_t nbytes, cpu_set_t* mask)
{
	const size_t nbits = nbytes * 8;
	const char *p = str.c_str();
	unsigned int a, b;
	do {
		if (!isdigit(*p)) { errno = EINVAL; return -1; }

		b = a = strtoul(p, (char **)&p, 10);
		if (*p == '-') {
			p++;
			if (!isdigit(*p)) { errno = EINVAL; return -1; }
			b = strtoul(p, (char **)&p, 10);
		}
		if (!(a <= b))  { errno = EINVAL; return -1; }
		if (b >= nbits) { errno = ERANGE; return -1; }
		while (a <= b) {
			CPU_SET_S(a, nbytes, mask);
			a++;
		}
		if (*p == ',') p++;
	} while (*p != '\0');
	return 0;
}

static int parse_cpumask(const std::string& str, size_t nbytes, cpu_set_t* mask)
{
	// FIXME: limited to 64 cpus

	errno = 0;
	unsigned long long m = std::strtoull(str.c_str(), 0, 0);
	if (errno)
		return -1;

	for (unsigned int i=0; i < 64; i++)
		if (m & (1<<i)) CPU_SET_S(i, nbytes, mask);
	return 0;
}

static int parse_cpu_affinity(const std::string& cpuset_str, size_t nbytes, cpu_set_t* mask)
{
	int err = 0;
	if (cpuset_str.compare(0, 5, "list:") == 0)
		err = parse_cpulist(cpuset_str.substr(5), nbytes, mask);
	else if (cpuset_str.compare(0, 5, "mask:") == 0)
		err = parse_cpumask(cpuset_str.substr(5), nbytes, mask);
	else
		err = parse_cpumask(cpuset_str, nbytes, mask);
	return err;
}

static int set_cpu_affinity(const std::string& cpuset_str, bool dryrun = false)
{
	int err = 0;

	const size_t ncpus  = 256; // Do we need more?
	const size_t nbytes = CPU_ALLOC_SIZE(ncpus);

	cpu_set_t* cpuset = CPU_ALLOC(ncpus);
	if (!cpuset)
		return -1;

	CPU_ZERO_S(nbytes, cpuset);

	err = parse_cpu_affinity(cpuset_str, nbytes, cpuset);
	if (!err && !dryrun) {
		err = sched_setaffinity(0 /* curr thread */, nbytes, cpuset);
		std::cout << "affinity apply \n";
	}

	CPU_FREE(cpuset);
	return err;
}

#else // !linux

static int set_cpu_affinity(const std::string& cpuset_str, bool dryrun = false)
{
	platform::post_early(internal::WARN, "cpu-affinity not supported on this platform");
	return 0; // just warn but don't fail
}

#endif // if linux

schedparam::schedparam(int _policy, int _priority, const std::string _cpu_affinity) :
	policy(_policy), priority(_priority), cpu_affinity(_cpu_affinity)
{ }

schedparam::schedparam() :
	policy(SCHED_OTHER), priority(0), cpu_affinity()
{ }

bool schedparam::apply(const char *title) const
{
	platform::set_thread_title(title);

	bool failed = false;
	int  err = 0;

	// Apply CPU affinity settings
	if (!this->cpu_affinity.empty()) {
		err = set_cpu_affinity(this->cpu_affinity);
		if (err) {
			platform::post_early(internal::WARN, "set-cpu-affinity failed: %s(%d)", strerror(errno), errno);
			failed = true;
		}
	}

	struct sched_param sp = {0};
        sp.sched_priority = this->priority;

	err = sched_setscheduler(0 /* current pid */, this->policy, &sp);
	if (err < 0) {
		platform::post_early(internal::WARN, "set-scheduler failed: %s(%d)", strerror(errno), errno);
		failed = true;
	}

	return !failed;
}

bool schedparam::validate() const
{
	bool failed = false;
	int  err = 0;

	// Check CPU affinity settings
	if (!this->cpu_affinity.empty()) {
		err = set_cpu_affinity(this->cpu_affinity, true /* dryrun */);
		if (err) {
			platform::post_early(internal::WARN, "invalid cpu-affinity: %s %s", this->cpu_affinity.c_str(), strerror(errno));
			failed = true;
		}
	}

	// FIXME: sanity check policy and priority
	return !failed;
}

std::ostream& operator<< (std::ostream& s, const schedparam& p)
{
	std::ios_base::fmtflags fmt = s.flags();

	s << "" << "{ "
		<< "policy:" << p.policy << ", "
		<< "priority:"  << p.priority << ", "
		<< "cpu-affinity:" << p.cpu_affinity
		<< " }";

	s.flags(fmt);

	return s;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

