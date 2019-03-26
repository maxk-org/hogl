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
 * @file hogl/detail/engine.h
 * Engine implementation details
 */
#ifndef HOGL_DETAIL_ENGINE_HPP
#define HOGL_DETAIL_ENGINE_HPP

#include <string>
#include <map>

#include <hogl/detail/types.hpp>
#include <hogl/detail/magic.hpp>
#include <hogl/detail/ringbuf.hpp>
#include <hogl/detail/tsobuf.hpp>
#include <hogl/detail/format.hpp>
#include <hogl/detail/output.hpp>
#include <hogl/detail/area.hpp>
#include <hogl/detail/mask.hpp>
#include <hogl/detail/timesource.hpp>

namespace hogl {

/**
 * Logging engine
 */
class engine {
public:
	/**
	 * Engine feature flags
	 */ 	
	enum features {
		DISABLE_TSO = (1<<0),
	};

	/**
	 * Engine options.
	 */
	struct options {
		mask         default_mask;
		unsigned int polling_interval_usec;
		unsigned int tso_buffer_capacity;
		unsigned int internal_ring_capacity; // obsolete
		unsigned int features;
		uint64_t     cpu_affinity_mask; // CPU affinity
		hogl::timesource  *timesource;
	};

	static options default_options;

	/**
	 * Engine stats.
	 */
	struct stats {
		unsigned long tso_full;      // Number of times TSO buffer was full
		unsigned long recs_out;      // Number of records sent to the output
		unsigned long recs_dropped;  // Number of records dropped
		unsigned long loops;         // Number of loops the engine went through
		unsigned long rings_indexed; // Number of times ring index was rebuilt
		unsigned long areas_added;   // Number of times ring index was rebuilt
		unsigned long mask_changed;  // Number of times a mask was applied globally
		unsigned long timesource_changed;  // Number of times the timesource was changed
	};

	// Ring index structure
	struct ring_index {
		struct entry {
			ringbuf::pop_iterator pit;
			record   *lastrec;
			uint64_t  seqnum;
		};

		entry         *entries;
		unsigned int   count;
		volatile bool  dirty;

		entry* operator() (unsigned int i) { return &entries[i]; }
		entry* operator() (ringbuf *ring);
	};

private:
	typedef std::map<std::string, area*>    area_map;
	typedef std::map<std::string, ringbuf*> ring_map;
	typedef pthread_mutex_t                 mutex;

	/**
	 * Magic number
	 */
	magic           _magic;

	/**
	 * Container for areas
	 */
	area_map        _area_map;
	mutable mutex   _area_mutex;

	/**
	 * Container for rings
	 */
	ring_map        _ring_map;
	ring_index      _ring_index;
	mutable mutex   _ring_mutex;

	/**
	 * Timestamp ordering buffer
	 */
	tsobuf          _tso;
	unsigned int    _tso_leftover;

	stats           _stats;

	pthread_t       _thread;
	volatile bool   _running;
	volatile bool   _killed;

	/** Log output */
	output         &_output;

	/** Engine options */
	const options   _opts;

	/**
 	 * Area for internal messages
 	 */
	area           *_internal_area;

	/**
	 * Pointer to the current timesource
	 */
	timesource     *_timesource;

	// No copies
	engine(const engine&);
	engine& operator=( const engine& );

	static void *entry(void *_self);
	void loop();
	void process_rings();
	void process_rings_notso();
	void process_rings_tso();
	void flush_record(unsigned int i, record *rec);
	void flush_tso();
	void flush_full_tso();
	void do_flush_tso(unsigned int size);
	void kill_orphan(unsigned int i, ringbuf *ring);
	void drain_rings();
	void rebuild_ring_index();
	void switch_timesource(const ringbuf *ring, record *r);
	void add_internal_area();

	void inject_record(const char *ring_name, timestamp ts, uint64_t seqnum, unsigned int sect, const char *fmt, 
				uint64_t arg0 = 0, uint64_t arg1 = 0);
	void inject_record(const char *ring_name, timestamp ts, uint64_t seqnum, unsigned int sect, const char *fmt, 
				const char* arg0, const char* arg1 = 0);

public:
	engine(output &out, const options &opts = default_options);
	~engine();

	/**
	 * Check if the engine is running
	 */
	bool running() const { return _running; }

	/**
 	 * Get engine stats
 	 */
	const stats& get_stats() const { return _stats; }

	/**
 	 * Get engine options
 	 */
	const options& get_options() const { return _opts; }

	/**
	 * Add new log area.
	 */
	area *add_area(const char *name, const char **sections = 0);

	/**
	 * Find log area by name
	 */
	const area *find_area(const char *name) const;

	/**
	 * Get a pointer to the internal area
	 */
	const area *internal_area() const { return _internal_area; }

	/**
	 * Allocate new ringbuf
	 */
	ringbuf *add_ring(const char *name, const ringbuf::options &opts);

	/**
	 * Register an existing ringbuf
	 */
	bool add_ring(ringbuf *r);

	/**
	 * Find ring by name
	 */
	ringbuf *find_ring(const char *name) const;

	/**
	 * Apply mask
	 */
	void apply_mask(const mask &m);

	void list_rings(string_list &l) const;
	void list_areas(string_list &l) const;
};

std::ostream& operator<< (std::ostream& s, const engine& engine);
std::ostream& operator<< (std::ostream& s, const engine::stats& stats);
std::ostream& operator<< (std::ostream& s, const engine::options& opts);

} // namespace hogl

#endif // HOGL_DETAIL_ENGINE_HPP
