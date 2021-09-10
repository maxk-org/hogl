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

#include <sched.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#include "hogl/detail/internal.hpp"
#include "hogl/platform.hpp"
#include "hogl/post.hpp"

#if defined(__QNXNTO__)
#include <sys/neutrino.h>
#include <sys/syspage.h>
#endif

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// ****
// CPU affinity support

#if defined(__linux__)

struct cpumask {
	cpu_set_t* cpuset;
	size_t     ncpus;
	size_t     nbytes;

	cpumask(unsigned int capacity = 256) :
		cpuset(nullptr), ncpus(capacity)
	{
		nbytes = CPU_ALLOC_SIZE(ncpus);
		cpuset = CPU_ALLOC(ncpus);
		if (cpuset)
			CPU_ZERO_S(nbytes, cpuset);
	}

	~cpumask()
	{
		if (cpuset)
			CPU_FREE(cpuset);
	}


	bool valid() const { return cpuset != nullptr; }

	void set(unsigned int i) { CPU_SET_S(i, nbytes, cpuset); }

	int apply()
	{
		return pthread_setaffinity_np(pthread_self(), nbytes, cpuset);
	}
};

#elif defined(__QNXNTO__)

struct cpumask {
	unsigned *runmask;
	size_t    ncpus;

	cpumask(unsigned int capacity = _syspage_ptr->num_cpu) :
		runmask(nullptr), ncpus(capacity)
	{
		int nbytes = RMSK_SIZE(ncpus) * sizeof(unsigned);

		runmask = (unsigned *) malloc(nbytes);
		if (!runmask)
			return;
		memset(runmask, 0, nbytes);
	}

	~cpumask()
	{
		free(runmask);
	}

	bool valid() const { return false; }

	void set(unsigned int i) { RMSK_SET(i, runmask); }

	int apply()
	{
		return ThreadCtl_r(_NTO_TCTL_RUNMASK, runmask);
	}
};

#else

struct cpumask {
	bool valid() const { return false; }

	void set(unsigned int i) { ; }

	int apply()
	{
		platform::post_early(internal::WARN, "cpu-affinity not supported on this platform");
		return EOPNOTSUPP;
	}
};

#endif

// FIXME: add support Linux kernel cpusets

// Convert list format string into cpu mask.
// @param str list string (comma-separated list of decimal numbers and ranges)
// @param mask output mask
// @param nbytes number of bytes in the mask
// returns 0 on success, errno on invalid input strings.
static int parse_cpulist(const std::string& str, cpumask& mask)
{
	const char *p = str.c_str();
	unsigned int a, b;
	do {
		if (!isdigit(*p)) return EINVAL;

		b = a = strtoul(p, (char **)&p, 10);
		if (*p == '-') {
			p++;
			if (!isdigit(*p)) return EINVAL;
			b = strtoul(p, (char **)&p, 10);
		}
		if (!(a <= b))  return EINVAL;
		if (b >= mask.ncpus) return ERANGE;
		while (a <= b) {
			mask.set(a);
			a++;
		}
		if (*p == ',') p++;
	} while (*p != '\0');
	return 0;
}

static int parse_cpumask(const std::string& str, cpumask& mask)
{
	// FIXME: limited to 64 cpus

	errno = 0;
	unsigned long long m = std::strtoull(str.c_str(), 0, 0);
	if (errno)
		return errno;

	for (unsigned int i=0; i < mask.ncpus; i++)
		if (m & (1<<i)) mask.set(i);
	return 0;
}

static int parse_cpu_affinity(const std::string& cpuset_str, cpumask& mask)
{
	int err = 0;
	if (cpuset_str.compare(0, 5, "list:") == 0)
		err = parse_cpulist(cpuset_str.substr(5), mask);
	else if (cpuset_str.compare(0, 5, "mask:") == 0)
		err = parse_cpumask(cpuset_str.substr(5), mask);
	else
		err = parse_cpumask(cpuset_str, mask);
	return err;
}

static int set_cpu_affinity(const std::string& cpuset_str, bool dryrun = false)
{
	int err = 0;

	cpumask mask;
	if (!mask.valid())
		return EINVAL;

	err = parse_cpu_affinity(cpuset_str, mask);
	if (!err && !dryrun)
		err = mask.apply();

	return err;
}

schedparam::schedparam(int _policy, int _priority, unsigned int _flags, const std::string _cpu_affinity) :
	policy(_policy), priority(_priority), flags(_flags), cpu_affinity(_cpu_affinity)
{ }

schedparam::schedparam() :
	policy(SCHED_OTHER), priority(0), flags(0), cpu_affinity()
{ }

bool schedparam::thread_enter(const char * /* title */)
{
	bool failed = false;
	int  err = 0;

	// Apply CPU affinity settings
	if (!this->cpu_affinity.empty()) {
		err = set_cpu_affinity(this->cpu_affinity);
		if (err) {
			platform::post_early(internal::WARN, "set-cpu-affinity failed: %s(%d)", strerror(err), err);
			failed = true;
		}
	}

	struct sched_param sp = {0};
        sp.sched_priority = this->priority;

	err = pthread_setschedparam(pthread_self(), this->policy, &sp);
	if (err) {
		platform::post_early(internal::WARN, "pthread-setschedparam failed: %s(%d)", strerror(err), err);
		failed = true;
	}

	return !failed;
}

void schedparam::thread_exit()
{
	if (flags & DELETE_ON_EXIT)
		delete this;
}

bool schedparam::validate() const
{
	bool failed = false;
	int  err = 0;

	// Check CPU affinity settings
	if (!this->cpu_affinity.empty()) {
		err = set_cpu_affinity(this->cpu_affinity, true /* dryrun */);
		if (err) {
			platform::post_early(internal::WARN, "invalid cpu-affinity: %s %s", this->cpu_affinity.c_str(), strerror(err));
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

