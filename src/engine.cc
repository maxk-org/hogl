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
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>

#include <string>
#include <map>
#include <algorithm>

#include "hogl/detail/internal.hpp"
#include "hogl/detail/utils.hpp"
#include "hogl/detail/engine.hpp"
#include "hogl/detail/barrier.hpp"
#include "hogl/platform.hpp"
#include "hogl/post.hpp"

#ifdef HOGL_DEBUG
#define dprint(fmt, args...) fprintf(stderr, "hogl: " fmt "\n", ##args)
#else
#define dprint(a...)
#endif

namespace hogl {

engine::options engine::default_options = {
	.default_mask = hogl::mask(".*:(INFO|WARN|ERROR|FATAL).*", 0), // enable default sections in all areas
	.polling_interval_usec = 10000,           // polling interval usec
	.tso_buffer_capacity =   4096,            // tso buffer size (number of records)
	.internal_ring_capacity = 256,            // capacity of the internal ring buffer (number of records)
	.features = 0,                            // default feature set
	.cpu_affinity_mask = 0,                                // default CPU affinity
	.timesource = 0,                          // timesource for this engine (0 means default timesource)
};

/**
 * Add internal area.
 * Called from constructor.
 */
void engine::add_internal_area()
{
	const char *name = "HOGL";

	// Allocate and register internal area.
	// Note that we cannot use add_area(...) function here because it
	// logs things in the caller's context and we're not ready to process 
	// log records yet.

	_internal_area = new area(name, internal::section_names);

	_internal_area->enable(internal::INFO);
	_internal_area->enable(internal::WARN);
	_internal_area->enable(internal::ERROR);
	_internal_area->enable(internal::DROPMARK);
	_internal_area->enable(internal::TSOFULLMARK);

	_area_map.insert(area_map::value_type(name, _internal_area));
	_opts.default_mask.apply(_internal_area);
}

extern timesource default_timesource;

engine::engine(output &out, const engine::options &opts) :
	_magic(hogl::engine_magic),
	_running(false),
	_killed(false),
	_output(out),
	_opts(opts)
{
	int err;

	// Do we need prio inheritance on these?
	pthread_mutex_init(&_area_mutex, NULL);
	pthread_mutex_init(&_ring_mutex, NULL);

	_ring_index.dirty = true;
	_ring_index.count = 0;
	_ring_index.entries = 0;

	_timesource = _opts.timesource;
	if (!_timesource)
		_timesource = &default_timesource;

	if (!(_opts.features & DISABLE_TSO)) {
		unsigned int s = _opts.tso_buffer_capacity ? _opts.tso_buffer_capacity : 2048;
		_tso.resize(s);
		_tso.reset();
		_tso_leftover = 0;
	}

	memset(&_stats, 0, sizeof(_stats));

	add_internal_area();

	dprint("created engine %p", this);

	// Start engine thread
	err = pthread_create(&_thread, NULL, entry, (void *) this);
	if (err) {
		fprintf(stderr, "hogl::engine: failed to create engine thread. %d\n", err);
		abort();
	}
	err = setaffinity(_thread, _opts.cpu_affinity_mask);
	if (err) {
		fprintf(stderr, "hogl::engine: failed to set affinity for engine thread. %d\n", err);
		abort();
	}
}

engine::~engine()
{
	_killed = true;
	pthread_join(_thread, NULL);

	// Release all registered rings
	for (ring_map::iterator it = _ring_map.begin(); it != _ring_map.end(); ++it) {
		ringbuf *r = it->second;
		r->release();
	}

	// Release all registered areas
	for (area_map::iterator it = _area_map.begin(); it != _area_map.end(); ++it) {
		area *a = it->second;
		delete a;
	}

	delete [] _ring_index.entries;

	dprint("destroyed engine %p", this);
}

void *engine::entry(void *_self)
{
	engine *self = (engine *) _self;

	platform::set_thread_title("hogl::engine");

	// Run the loop
	self->loop();
	return 0;
}

// For whatever reason this cannot be inside of the rebuild_ring_index
// function below. GCC barfs on the std::sort() call in that case.
struct _ring_sorter {
	bool operator() (const engine::ring_index::entry &i, const engine::ring_index::entry &j) 
	{
		return i.pit.const_ring()->prio() > j.pit.const_ring()->prio();
	}
} ring_sorter;

// Look up the ring in the index and get the seqnum 
engine::ring_index::entry* engine::ring_index::operator() (ringbuf *ring)
{
	for (unsigned int i=0; i < this->count; i++) {
		if (this->entries[i].pit.ring() == ring)
			return &this->entries[i];
	}
	return 0;
}

// Ring index rebuild logic.
// Called under ring_mutex.
void engine::rebuild_ring_index()
{
	unsigned int i;

	_stats.rings_indexed++;

	ring_index oi = _ring_index;
	ring_index ni;

	ni.count = _ring_map.size();
	ni.dirty = false;

	dprint("engine rebuilding ring index: ocount %d ncount %u", oi.count, ni.count);

	ni.entries = new ring_index::entry [ni.count];

	// Populate index with ring pointers
	ring_map::const_iterator it;
	for (it = _ring_map.begin(), i = 0; it != _ring_map.end(); ++it, ++i) {
		ringbuf *ring = it->second;
		ni(i)->pit.ring(ring);
		ni(i)->lastrec = 0;
		ni(i)->seqnum    = 0;

		ring_index::entry *re = oi(ring);
		if (re) {
			ni(i)->seqnum = re->seqnum;
		}
	}

	// Sort it by priority
	std::sort(ni.entries, ni.entries + ni.count, ring_sorter);

	// Dump new ring index
	for (i=0; i < ni.count; i++) {
		dprint("ring index #%u: name [%s] prio %u seqnum %llu",
			i, ni(i)->pit.ring()->name(), ni(i)->pit.ring()->prio(), ni(i)->seqnum);
	}

	// Replace current index and delete the old one
	_ring_index = ni;
	delete [] oi.entries;
}

// Switch to the new timesource.
void engine::switch_timesource(const ringbuf *ring, record *r)
{
	_stats.timesource_changed++;

	timesource *ts = (timesource *) r->argval[1].u64;

	inject_record(ring->name(), r->timestamp, r->seqnum, internal::INFO,
			"switching timesource from %s to %s", _timesource->name(), ts->name());

	pthread_mutex_lock(&_ring_mutex);

	// FIXME: Ideally we need to at least try to flush the rings before switching
	// the timesource to avoid confusing TSO but I don't have a good solution for
	// that at this point.
	_timesource = ts;

	// Iterate all rings and update their timesource pointers.
	ring_map::const_iterator it;
	for (it = _ring_map.begin(); it != _ring_map.end(); ++it) {
		ringbuf *ring = it->second;
		ring->timesource(_timesource);
	}

	pthread_mutex_unlock(&_ring_mutex);
}

void engine::kill_orphan(unsigned int i, ringbuf *ring)
{
	// This is not critical. We don't want to stall the engine 
	// thread just to cleanup an orphan.
	if (!pthread_mutex_trylock(&_ring_mutex)) {
		_ring_map.erase(ring->name());

		// Invalidate index. It will be rebuilt next time we
		// enter this loop.
		_ring_index(i)->pit.invalidate();
		_ring_index.dirty = true;

		pthread_mutex_unlock(&_ring_mutex);

		ring->release();
	}
}

void engine::flush_record(unsigned int i, record *r)
{
	const ringbuf *ring = _ring_index(i)->pit.ring();

	if (r->special()) {
		// Process special record
		switch (r->argtype) {
		case internal::SPR_TIMESOURCE_CHANGE:
			switch_timesource(ring, r);
			break;

		case internal::SPR_FLUSH:
		default:
			break;
		};

		r->ack();
	} else {
		// Feed regular record to the output
		format::data d = { 0 };
		d.ring_name = ring->name();
		d.record    = r;
		_output.process(d);
	}

	_stats.recs_out++;
}

// Inject a fake record directly into the output (uint64_t args).
// Warning: only static strings are allowed 
void engine::inject_record(const char *ring_name, timestamp ts, uint64_t seqnum, unsigned int sect, const char *fmt, 
	uint64_t arg0, uint64_t arg1)
{
	record fake;
	fake.area    = internal_area();
	fake.section = sect;
	fake.timestamp = ts;
	fake.seqnum    = seqnum;
	fake.set_args(0, hogl::arg_gstr(fmt), arg0, arg1);

	format::data d = { 0 };
	d.ring_name = ring_name;
	d.record    = &fake;
	_output.process(d);
}

// Inject a fake record directly into the output (const char* args).
// Warning: only static strings are allowed 
void engine::inject_record(const char *ring_name, timestamp ts, uint64_t seqnum, unsigned int sect, const char *fmt, 
		const char *arg0, const char *arg1)
{
	record fake;
	fake.area    = internal_area();
	fake.section = sect;
	fake.timestamp = ts;
	fake.seqnum    = seqnum;
	fake.set_args(0, hogl::arg_gstr(fmt), hogl::arg_gstr(arg0), hogl::arg_gstr(arg1));

	format::data d = { 0 };
	d.ring_name = ring_name;
	d.record    = &fake;
	_output.process(d);
}

void engine::do_flush_tso(unsigned int size)
{
	dprint("tso-flush: size %u (total %u)", size, _tso.size());

	_tso.sort();

	tsobuf::entry te;
	for (unsigned int j=0; j < size && _tso.pop(te); j++) {
		unsigned int i = te.tag;
		record *r = te.rec;

		const ringbuf *ring = _ring_index(i)->pit.ring();

		// Check for dropped records by comparing sequence numbers
		if (r->seqnum != _ring_index(i)->seqnum) {
			// Detected a drop
			uint64_t n = r->seqnum - _ring_index(i)->seqnum;

			if (_internal_area->test(internal::DROPMARK))
				inject_record(ring->name(), r->timestamp - 1, 0, internal::DROPMARK, "dropped %llu record(s)", n);

			_stats.recs_dropped += n;
		}

		// Update expected seqnum and last processed record for this ring
		_ring_index(i)->lastrec = r;
		_ring_index(i)->seqnum  = r->seqnum + 1;

		flush_record(i, r);
	}
}

// Emergency TSO flush.
// Called when TSO gets full and the engine must flush it.
// We only flush half of it to avoid flushing records out-of-order.
void engine::flush_full_tso()
{
	unsigned int size  = _tso.size() / 2;

	if (_internal_area->test(internal::TSOFULLMARK)) {
		tsobuf::entry te = { 0 };
		_tso.top(te);
		inject_record("ENGINE", te.rec->timestamp - 1, 0, internal::TSOFULLMARK, "following records may be out of order");
	}

	do_flush_tso(size);

	_tso_leftover = 0;
	_tso.flip();

	_stats.tso_full++;
}

// Normal TSO flush.
// Called at the end of the poll iteration.
// Leaves 1/8 of the TSO buffer capacity to avoid flushing records out-of-order.
void engine::flush_tso()
{
	unsigned int size = _tso.size();
	unsigned int keep = _tso.capacity() / 8;

	size = (size > keep) ? (size - keep) : 0;
	if (size < _tso_leftover)
		size = _tso_leftover;
	if (size)
		do_flush_tso(size);

	_tso_leftover = _tso.size();
	_tso.reset();
}

// Process all rings with timestamp ordering
void engine::process_rings_tso()
{
	// Iterate and process all rings.
	// Records are pushed into the TSO buffer for sorting later.
	// TSO buffer entries are tagged with the ring index number.
	unsigned int i;
	for (i = 0; i < _ring_index.count; i++) {
		ringbuf::pop_iterator &it = _ring_index(i)->pit;
		if (hogl_unlikely(!it.valid()))
			continue;

		tsobuf::entry te;
		te.tag = i;

		dprint("engine processing: ring %p prio %u", it.ring(), it.ring()->prio());

		hogl::timestamp tso_ts = 0;

		it.reset();
		while (1) {
			te.rec = it.next();
			if (!te.rec)
				break;

			// Make sure that timestamps from the same ring never go backwards.
			// At this point we must flush records in order. So this is more of 
			// hack for systems with broken timesources. It never kicks in if 
			// timestamps are monotonically increasing.
			te.timestamp = te.rec->timestamp;
			if (hogl_unlikely(te.timestamp <= tso_ts))
				te.timestamp = tso_ts + 1;
			tso_ts = te.timestamp;

			dprint("engine processing: record %p seq %lu", te.rec, te.rec->seqnum);
			_tso.push(te);
			if (_tso.full())
				flush_full_tso();
		}

		// Check for orphans and kill them.
		// Only empty rings are killed.
		if (hogl_unlikely(it.ring()->orphan() && it.ring()->empty()))
			kill_orphan(i, it.ring());
	}

	flush_tso();

	// Read membarrier makes sure all reads from the rings are done
	barrier::memr();

	// Commit all changes
	for (i = 0; i < _ring_index.count; i++) {
		if (!_ring_index(i)->lastrec)
			continue;

		ringbuf::pop_iterator &it = _ring_index(i)->pit;
		if (hogl_unlikely(!it.valid()))
			continue;
		it.rewind(_ring_index(i)->lastrec);
		it.commit(ringbuf::NOBARRIER);
		_ring_index(i)->lastrec = 0;
	}
}

// Process all rings without timestamp ordering
void engine::process_rings_notso()
{
	// Iterate and process all rings
	unsigned int i;
	for (i = 0; i < _ring_index.count; i++) {
		ringbuf::pop_iterator &it = _ring_index(i)->pit;
		if (hogl_unlikely(!it.valid()))
			continue;

		dprint("engine processing: ring %p prio %u", it.ring(), it.ring()->prio());
		it.reset();
		record *r;
		while ((r = it.next()))
			flush_record(i, r);
		it.commit();

		// Check for orphans and kill them.
		// Only empty rings are killed.
		if (hogl_unlikely(it.ring()->orphan() && it.ring()->empty()))
			kill_orphan(i, it.ring());
	}
}

// Process all rings.
void engine::process_rings()
{
	_stats.loops++;

	// Check and rebuild the index if needed
	if (hogl_unlikely(_ring_index.dirty)) {
		// Index rebuild is not critical. We don't want
		// to stall the engine thread just because something
		// is hogging the lock.
		if (!pthread_mutex_trylock(&_ring_mutex)) {
			rebuild_ring_index();
			pthread_mutex_unlock(&_ring_mutex);
		}
	}

	// Iterate and process all rings
	if (_opts.features & DISABLE_TSO)
		process_rings_notso();
	else
		process_rings_tso();

	// Flush output buffers
	_output.flush();
}

void engine::drain_rings()
{
	dprint("draining all rings");

	while (1) {
		process_rings();

		// We can exit once we have no rings in the index
		if (!_ring_index.dirty && !_ring_index.count)
			break;

		usleep(_opts.polling_interval_usec);
	}
}

static uint64_t gtod_usec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

void engine::loop()
{
	_running = true;

	dprint("engine thread running");

	uint64_t start, elapsed, ts;

	start = gtod_usec();
	while (!_killed) {
		process_rings();
		ts = gtod_usec();
		elapsed = ts - start;
		if (elapsed < _opts.polling_interval_usec)
			usleep(_opts.polling_interval_usec - elapsed);
		start = ts;
	}

	drain_rings();

	dprint("engine thread stopped");

	_running = false;
}

/**
 * Find log area by name
 */
const area *engine::find_area(const char *name) const
{
	area *a = 0;

	pthread_mutex_lock(&_area_mutex);
	area_map::const_iterator it = _area_map.find(name);
	if (it != _area_map.end())
		a = it->second;
	pthread_mutex_unlock(&_area_mutex);
	return a;
}

/**
 * Add new log area.
 * This function must be called outside of the engine context.
 * It posts messages using the ring of the caller thread.
 */
area *engine::add_area(const char *name, const char **sections)
{
	std::pair<area_map::iterator, bool> s;
	area *a, *na = new area(name, sections);

	pthread_mutex_lock(&_area_mutex);
	s = _area_map.insert(area_map::value_type(name, na));
	a = s.first->second;
	_stats.areas_added++;
	pthread_mutex_unlock(&_area_mutex);

	if (s.second != false) {
		// New area.
		// Apply default mask before returning the area to the caller.
		_opts.default_mask.apply(a);

		hogl::post(internal_area(), internal::AREA_DEBUG,
			"new area %s(%p): number-of-sections %u", a->name(), a, a->count());

		return a;
	}

	// Area with this name already exists.
	// Let's see if it can be reused.
	bool reuse = (*na == *a);
	delete na;

	if (reuse)
		return a;

	hogl::post(internal_area(), internal::ERROR,
			"failed to add area %s. already exists and is not reusable.", name);
	return 0;
}

/**
 * Add new ring buffer.
 * This function must always be called outside of the engine context. 
 * It posts messages using ring buffer of the caller thread.
 */
ringbuf *engine::add_ring(const char *name, const ringbuf::options &opts)
{
	std::pair<ring_map::iterator, bool> s;

	ringbuf *nr = new ringbuf(name, opts);
	nr->hold();
	nr->timesource(_timesource);

	pthread_mutex_lock(&_ring_mutex);
	s = _ring_map.insert(ring_map::value_type(name, nr));
	s.first->second->hold();
	pthread_mutex_unlock(&_ring_mutex);

	if (s.second == true) {
		// Ring added. Invalidate the index.
		_ring_index.dirty = true;

		hogl::post(internal_area(), internal::RING_DEBUG,
			"new ring %s(%p): prio %u capacity %u record-size %u",
				nr->name(), nr, nr->prio(), nr->capacity(), nr->record_size());
		
		return nr;
	}

	// Ring already exists. Drop the one we allocated 
	// and see if we can reuse the one we found.
	nr->release();

	ringbuf *r = s.first->second;

	// Shared rings can be reused of course.
	if (r->shared())
		return r;

	// Reusable rings can be reused only if the original
	// owner is dead.
	// refcnt == 2 here means one reference is local (we did
	// the hold() when we found the ring) and one reference 
	// belongs to this engine.
	if (r->reusable() && r->refcnt() == 2)
		return r;

	hogl::post(internal_area(), internal::ERROR,
		"failed to add ring %s. already exists and is not reusable.", r->name());

	// Cannot reuse it. Original owner is probably still alive.
	// Drop the local reference.
	r->release();
	return 0;
}

/**
 * Add new ring buffer
 */
bool engine::add_ring(ringbuf *r)
{
	std::pair<ring_map::iterator, bool> s;

	pthread_mutex_lock(&_ring_mutex);
	s = _ring_map.insert(ring_map::value_type(r->name(), r));
	if (s.second == true) {
		_ring_index.dirty = true;
		r->hold();
		r->timesource(_timesource);
	}
	pthread_mutex_unlock(&_ring_mutex);

	if (!s.second) {
		hogl::post(internal_area(), internal::ERROR,
			"failed to add ring %s. already exists.", r->name());
	}

	return s.second;
}

/**
 * Find ring buffer by name
 */
ringbuf *engine::find_ring(const char *name) const
{
	ringbuf *r = 0;

	pthread_mutex_lock(&_ring_mutex);
	ring_map::const_iterator it = _ring_map.find(name);
	if (it != _ring_map.end()) {
		r = it->second;
		r = r->hold();
	}
	pthread_mutex_unlock(&_ring_mutex);
	return r;
}

/**
 * List all ring buffers
 */
void engine::list_rings(string_list &l) const
{
	pthread_mutex_lock(&_ring_mutex);

	ring_map::const_iterator it;
	for (it = _ring_map.begin(); it != _ring_map.end(); ++it)
		l.push_back(it->second->name());

	pthread_mutex_unlock(&_ring_mutex);
}

void engine::apply_mask(const mask &m)
{
	pthread_mutex_lock(&_area_mutex);

	area_map::const_iterator it;
	for (it = _area_map.begin(); it != _area_map.end(); ++it)
		m.apply(it->second);

	_stats.mask_changed++;
	pthread_mutex_unlock(&_area_mutex);
}

/**
 * List all ring buffers
 */
void engine::list_areas(string_list &l) const
{
	pthread_mutex_lock(&_area_mutex);

	area_map::const_iterator it;
	for (it = _area_map.begin(); it != _area_map.end(); ++it)
		l.push_back(it->second->name());

	pthread_mutex_unlock(&_area_mutex);
}

std::ostream& operator<< (std::ostream& s, const engine::stats& stats)
{
	s << "" << "{ "
		<< "tso_full:"           << stats.tso_full           << ", "
		<< "recs_out:"           << stats.recs_out           << ", "
		<< "recs_dropped:"       << stats.recs_dropped       << ", "
		<< "loops:"              << stats.loops              << ", "
		<< "rings_indexed:"      << stats.rings_indexed      << ", "
		<< "areas_added:"        << stats.areas_added        << ", "
		<< "mask_changed:"       << stats.mask_changed       << ", "
		<< "timesource_changed:" << stats.timesource_changed << ", "
		<< " }"	<< std::endl;
	return s;
}

std::ostream& operator<< (std::ostream& s, const engine::options& opts)
{
	const char *ts_name = "null";
	if (opts.timesource)
		ts_name = opts.timesource->name();

	std::ios_base::fmtflags fmt = s.flags();

	s << "" << "{ "
		<< "polling_interval_usec:" << opts.polling_interval_usec << ", "
		<< "tso_buffer_capacity:"   << opts.tso_buffer_capacity << ", "
		<< "tso_buffer_capacity:"   << opts.tso_buffer_capacity << ", "
		<< "features:" << std::hex  << opts.features << ", "
		<< "timesource:"            << ts_name
		<< " }"	<< std::endl;

	s.flags(fmt);

	return s;
}

// Dump engine info into a stream
std::ostream& operator<< (std::ostream& s, const engine& engine)
{
	s << "Options: " << std::endl;
	s.width(4);
	s << engine.get_options();

	s << "Default mask: " << std::endl;
	s.width(4);
	s << engine.get_options().default_mask;

	s << "Stats: " << std::endl;
	s.width(4);
	s << engine.get_stats();

	hogl::string_list alist, rlist;
	engine.list_areas(alist);
	engine.list_rings(rlist);

	hogl::string_list::const_iterator it;

	s << "Rings:" << std::endl;
	for (it = rlist.begin(); it != rlist.end(); ++it) {
		const hogl::ringbuf *ring = engine.find_ring(it->c_str());
		if (ring) {
			s.width(4);
			s << *ring;
			ring->release();
		}
	}

	s << "Areas:" << std::endl;
	for (it = alist.begin(); it != alist.end(); ++it) {
		const hogl::area *area = engine.find_area(it->c_str());
		if (area) {
			s.width(4);
			s << *area;
		}
	}

	return s;
}

// Default engine instance
engine *default_engine;

// Default ringbuf
ringbuf default_ring("DEFAULT", default_ring_options);

void activate(output &out, const engine::options &engine_opts)
{
	assert(default_engine == 0);

	// Default ringbuf must be shared and immortal.
	// We check here because user app is allowed to replace
	// default options.
	assert(default_ring.immortal() && default_ring.shared());

	// Initialize default engine
	engine *e = new engine(out, engine_opts);

	// Indicate that default ring is in use
	default_ring.hold();

	// Register default shared ringbuf
	e->add_ring(&default_ring);

	default_engine = e;

	// Call deactivate on exit
	atexit(hogl::deactivate);
}

void deactivate()
{
	if (!default_engine)
		return;

	// Release the default ring.
	// So that it becomes an orphan and the engine could
	// drop it from its list.
	default_ring.release();

	delete default_engine;
	default_engine = 0;
}

} // namespace hogl
