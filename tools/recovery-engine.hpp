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

#ifndef HOGL_RECOVERY_ENGINE_H
#define HOGL_RECOVERY_ENGINE_H

#include <stdint.h>

#include <list>
#include <set>
#include <map>

#include "hogl/detail/area.hpp"
#include "hogl/detail/ringbuf.hpp"
#include "hogl/detail/format.hpp"

#include "tools/coredump.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

class recovery_engine 
{
private:
	struct record_entry {
		ringbuf *ring;
		record  *rec;

		record_entry(ringbuf *_ring, record *_rec) : ring(_ring), rec(_rec) {}
	};

	struct record_sorter {
		bool operator() (const record_entry &r0, const record_entry &r1)
		{
			return r0.rec->timestamp < r1.rec->timestamp;
		}
	};

	typedef std::set<record_entry, record_sorter> record_set;
	typedef std::map<uint64_t, area*> area_map;
	typedef std::map<uint64_t, timesource*> timesource_map;
	typedef std::map<uint64_t, ostrbuf *>   ostrbuf_map;
	typedef std::list<ringbuf *> ring_list;

	area_map        _areas;
	timesource_map  _timesources;
	ring_list	_rings;
	ostrbuf_map	_outbufs;
	record_set      _records;

	format         &_format;
	coredump       &_core;
	unsigned int    _flags;

public:
	enum Flags {
		DUMP_ALL = (1<<0),
	};

	recovery_engine(coredump &core, format &fmt, unsigned int flags = 0);
	~recovery_engine();

	area* fixup_area(const void *ptr);
	timesource* fixup_timesource(const void *ptr);
	ostrbuf* fixup_ostrbuf(const void *ptr);
	void fixup_records(ringbuf *ring);
	void find_and_fixup_rings();
	void find_and_fixup_outbufs();
	void dump_rings();
	void dump_areas();
	void dump_records();
	void dump_outbufs();
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif
