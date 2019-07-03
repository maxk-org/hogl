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

#include <algorithm>

#include "hogl/detail/ostrbuf-stdio.hpp"
#include "hogl/detail/output.hpp"

#include "tools/recovery-engine.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

// Bitmap validation and fixup 
class bitmap_validator : public bitmap 
{
public:
	static bool fixup(coredump &core, void *ptr)
	{
		if (!ptr)
			return false;

		bitmap_validator *b = (bitmap_validator *) ptr;

		// Lets do some sanity checks here
		// 256K of sections is probably too many
		if (!b->size() || b->size() > 256*1024)
			return false;

		b->_data = (bitmap::word *) core.remap(b->_data, b->nbytes(b->size()));
		if (!b->_data)
			return false;
		return true;
	}
};

// Area validation and fixup 
class area_validator : public area
{
public:
	static area* fixup(coredump &core, void *ptr)
	{
		if (!ptr)
			return 0;

		area_validator *a = (area_validator *) ptr;

		// Lets do some sanity checks here
		if (memcmp(&a->_magic, &hogl::area_magic, sizeof(hogl::magic)))
			return 0;

		if (!bitmap_validator::fixup(core, &a->_bitmap))
			return 0;

		// 256K of sections is probably too many
		if (!a->count() || a->count() > 256*1024)
			return 0;

		a->_name = (char *) core.remap(a->_name);
		if (!a->_name)
			return 0;

		// Fixup section names
		a->_section = (const char **) core.remap(a->_section, sizeof(char *) * a->count());
		if (!a->_section)
			return 0;

		unsigned int i;
		for (i=0; i < a->count(); ++i)
			a->_section[i] = (const char *) core.remap((void *) a->_section[i]);

		return a;
	}
};

// Timesource validation and fixup
class timesource_validator : public timesource
{
public:
	static timesource* fixup(coredump &core, void *ptr)
	{
		if (!ptr)
			return 0;

		timesource_validator *ts = (timesource_validator *) ptr;

		ts->_name = (char *) core.remap(ts->_name);
                if (!ts->_name)
                        return 0;
		return ts;
	}
};

// Ring validation and fixup
class ring_validator : public ringbuf
{
public:
	static bool fixup(coredump &core, void *ptr)
	{
		ring_validator *r = (ring_validator *) ptr;

		// Lets do some sanity checks here.
		// Should be self-explanatory.

		if (r->dropcnt() > r->seqnum())
			return false;

		if (r->prio() > r->PRIORITY_CEILING)
			return false;

		// Record size of more than 1MB is definitely insane
		if (!r->record_size() || r->record_size() > 1024*1024)
			return false;
 
		if (r->record_tailroom() > r->record_size())
			return false;

		if (r->_head > r->_capacity || r->_tail > r->_capacity)
			return false;

		r->_name    = (char *)    core.remap(r->_name);
		r->_rec_top = (uint8_t *) core.remap(r->_rec_top, r->record_size() * r->capacity());

		if (!r->_name || !r->_rec_top)
			return false;

		return true;
	}
};

class ostrbuf_validator : public ostrbuf
{
public:
	static bool fixup(coredump &core, void *ptr)
	{
		ostrbuf_validator *s = (ostrbuf_validator *) ptr;

		// Lets do some sanity checks here.
		// Should be self-explanatory.

		if (!s->_capacity || s->_capacity > 256 * 1024 * 1024)
			return false;
		if (s->_size > s->_capacity)
			return false;

		s->_data = (uint8_t *) core.remap(s->_data);
		if (!s->_data)
			return false;

		return true;
	}
};

// Output validation and fixup
class output_validator : public output
{
public:
	static ostrbuf* fixup(coredump &core, void *ptr)
	{
		// Account for the vtable pointer that preceeds magic signature,
		// output class has virtual methods.
		ptr = (uint8_t *) ptr - sizeof(void *);

		output_validator *o = (output_validator *) ptr;

		o->_ostrbuf = (ostrbuf *) core.remap(o->_ostrbuf);
                if (!o->_ostrbuf)
                        return 0;
		if (!ostrbuf_validator::fixup(core, o->_ostrbuf))
			return 0;
		return o->_ostrbuf;
	}
};

hogl::timesource* recovery_engine::fixup_timesource(const void *ptr)
{
	// First look up the timesource in the map
	timesource_map::const_iterator it = _timesources.find((uint64_t) ptr);
	if (it != _timesources.end())
        	return it->second;

	// Ok. It's not in the map.
	// We need to fix it up and update the map.
	// ** Note that we update the map even if validator fails,
	// since there is no point in trying to validate that area
	// over and over again. 
	timesource *ts = timesource_validator::fixup(_core, _core.remap(ptr, sizeof(hogl::timesource)));
	_timesources.insert(timesource_map::value_type((uint64_t) ptr, ts));

	return ts;
}

// Dummy area used for special records.
// Last section must be named INVALID and is used to map all INVALID values to.
static const char *special_sections[] = { "FLUSH", "TIMESOURCE_CHANGE", "INVALID", 0 };
static class area special_area("SPECIAL", special_sections);

hogl::area* recovery_engine::fixup_area(const void *ptr)
{
	// First look up the area in the map
	area_map::const_iterator it = _areas.find((uint64_t) ptr);
	if (it != _areas.end())
        	return it->second;

	// Ok. It's not in the map.
	// We need to fix it up and update the map.
	// ** Note that we update the map even if validator fails,
	// since there is no point in trying to validate that area
	// over and over again. 
	// The format handler can deal with busted areas.
	area *a = area_validator::fixup(_core, _core.remap(ptr, sizeof(hogl::area)));
	_areas.insert(area_map::value_type((uint64_t) ptr, a));

	return a;
}

// Validate and fixup records.
// Things like area pointers, argument pointers, etc.
// Note that this function fixes up areas as it goes through the records.
void recovery_engine::fixup_records(ringbuf *ring)
{
	ringbuf::pop_iterator it(ring);
	record *r;
	while ((r = it.next())) {
		if (r->special()) {
			// Special record.
			// Remap area/section and reset argtype.
			r->area = &special_area;
			r->section = std::min(r->argtype, (uint64_t) special_area.size() - 1);
			r->argtype = 0;
			continue;
		}

		// Normal record. 
		// Fixup area and string arguments

		r->area = fixup_area(r->area);

		unsigned int i;
		for (i=0; i < 8; i++) {
			unsigned int type = r->get_arg_type(i);
			switch (type) {
			case arg::GSTR:
				if (arg::is_32bit(arg::GSTR))
					r->argval[i].u32 = (uint64_t) _core.remap((void *) (uint64_t) r->get_arg_val32(i));
				else
					r->argval[i].u64 = (uint64_t) _core.remap((void *) r->get_arg_val64(i));
				break;
			default:
				break;
			}
		}
	}
}

void recovery_engine::find_and_fixup_rings()
{
	// Iterate over coredump sections and find all ring buffers
	coredump::section_set::const_iterator it;
	for (it = _core.sections.begin(); it != _core.sections.end(); ++it) {
		coredump::section *s = *it;
		unsigned long size = s->size;
		uint8_t *ptr = s->addr;

		while (size >= sizeof(hogl::ringbuf)) {
			ptr = (uint8_t *) memmem(ptr, size, &hogl::ring_magic, sizeof(hogl::ring_magic));
			if (!ptr)
				break;

			if (!s->inside(ptr, sizeof(hogl::ringbuf)))
				break;

			hogl::ringbuf *ring = (hogl::ringbuf *) ptr;
			if (ring_validator::fixup(_core, ring)) {
				ring->_timesource = fixup_timesource(ring->_timesource);

				// Found a good ring.
				fixup_records(ring);
				_rings.push_back(ring);

				ptr += sizeof(hogl::ringbuf);
			} else {
				// Validation failed.
				// Advance by one byte and continue scanning.
				ptr += 1;
			}
			size = s->size - (ptr - s->addr);
		}
	}
}

void recovery_engine::find_and_fixup_outbufs()
{
	// Iterate over coredump sections and find all output buffers
	coredump::section_set::const_iterator it;
	for (it = _core.sections.begin(); it != _core.sections.end(); ++it) {
		coredump::section *s = *it;
		unsigned long size = s->size;
		uint8_t *ptr = s->addr;

		while (size >= sizeof(hogl::ringbuf)) {
			ptr = (uint8_t *) memmem(ptr, size, &hogl::output_magic, sizeof(hogl::output_magic));
			if (!ptr)
				break;

			if (!s->inside(ptr, sizeof(hogl::output)))
				break;

			hogl::ostrbuf *sb = output_validator::fixup(_core, ptr);
			if (sb) {
				// Found a valid output buffer
				_outbufs.push_back(sb);

				ptr += sizeof(hogl::output);
			} else {
				// Validation failed.
				// Advance by one byte and continue scanning.
				ptr += 1;
			}
			size = s->size - (ptr - s->addr);
		}
	}
}

void recovery_engine::dump_rings()
{
	ring_list::const_iterator it;
	for (it = _rings.begin(); it != _rings.end(); ++it) {
		ringbuf *r = *it;
		std::cout << *r;
	}
}

void recovery_engine::dump_areas()
{
	area_map::const_iterator it;
	for (it = _areas.begin(); it != _areas.end(); ++it) {
		area *a = it->second;
		if (!a)
			continue;
		std::cout << *a;
	}
}

void recovery_engine::dump_records()
{
	fprintf(stdout, "Dumping records from ring buffers:\n");

	// Iterate over all rings and insert records into the 
	// record_set (sorted by timestamp).
	ring_list::const_iterator ring_it;
	for (ring_it = _rings.begin(); ring_it != _rings.end(); ++ring_it) {
		ringbuf::pop_iterator pit(*ring_it);
		record *r;
		while ((r = pit.next()))
			_records.insert(record_entry(pit.ring(), r));
	}

	// Iterate over sorted record set, format the records
	// and pipe them to stdout
	ostrbuf_stdio sb(stdout, 8192);

	record_set::const_iterator rec_it;
	for (rec_it = _records.begin(); rec_it != _records.end(); ++rec_it) {
		record_entry re = *rec_it;
		format::data d;
		d.ring_name = re.ring->name();
		d.record    = re.rec;
		_format.process(sb, d);
	}

	sb.flush();
}

void recovery_engine::dump_outbufs()
{
	// Iterate over all output buffers and dump their content
	ostrbuf_list::const_iterator sb_it;
	for (sb_it = _outbufs.begin(); sb_it != _outbufs.end(); ++sb_it) {
		ostrbuf *sb = *sb_it;
		fprintf(stdout, "Dumping output buffer: capacity %u size %u\n",
			(unsigned int) sb->capacity(), (unsigned int) sb->size());
		fwrite(sb->head(), sb->size(), 1, stdout);
		fprintf(stdout, "\n");
	}
}

recovery_engine::recovery_engine(coredump &core, format &fmt, unsigned int flags):
	_format(fmt), _core(core), _flags(flags)
{
	find_and_fixup_rings();
	find_and_fixup_outbufs();
}

recovery_engine::~recovery_engine()
{

}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

