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

#include <stdlib.h>
#include <stdio.h>
#include <boost/format.hpp>

#include "hogl/detail/ringbuf.hpp"

#ifdef HOGL_DEBUG
#define dprint(fmt, args...) fprintf(stderr, "hogl: " fmt "\n", ##args)
#else
#define dprint(a...)
#endif

namespace hogl {

extern timesource default_timesource;

// Make sure ringbufs are always aligned to 64 bytes (most common cacheline size)
void* ringbuf::operator new(size_t s)
{
	void *m = NULL;
	if (posix_memalign(&m, 64, s) < 0)
		throw std::bad_alloc();
	return m;
}

// Deallocate with free() because overloaded new() uses posix_memalign()
void ringbuf::operator delete(void *p)
{
	free(p);
}

// Round up to a power of two and return the exponent
static unsigned int __roundup_log2(unsigned int v)
{
	unsigned int i;
	for (i=0; (1U << i) < v; i++);
	return i;
}

// Round up to a power of two
static unsigned int __roundup_power2(unsigned int size)
{
	return (1 << __roundup_log2(size));
}

/**
 * Allocate ringbuf with specified options
 */
ringbuf::ringbuf(const char *name, const options &opts) :
	_refcnt(0)
{
	int err;

	_magic    = hogl::ring_magic;
	_name     = strdup(name);
	_flags    = opts.flags;
	_seqnum   = 0;
	_dropcnt  = 0;
	_timesource = &default_timesource;

	_prio = opts.prio;
	if (_prio > PRIORITY_CEILING)
		_prio = PRIORITY_CEILING;

	// Compute the desired capacity.
	// This field is used as a mask for advancine head and tail indexes
	// hence the decrement. ie tail = ((tail + 1) & capacity).
	_capacity = __roundup_power2(opts.capacity);
	_capacity--;

	_tail = 0;
	_head = _capacity;

	// Adjust record tail room and compute record size and shift for indexing
	_rec_tailroom = opts.record_tailroom;
	_rec_shift    = __roundup_log2(sizeof(record) + _rec_tailroom);
	_rec_tailroom = record_size() - record::header_size();

	// At this point this->record_size() and this->capacity() functions
	// return correct values. Use them below.

	// Allocate record buffers
	_rec_top = 0;
	err = posix_memalign((void **) &_rec_top, sysconf(_SC_PAGESIZE), capacity() * record_size());
	if (err && !_rec_top) {
		_capacity = 0;
		return;
	}
	memset(_rec_top, 0, capacity() * record_size());

	// Init ringbuf mutex.
	// Enable priority inherintance.
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);

	err = pthread_mutex_init(&_mutex, &mattr);
	if (err) {
		fprintf(stderr, "hogl::ring: failed to init ring mutex. err %u\n", err);
		abort();
	}

	pthread_mutexattr_destroy(&mattr);

	dprint("created ringbuf %p. name %s capacity %u prio %u", this, _name, _capacity, _prio);
}

void ringbuf::reset(void)
{
	lock();
	_tail = 0;
	_head = _capacity;
	_seqnum   = 0;
	_dropcnt  = 0;
	unlock();
}

ringbuf::~ringbuf()
{
	if (_refcnt.get() != 0) {
		fprintf(stderr, "hogl::ring: destroying ringbuf %s(%p) which is still in use.\n",
			_name, this);
		abort();
	}

	if (!empty())
		fprintf(stderr, "hogl::ring: warning: destroying non-empty ringbuf %s(%p)\n", _name, this);

	dprint("destroyed ringbuf %p. name %s (empty %u)", this, _name, empty());

	free(_rec_top);
	free(_name);
}

void ringbuf::timesource(hogl::timesource *ts)
{
	// Make sure the caller finished updated the timesource 
	// struct before we start using it.
	barrier::memw();

	_timesource = ts;
}

ringbuf::options ringbuf::default_options = {
	.capacity = 1024,
	.prio = 0,
	.flags = 0
};

std::ostream& operator<< (std::ostream& s, const ringbuf& ring)
{
	const char *ts_name = "null";
	if (ring.timesource())
		ts_name = ring.timesource()->name();

	s << boost::format("%s: { prio:%u, refcnt:%d, seqnum:%lu, dropcnt:%lu, capacity:%u, size:%u, room:%u, record_size: %u, record_tailroom: %u, timesource: \"%s\" }")
			% ring.name()
			% ring.prio()
			% ring.refcnt()
			% ring.seqnum()
			% ring.dropcnt()
			% ring.capacity()
			% ring.size()
			% ring.room()
			% ring.record_size()
			% ring.record_tailroom()
			% ts_name;
	s << std::endl;
	return s;
}

} // namespace hogl
