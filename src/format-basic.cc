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
	sb.cat('\n');
}

static void do_raw(hogl::ostrbuf &sb, const uint8_t *data, uint32_t len)
{
	sb.printf("rawdata %u bytes @ %p\n", len, data);
}

void format_basic::output_plain(hogl::ostrbuf &sb, record_data& d) 
{
	const record& r = *d.record;

	for (unsigned int i = d.next_arg; i < r.NARGS; i++) {
		unsigned int type = r.get_arg_type(i);

		if (type == arg::NONE)
			break;

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
		case (arg::CSTR):
			sb.printf("%s", d.arg_str[i]);
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

	sb.cat('\n');
}

class ffi_stack
{
private:
	enum { MAX_DEPTH = 24 };

	ffi_type    *_arg_type[MAX_DEPTH];
	void        *_arg_val[MAX_DEPTH];
	unsigned int _depth;

public:
	unsigned int depth() const { return _depth; }

	ffi_type **arg_type() { return _arg_type; }
	void **arg_val() { return _arg_val; }

	void add_arg(const format_basic::record_data& d, unsigned int type, unsigned int i);
	void add_arg(ffi_type *type, void *val);
	void populate(format_basic::record_data& d);

	void reset() { _depth = 0; }
	ffi_stack() { reset(); }
};

void ffi_stack::add_arg(ffi_type *type, void *val)
{
	_arg_type[_depth] = type;
	_arg_val[_depth]  = val;
	_depth++;
}

void ffi_stack::add_arg(const format_basic::record_data& d, unsigned int type, unsigned int i)
{
	static const char *hexdump_not_supported = "hexdump not supported with format strings";
	static const char *rawdata_not_supported = "rawdata not supported with format strings";

	const record& r = *d.record;
	_arg_val[_depth] = (void *) &r.argval[i];

	switch (type) {
	case (arg::CSTR):
	case (arg::GSTR):
		_arg_type[_depth] = &ffi_type_pointer;
		_arg_val[_depth]  = (void *) &d.arg_str[i];
		break;
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

void ffi_stack::populate(format_basic::record_data& d)
{
	const record& r = *d.record;

	for (unsigned int i = d.next_arg; i < record::NARGS && _depth < MAX_DEPTH; i++) {
		unsigned int type = r.get_arg_type(i);
		if (type == arg::NONE)
			break;
		add_arg(d, type, i);
	}
}

// This function uses libffi to dynamically construct
// a call to
// 	fprintf(sb.stdio(), fmt, arg0, arg1, ...);
void format_basic::output_fmt(hogl::ostrbuf &sb, record_data& d) 
{
	ffi_stack stack;
	ffi_cif   cif;

	FILE* file = sb.stdio();

	stack.add_arg(&ffi_type_pointer, &file);
	stack.populate(d);

	ffi_status err;
        err = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, stack.depth(), &ffi_type_uint, stack.arg_type());
	if (err != FFI_OK) {
		fprintf(stderr, "hogl::format_basic: FFI failed %d\n", err);
		abort();
	}

	ffi_arg result;
        ffi_call(&cif, FFI_FN(fprintf), &result, stack.arg_val());

	sb.cat('\n');
}

void format_basic::output_raw(hogl::ostrbuf& sb, record_data& d)
{
	const record& r = *d.record;
	unsigned int len; const uint8_t *data = r.get_arg_data(0, len);
	do_raw(sb, data, len);
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

// Default header with date/time
void format_basic::default_header(ostrbuf& sb, record_data& d)
{
	const hogl::record &r = *d.record;

	struct tm tm;
	struct timespec ts;
	r.timestamp.to_timespec(ts);
        localtime_r(&ts.tv_sec, &tm);

	sb.printf("%02u%02u%04u %02u:%02u:%02u.%09u %s:%lu %s:%s ",
			(tm.tm_mon + 1), tm.tm_mday, (1900 + tm.tm_year),
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec,
			d.ring_name, r.seqnum, d.area_name, d.sect_name);
}

// Super fast header formatter
void format_basic::fast0_header(ostrbuf& sb, record_data& d)
{
	const hogl::record &r = *d.record;

	// The following sequence is equvalent to 
	//	sb.printf("%lu.%09u %s:%lu %s:%s ", ts.tv_sec, ts.tv_nsec,
	//			d.ring_name, r.seqnum, d.area_name, d.sect_name);

	unsigned int len_area_name = strlen(d.area_name);
	unsigned int len_sect_name = strlen(d.sect_name);
	unsigned int len_ring_name = strlen(d.ring_name);

	// Compute total header len
	unsigned int hlen = len_area_name + len_sect_name + len_ring_name + 6 + 3 * 20;

	uint8_t str[hlen];
	unsigned int i = 0;

	_tscache.update(r.timestamp);
	memcpy(&str[i], _tscache.str(), _tscache.len()); i += _tscache.len();
	str[i++] = ' ';
	memcpy(&str[i], d.ring_name, len_ring_name); i += len_ring_name;
	str[i++] = ':';
	u64tod(r.seqnum, str, i);
	str[i++] = ' ';
	memcpy(&str[i], d.area_name, len_area_name); i += len_area_name;
	str[i++] = ':';
	memcpy(&str[i], d.sect_name, len_sect_name); i += len_sect_name;
	str[i++] = ' ';

	sb.put(str, i);
}

// Same as above plus timedelta
void format_basic::fast1_header(ostrbuf& sb, record_data& d)
{
	const hogl::record &r = *d.record;

	timestamp delta = r.timestamp - _last_timestamp;
	if (hogl_unlikely(_last_timestamp == timestamp(0)))
		delta = 0;

	_last_timestamp = r.timestamp;

	// The following sequence is equvalent to:
	// 	sb.printf("%lu.%09u (%llu) %s:%lu %s:%s ", ts.tv_sec, ts.tv_nsec, delta.to_nsec(),
	//		d.ring_name, r.seqnum, d.area_name, d.sect_name);
	unsigned int len_area_name = strlen(d.area_name);
	unsigned int len_sect_name = strlen(d.sect_name);
	unsigned int len_ring_name = strlen(d.ring_name);

	// Compute total header len	
	unsigned int hlen = len_area_name + len_sect_name + len_ring_name + 9 + 4 * 20;

	uint8_t str[hlen];
	unsigned int i = 0;

	_tscache.update(r.timestamp);
	memcpy(&str[i], _tscache.str(), _tscache.len()); i += _tscache.len();
	str[i++] = ' ';
	str[i++] = '(';
	u64tod(delta, str, i);
	str[i++] = ')';
	str[i++] = ' ';
	memcpy(&str[i], d.ring_name, len_ring_name); i += len_ring_name;
	str[i++] = ':';
	u64tod(r.seqnum, str, i);
	str[i++] = ' ';
	memcpy(&str[i], d.area_name, len_area_name); i += len_area_name;
	str[i++] = ':';
	memcpy(&str[i], d.sect_name, len_sect_name); i += len_sect_name;
	str[i++] = ' ';

	sb.put(str, i);
}

void format_basic::flexi_header(ostrbuf& sb, record_data& d)
{
	const hogl::record &r = *d.record;

	if (_fields & RECDUMP) {
		sb.printf("ring %s record %p area %p section %u timestamp %llu seqnum %llu argtype 0x%x "
			"args:[ 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx ]\n",
				d.ring_name, &r, r.area, r.section, r.timestamp.to_nsec(),
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
		sb.printf("%s:%lu ", d.ring_name, r.seqnum);
		break;
	case RING:
		sb.printf("%s ", d.ring_name);
		break;
	case SEQNUM:
		sb.printf("%lu ", r.seqnum);
		break;
	}

	switch(_fields & (AREA | SECTION)) {
	case (AREA | SECTION):
		sb.printf("%s:%s ", d.area_name, d.sect_name);
		break;
	case AREA:
		sb.printf("%s ", d.area_name);
		break;
	case SECTION:
		sb.printf("%s ", d.sect_name);
		break;
	}
}

static const char* get_arg_str(const record& r, unsigned int type, unsigned int i)
{
	if (type == arg::CSTR) {
		unsigned int len;
		const char *str = (const char*) r.get_arg_data(i, len);
		if (!len)
			return "null";
		return str;
	}

	uint64_t ptr;
	if (arg::is_32bit(type))
		ptr = r.get_arg_val32(i);
	else
		ptr = r.get_arg_val64(i);
	return (const char *) ptr;
}

void format_basic::process(ostrbuf &sb, const format::data &d)
{
	const hogl::record &r = *d.record;

	record_data rd = {};
	rd.record    = d.record;
	rd.ring_name = d.ring_name;
	rd.next_arg  = 0;

	// Preprocess names
	const hogl::area *area = r.area;
	if (area) {
		rd.area_name = area->name();
		rd.sect_name = area->section_name(r.section);
	} else {
		rd.area_name = "INVALID";
		rd.sect_name = "INVALID";
	}

	// Preprocess strings
	for (unsigned int i = 0; i < record::NARGS; i++) {
		unsigned int type = r.get_arg_type(i);
		if (type == arg::NONE)
			break;
		if (type == arg::CSTR || type == arg::GSTR)
			rd.arg_str[i] = get_arg_str(r, type, i);
	}

	if (_fields == DEFAULT)
		default_header(sb, rd);
	else if (_fields == FAST0)
		fast0_header(sb, rd);
	else if (_fields == FAST1)
		fast1_header(sb, rd);
	else
		flexi_header(sb, rd);

	unsigned int t0 = r.get_arg_type(0);
	unsigned int t1 = r.get_arg_type(1);

	if ((t0 == arg::CSTR || t0 == arg::GSTR) && t1 != arg::NONE)
		output_fmt(sb, rd);
	else if (t0 == arg::RAW)
		output_raw(sb, rd);
	else
		output_plain(sb, rd);
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

