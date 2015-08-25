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

/**
 * @file hogl/detail/timestamp.hpp
 * Timestamp helpers
 */
#ifndef HOGL_DETAIL_TIMESTAMP_HPP
#define HOGL_DETAIL_TIMESTAMP_HPP

#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include <hogl/detail/compiler.hpp>

namespace hogl {

class timestamp {
private:
	static const unsigned int nsec_per_sec = 1000000000;

	// We support two timestamp formats native timespec 
	// and plain nanoseconds. When the timespec format is used
	// the union helps to make sure that the 64bit value can be used
	// for sorting and things without any conversions.

	union {
		uint64_t u64;
		struct {
		#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint32_t sec;
			uint32_t nsec;
		#else
			uint32_t nsec;
			uint32_t sec;
		#endif
		} u32;
	} _v;

public:
	// These are the most performance critical and are not
	// affected by the timestamp format. ie Whether we use
	// native timespec or nsec we can always just use the 
	// 64bit value here.
	bool operator<(const timestamp &ts) const
	{
		return _v.u64 < ts._v.u64;
	}

	bool operator>(const timestamp &ts) const
	{
		return _v.u64 > ts._v.u64;
	}

	bool operator==(const timestamp &ts) const
	{
		return _v.u64 == ts._v.u64;
	}

	void operator=(const timestamp &ts)
	{
		_v.u64 = ts._v.u64;
	}

	timestamp(const timestamp &ts)
	{
		_v.u64 = ts._v.u64;
	}

#ifdef HOGL_TIMESTAMP_USE_TIMESPEC_FORMAT
	// This version uses native timespec format to avoid multiplications
	// and divisions when convertion from to timespec at the expense of 
	// doing conversions in operator+() and operator-(). This affects 
	// only a few places in the engine and the format handler.
	void from_timespec(const struct timespec &ts)
	{
		_v.u32.sec  = ts.tv_sec;
		_v.u32.nsec = ts.tv_nsec;
	}
	void to_timespec(struct timespec &ts) const
	{
		ts.tv_sec  = _v.u32.sec;
		ts.tv_nsec = _v.u32.nsec;
	}

	void from_nsec(uint64_t nsec)
	{
		_v.u32.sec  = nsec / nsec_per_sec; 
		_v.u32.nsec = nsec % nsec_per_sec;
	}

	uint64_t to_nsec() const
	{
		return _v.u32.sec * nsec_per_sec + _v.u32.nsec;
	}
#else
	// This version stores timestamp in nanoseconds which means 
	// that we need to do multiplication when the timestamp is first
	// taken (from clock_gettime for example). The rest of the operations
	// just deal with 64bit nanosecond values.

	void from_timespec(const struct timespec &ts)
	{
		_v.u64 = (uint64_t) ts.tv_sec * nsec_per_sec + ts.tv_nsec;
	}
	void to_timespec(struct timespec &ts) const
	{
		ts.tv_sec  = _v.u64 / nsec_per_sec;
		ts.tv_nsec = _v.u64 % nsec_per_sec;
	}

	void from_nsec(uint64_t nsec)
	{
		_v.u64 = nsec;
	}

	uint64_t to_nsec() const
	{
		return _v.u64;
	}
#endif

	void from_usec(uint64_t usec)
	{
		from_nsec(usec * 1000); 
	}

	uint64_t to_usec() const
	{
		return to_nsec() / 1000;
	}

	void from_msec(uint64_t msec)
	{
		from_nsec(msec * 1000 * 1000);
	}

	uint64_t to_msec() const
	{
		return to_nsec() / (1000 * 1000);
	}

	void from_sec(uint64_t sec)
	{
		from_nsec(sec * nsec_per_sec);
	}

	uint64_t to_sec() const
	{
		return to_nsec() / (nsec_per_sec);
	}

	operator uint64_t() const
	{
		return to_nsec();
	}

	timestamp(uint64_t v = 0)
	{
		from_nsec(v);
	}

	timestamp(const struct timespec &ts)
	{
		from_timespec(ts);
	}

	void operator+=(uint64_t a)
	{
		from_nsec(to_nsec() + a);
	}
	void operator+=(int a)
	{
		from_nsec(to_nsec() + a);
	}

#ifdef HOGL_CHECK_TIMESTAMP_WRAP
	#define __hogl_ts_wrap_assert assert
#else
	#define __hogl_ts_wrap_assert(...)
#endif

	timestamp& operator++()
	{
		from_nsec(to_nsec() + 1);
		return *this;
	}
	timestamp& operator--()
	{
		__hogl_ts_wrap_assert(to_nsec() > 1);
		from_nsec(to_nsec() - 1);
		return *this;
	}

	timestamp operator+(uint64_t a) const
	{
		return timestamp(to_nsec() + a);
	}

	timestamp operator-(uint64_t m) const
	{
		__hogl_ts_wrap_assert(to_nsec() > m);
		return timestamp(to_nsec() - m);
	}
	timestamp operator+(int a) const
	{
		return timestamp(to_nsec() + a);
	}

	timestamp operator-(int m) const
	{
		__hogl_ts_wrap_assert(to_nsec() > (unsigned) m);
		return timestamp(to_nsec() - m);
	}

	timestamp operator-(const timestamp& ts) const
	{
		__hogl_ts_wrap_assert(to_nsec() > ts.to_nsec());
		return timestamp(to_nsec() - ts.to_nsec());
	}

	timestamp operator+(const timestamp& ts) const
	{
		return timestamp(to_nsec() + ts.to_nsec());
	}
};

} // namespace hogl

#endif // HOGL_DETAIL_TIMESTAMP_HPP
