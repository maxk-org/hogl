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

#include <time.h>
#include <ffi.h>
#include <stdio.h>
#include <ctype.h>

#include <sstream>

#include "hogl/detail/ntos.hpp"
#include "hogl/area.hpp"
#include "hogl/format-basic.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

format_basic::format_basic(uint32_t fields) :
	_fields(fields), _last_timestamp(0)
{ }

format_basic::format_basic(const char *fields) :
	_fields(0), _last_timestamp(0)
{
	if (!fields) {
		_fields = DEFAULT;
		return;
	}

	std::stringstream ss(fields);

	char d = ss.str().find(',') != std::string::npos ? ',' : '|';

	for (std::string s; std::getline(ss, s, d); ) {
		if      (s == "timestamp") _fields |= TIMESTAMP;
		else if (s == "timedelta") _fields |= TIMEDELTA;
		else if (s == "timespec")  _fields |= TIMESPEC;
		else if (s == "ring")      _fields |= RING;
		else if (s == "seqnum")    _fields |= SEQNUM;
		else if (s == "area")      _fields |= AREA;
		else if (s == "section")   _fields |= SECTION;
		else if (s == "recdump")   _fields |= RECDUMP;
		else if (s == "default")   _fields = DEFAULT;
		else if (s == "fast0")     _fields = FAST0;
		else if (s == "fast1")     _fields = FAST1;
	}
}

static void do_hexdump(hogl::ostrbuf &sb, const uint8_t *data, uint32_t len)
{
	const uint8_t *ptr = data;
	unsigned int offset;
	sb.cat('\n');

        for (offset = 0; offset < len; offset += 16) {
                sb.printf("\t%03d: ", offset);

		unsigned int i;
                for (i = 0; i < 16; i++) {
                        if ((i + offset) < len)
                                sb.printf("%02x ", ptr[offset + i]);
                        else
                                sb.cat("   ");
                }
                sb.cat("  ");
                for (i = 0; i < 16 && ((i + offset) < len); i++) {
			uint8_t b = ptr[offset + i];
                        sb.cat(isprint(b) ? b : '.');
		}
		sb.cat('\n');
        }
}

static void do_raw(hogl::ostrbuf &sb, const uint8_t *data, uint32_t len)
{
	sb.printf("rawdata %u bytes @ %p", len, data);
}

static void do_cstr(hogl::ostrbuf &sb, const char *str, uint32_t len)
{
	if (len)
        	sb.cat(str, len);
	else
		sb.cat("(null)");
}

void format_basic::output_without_fmt(hogl::ostrbuf &sb, const record &r, unsigned int start_with) const
{
	unsigned int i;
	for (i=start_with; i < record::NARGS; i++) {
		unsigned int type = r.get_arg_type(i);

		if (type == arg::NONE)
			return;

		union { uint64_t u64; double dbl; } v;
		const uint8_t *data; unsigned int len;

		if (arg::is_32bit(type))
			v.u64 = r.get_arg_val32(i);
		else
			v.u64 = r.get_arg_val64(i);

		// Add space between the args if needed
		if (i > 0)
			sb.cat(' ');

		switch (type) {
		case (arg::GSTR):
			sb.printf("%s", (const char *) v.u64);
			break;
		case (arg::POINTER):
			sb.printf("%p", (void *) v.u64);
			break;
		case (arg::INT32):
			sb.printf("%ld", (int32_t) v.u64);
			break;
		case (arg::INT64):
			sb.printf("%lld", (int64_t) v.u64);
			break;
		case (arg::UINT32):
			sb.printf("%lu", (uint32_t) v.u64);
			break;
		case (arg::UINT64):
			sb.printf("%llu", v.u64);
			break;
		case (arg::DOUBLE):
			sb.printf("%lf", v.dbl);
			break;
		case (arg::CSTR):
			data = r.get_arg_data(i, len);
			do_cstr(sb, (const char *) data, len);
			break;
		case (arg::HEXDUMP):
			data = r.get_arg_data(i, len);
			do_hexdump(sb, data, len);
			break;
		case (arg::RAW):
			data = r.get_arg_data(i, len);
			do_raw(sb, data, len);
			break;
		default:
			sb.printf("%lu", v.u64);
			break;
		}
	}
}

class ffi_stack
{
private:
	enum { MAX_DEPTH = 24 };

	ffi_type    *_arg_type[MAX_DEPTH];
	void        *_arg_val[MAX_DEPTH];
	void        *_arg_shadow[MAX_DEPTH];
	unsigned int _depth;

public:
	unsigned int depth() const { return _depth; }

	ffi_type **arg_type() { return _arg_type; }
	void **arg_val() { return _arg_val; }

	void populate(const record &r, unsigned int start_with = 0);
	void add_arg(const record &r, unsigned int type, unsigned int i);
	void add_arg(ffi_type *type, void *val);

	void add_cstr(const record &r, unsigned int i);

	void reset() { _depth = 0; }
	ffi_stack() { reset(); }
};

void ffi_stack::add_arg(ffi_type *type, void *val)
{
	_arg_type[_depth] = type;
	_arg_val[_depth]  = val;
	_depth++;
}

void ffi_stack::add_arg(const record &r, unsigned int type, unsigned int i)
{
	static const char *hexdump_not_supported = "hexdump not supported with format strings";
	static const char *rawdata_not_supported = "rawdata not supported with format strings";

	unsigned int len;

	_arg_val[_depth] = (void *) &r.argval[i];

	switch (type) {
	case (arg::CSTR):
		_arg_shadow[_depth] = (void*) r.get_arg_data(i, len);
		if (!len)
			_arg_shadow[_depth] = 0;

		_arg_type[_depth] = &ffi_type_pointer;
		_arg_val[_depth] = (void *) &_arg_shadow[_depth];
		break;
	case (arg::GSTR):
	case (arg::POINTER):
		_arg_type[_depth] = &ffi_type_pointer;
		break;
	case (arg::INT32):
		_arg_type[_depth] = &ffi_type_sint32;
		break;
	case (arg::INT64):
		_arg_type[_depth] = &ffi_type_sint64;
		break;
	case (arg::UINT32):
		_arg_type[_depth] = &ffi_type_uint32;
		break;
	case (arg::UINT64):
		_arg_type[_depth] = &ffi_type_uint64;
		break;
	case (arg::DOUBLE):
		_arg_type[_depth] = &ffi_type_double;
		break;
	case (arg::HEXDUMP):
		_arg_type[_depth] = &ffi_type_pointer;
		_arg_val[_depth]  = &hexdump_not_supported;
		break;
	case (arg::RAW):
		_arg_type[_depth] = &ffi_type_pointer;
		_arg_val[_depth]  = &rawdata_not_supported;
		break;
	default:
		_arg_type[_depth] = &ffi_type_uint64;
		break;
	}

	_depth++;
}

void ffi_stack::populate(const record &r, unsigned int start_with)
{
	unsigned int i;
	for (i=start_with; i < record::NARGS && _depth < MAX_DEPTH; i++) {
		unsigned int type = r.get_arg_type(i);
		if (type == arg::NONE)
			break;

		add_arg(r, type, i);
	}
}

// This function uses libffi to dynamically construct
// a call to
// 	fprintf(sb.stdio(), fmt, arg0, arg1, ...);
void format_basic::output_with_fmt(hogl::ostrbuf &sb, const record &r, unsigned int start_with) const
{
	ffi_stack stack;
	ffi_cif   cif;

	FILE* file = sb.stdio();

	stack.add_arg(&ffi_type_pointer, &file);
	stack.populate(r, start_with);

	ffi_status err;
        err = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, stack.depth(), &ffi_type_uint, stack.arg_type());
	if (err != FFI_OK) {
		fprintf(stderr, "hogl::format_basic: FFI failed %d\n", err);
		abort();
	}

	ffi_arg result;
        ffi_call(&cif, FFI_FN(fprintf), &result, stack.arg_val());
}

void format_basic::output_raw(hogl::ostrbuf &sb, const record &r) const
{
	unsigned int len; const uint8_t *data = r.get_arg_data(0, len);
	do_raw(sb, data, len);
}

void format_basic::default_header(const format::data &d, ostrbuf &sb)
{
	const hogl::record &r = *d.record;
	const hogl::area *area = r.area;

	const char *area_name = "INVALID";
	const char *sect_name = "INVALID";
	const char *ring_name = d.ring_name;

	if (area) {
		area_name = area->name();
		sect_name = area->section_name(r.section);
	}

	struct tm tm;
	struct timespec ts;
	d.record->timestamp.to_timespec(ts);
        localtime_r(&ts.tv_sec, &tm);

	sb.printf("%02u%02u%04u %02u:%02u:%02u.%09u %s:%lu %s:%s ",
			(tm.tm_mon + 1), tm.tm_mday, (1900 + tm.tm_year),
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec,
			ring_name, r.seqnum, area_name, sect_name);
}

void format_basic::tscache::update(hogl::timestamp t)
{
	struct timespec ts; t.to_timespec(ts);

	uint32_t nhi = ts.tv_nsec / NSEC_SPLIT;
	uint32_t nlo = ts.tv_nsec % NSEC_SPLIT;
	unsigned int i = 0;

	if (_sec != ts.tv_sec) {
		_sec = ts.tv_sec;
		u64tod(_sec, _str, i, SEC_LEN);
	}

	i = NSEC_HI_OFFSET;
	if (_nsec_hi != nhi) {
		_nsec_hi = nhi;
		u64tod(nhi, _str, i, NSEC_HI_LEN);
	}

	i = NSEC_LO_OFFSET;
	u64tod(nlo, _str, i, NSEC_LO_LEN);
}

void format_basic::add_timespec(hogl::timestamp t, uint8_t *str, unsigned int &i)
{
	_tscache.update(t);
	memcpy(str + i, _tscache.str(), _tscache.len()); i += _tscache.len();
}

// Super fast header formatter
void format_basic::fast0_header(const format::data &d, ostrbuf &sb)
{
	const hogl::record &r = *d.record;
	const hogl::area *area = r.area;

	const char *area_name = "INVALID";
	const char *sect_name = "INVALID";
	const char *ring_name = d.ring_name;

	if (area) {
		area_name = area->name();
		sect_name = area->section_name(r.section);
	}

	// The following sequence is equvalent to 
	//	sb.printf("%lu.%09u %s:%lu %s:%s ", ts.tv_sec, ts.tv_nsec,
	//			d.ring, d.record->seqnum, area_name, sect_name);

	unsigned int len_area_name = strlen(area_name);
	unsigned int len_sect_name = strlen(sect_name);
	unsigned int len_ring_name = strlen(ring_name);

	// Compute total header len	
	unsigned int hlen = len_area_name + len_sect_name + len_ring_name + 6 + 3 * 20;

	uint8_t str[hlen];
	unsigned int i = 0;

	add_timespec(r.timestamp, str, i);
	str[i++] = ' ';
	memcpy(&str[i], ring_name, len_ring_name); i += len_ring_name;
	str[i++] = ':';
	u64tod(r.seqnum, str, i);
	str[i++] = ' ';
	memcpy(&str[i], area_name, len_area_name); i += len_area_name;
	str[i++] = ':';
	memcpy(&str[i], sect_name, len_sect_name); i += len_sect_name;
	str[i++] = ' ';

	sb.put(str, i);
}

// Same as above plus timedelta
void format_basic::fast1_header(const format::data &d, ostrbuf &sb)
{
	const hogl::record &r = *d.record;
	const hogl::area *area = r.area;

	const char *area_name = "INVALID";
	const char *sect_name = "INVALID";
	const char *ring_name = d.ring_name;

	if (area) {
		area_name = area->name();
		sect_name = area->section_name(r.section);
	}

	timestamp delta = r.timestamp - _last_timestamp;
	if (hogl_unlikely(_last_timestamp == timestamp(0)))
		delta = 0;

	_last_timestamp = r.timestamp;

	// The following sequence is equvalent to:
	// 	sb.printf("%lu.%09u (%llu) %s:%lu %s:%s ", ts.tv_sec, ts.tv_nsec, delta.to_nsec(),
	//		d.ring, d.record->seqnum, area_name, sect_name);
	unsigned int len_area_name = strlen(area_name);
	unsigned int len_sect_name = strlen(sect_name);
	unsigned int len_ring_name = strlen(ring_name);

	// Compute total header len	
	unsigned int hlen = len_area_name + len_sect_name + len_ring_name + 9 + 4 * 20;

	uint8_t str[hlen];
	unsigned int i = 0;

	add_timespec(r.timestamp, str, i);
	str[i++] = ' ';
	str[i++] = '(';
	u64tod(delta, str, i);
	str[i++] = ')';
	str[i++] = ' ';
	memcpy(&str[i], ring_name, len_ring_name); i += len_ring_name;
	str[i++] = ':';
	u64tod(r.seqnum, str, i);
	str[i++] = ' ';
	memcpy(&str[i], area_name, len_area_name); i += len_area_name;
	str[i++] = ':';
	memcpy(&str[i], sect_name, len_sect_name); i += len_sect_name;
	str[i++] = ' ';

	sb.put(str, i);
}

void format_basic::flexible_header(const format::data &d, ostrbuf &sb)
{
	const hogl::record &r = *d.record;

	const hogl::area *area = r.area;
	const char *area_name = "INVALID";
	const char *sect_name = "INVALID";
	const char *ring_name = d.ring_name;

	if (area) {
		area_name = area->name();
		sect_name = area->section_name(r.section);
	}

	if (_fields & RECDUMP) {
		sb.printf("ring %s record %p area %p section %u timestamp %llu seqnum %llu argtype 0x%x "
			"args:[ 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx ]\n",
				ring_name, &r, area, r.section, r.timestamp.to_nsec(),
				r.seqnum, r.argtype,
				r.argval[0], r.argval[1], r.argval[2], r.argval[3],
				r.argval[4], r.argval[5], r.argval[6], r.argval[7]);
	}

	if (_fields & TIMESPEC) {
		_tscache.update(r.timestamp);
		sb.cat(_tscache.str(), _tscache.len());
		sb.cat(' ');
	}

	if (_fields & TIMESTAMP) {
		struct tm tm;
		struct timespec ts;
		r.timestamp.to_timespec(ts);

        	localtime_r(&ts.tv_sec, &tm);

		sb.printf("%02u%02u%04u %02u:%02u:%02u.%09u ",
			(tm.tm_mon + 1), tm.tm_mday, (1900 + tm.tm_year),
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}

	if (_fields & TIMEDELTA) {
		timestamp delta = r.timestamp - _last_timestamp;
		if (hogl_unlikely(_last_timestamp == timestamp(0)))
			delta = 0;
		sb.printf("(%llu) ", delta.to_nsec());
		_last_timestamp = r.timestamp;
	}

	switch(_fields & (RING | SEQNUM)) {
	case (RING | SEQNUM):
		sb.printf("%s:%lu ", ring_name, r.seqnum);
		break;
	case RING:
		sb.printf("%s ", ring_name);
		break;
	case SEQNUM:
		sb.printf("%lu ", r.seqnum);
		break;
	}

	switch(_fields & (AREA | SECTION)) {
	case (AREA | SECTION):
		sb.printf("%s:%s ", area_name, sect_name);
		break;
	case AREA:
		sb.printf("%s ", area_name);
		break;
	case SECTION:
		sb.printf("%s ", sect_name);
		break;
	}
}

void format_basic::process(ostrbuf &sb, const format::data &d)
{
	const hogl::record &r = *d.record;

	if (_fields == DEFAULT)
		default_header(d, sb);
	else if (_fields == FAST0)
		fast0_header(d, sb);
	else if (_fields == FAST1)
		fast1_header(d, sb);
	else
		flexible_header(d, sb);

	unsigned int t0 = r.get_arg_type(0);
	unsigned int t1 = r.get_arg_type(1);

	if ((t0 == arg::CSTR || t0 == arg::GSTR) && t1 != arg::NONE)
		output_with_fmt(sb, r);
	else if (t0 == arg::RAW)
		output_raw(sb, r);
	else
		output_without_fmt(sb, r);

	sb.cat('\n');
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

