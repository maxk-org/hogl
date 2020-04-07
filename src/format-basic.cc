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
		else if (s == "default")   _fields = DEFAULT;
		else if (s == "fast0")     _fields = FAST0;
		else if (s == "fast1")     _fields = FAST1;
	}
}

static void do_hexdump(hogl::ostrbuf &sb, const uint8_t *data, uint32_t n, const hogl::arg_xdump::format& xf)
{
	sb.push_back('\n');

	unsigned int offset;
	for (offset = 0; offset < n; offset += xf.line_width) {
		sb.printf("\t%03d: ", offset);

		unsigned int i;
		for (i = 0; i < xf.line_width; i++) {
			if ((i + offset) < n)
				sb.printf("%02x ", data[offset + i]);
			else
				sb.push_back("   ");
		}
		sb.push_back("  ");
		for (i = 0; i < xf.line_width && ((i + offset) < n); i++) {
			uint8_t b = data[offset + i];
			sb.push_back(isprint(b) ? b : '.');
		}
		sb.push_back('\n');
	}
}

template <typename T>
static void xdump_single(hogl::ostrbuf &sb, const T* data, uint32_t n, const char *fmt, const hogl::arg_xdump::format& xf)
{
	struct safe_ptr { T v; } hogl_packed;
	safe_ptr* ptr = (safe_ptr*) data;

	for (unsigned int i=0; i < n; i++) {
		if (i) sb.push_back(xf.delim);
		sb.printf(fmt, ptr[i].v);
	}
	sb.push_back('\n');
}

template <typename T>
static void xdump_multi(hogl::ostrbuf &sb, const T* data, uint32_t n, const char *fmt, const hogl::arg_xdump::format& xf)
{
	if (!xf.line_width)
		return xdump_single(sb, data, n, fmt, xf);

	struct safe_ptr { T v; } hogl_packed;
	safe_ptr* ptr = (safe_ptr*) data;

	sb.push_back("\n");
	unsigned int lw = 0;
	for (unsigned int i=0; i < n; i++) {
		sb.push_back(lw ? xf.delim : '\t');
		sb.printf(fmt, ptr[i].v); lw += 1;
		if (lw == xf.line_width) { sb.push_back("\n"); lw = 0; }
	}
	if (lw)
		sb.push_back('\n');
}

static const char* __flt_fmt(unsigned int pr)
{
	switch(pr) {
	case 1: return "%.1f";
	case 2: return "%.2f";
	case 3: return "%.3f";
	case 4: return "%.4f";
	case 5: return "%.5f"; }
	return "%f";
}

static const char* __dbl_fmt(unsigned int pr)
{
	switch(pr) {
	case 1: return "%.1lf";
	case 2: return "%.2lf";
	case 3: return "%.3lf";
	case 4: return "%.4lf";
	case 5: return "%.5lf"; }
	return "%f";
}

static void do_xdump(hogl::ostrbuf &sb, const uint8_t *data, uint32_t len)
{
	const arg_xdump::format& xf = *(const arg_xdump::format *) data;

	const uint8_t *ptr = data + sizeof(arg_xdump::format);
	len = (len - sizeof(arg_xdump::format)) / xf.byte_width;

	switch (xf.type) {
	case 'H':
		return do_hexdump(sb, ptr, len, xf);

	case 'd':
		switch (xf.byte_width) {
		case 1: return xdump_multi(sb, (int8_t*)  ptr, len, "%d",   xf);
		case 2: return xdump_multi(sb, (int16_t*) ptr, len, "%d",   xf);
		case 4: return xdump_multi(sb, (int32_t*) ptr, len, "%ld",  xf);
		case 8: return xdump_multi(sb, (int64_t*) ptr, len, "%lld", xf); }
		break;

	case 'u':
		switch (xf.byte_width) {
		case 1: return xdump_multi(sb, (uint8_t*) ptr, len, "%u",   xf);
		case 2: return xdump_multi(sb, (uint16_t*)ptr, len, "%u",   xf);
		case 4: return xdump_multi(sb, (uint32_t*)ptr, len, "%lu",  xf);
		case 8: return xdump_multi(sb, (uint64_t*)ptr, len, "%llu", xf); }
		break;

	case 'f':
		switch (xf.byte_width) {
		case 4: return xdump_multi(sb, (float*) ptr, len, __flt_fmt(xf.precision), xf);
		case 8: return xdump_multi(sb, (double*)ptr, len, __dbl_fmt(xf.precision), xf); }
	}

	sb.printf("xdump conversion type %c not supported", xf.type);
	return;
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

		union { uint64_t u64; int64_t i64; double dbl; } v;
		const uint8_t *data; unsigned int len;

		if (arg::is_32bit(type))
			v.u64 = r.get_arg_val32(i);
		else
			v.u64 = r.get_arg_val64(i);

		// Add space between the args if needed
		if (i > 0)
			sb.push_back(' ');

		switch (type) {
		case (arg::GSTR):
		case (arg::CSTR):
			sb.printf("%s", d.arg_str[i]);
			break;
		case (arg::POINTER):
			sb.printf("%p", (void *) v.u64);
			break;
		case (arg::INT32):
			sb.printf("%ld", (int32_t) v.i64);
			break;
		case (arg::UINT32):
			sb.printf("%lu", (uint32_t) v.u64);
			break;
		case (arg::INT64):
			sb.printf("%lld", v.i64);
			break;
		case (arg::UINT64):
			sb.printf("%llu", v.u64);
			break;
		case (arg::DOUBLE):
			sb.printf("%lf", v.dbl);
			break;
		case (arg::XDUMP):
			data = r.get_arg_data(i, len);
			do_xdump(sb, data, len);
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

	sb.push_back('\n');
}

void format_basic::output_fmt(hogl::ostrbuf &sb, record_data& d) 
{
	using bi  = std::back_insert_iterator<hogl::ostrbuf>;
	using ctx = fmt::basic_printf_context<bi, char>;
	using arg_fmt = fmt::printf_arg_formatter<fmt::internal::output_range<bi, char>>;

	fmt::dynamic_format_arg_store<ctx> arg_store;

	const record& r = *d.record;

	for (unsigned int i = d.next_arg + 1; i < record::NARGS; i++) {
	        unsigned int type = r.get_arg_type(i);
	        if (type == arg::NONE)
	                break;

		union { uint64_t u64; int64_t i64; double dbl; } v;
		if (arg::is_32bit(type))
			v.u64 = r.get_arg_val32(i);
		else
			v.u64 = r.get_arg_val64(i);

		switch (type) {
		case (arg::CSTR):
		case (arg::GSTR):
			arg_store.push_back(d.arg_str[i]);
			break;
		case (arg::POINTER):
			arg_store.push_back((void *) v.u64);
			break;
		case (arg::INT32):
			arg_store.push_back((int32_t) v.u64);
			break;
		case (arg::UINT32):
			arg_store.push_back((uint32_t) v.u64);
			break;
		case (arg::INT64):
			arg_store.push_back(v.i64);
			break;
		case (arg::UINT64):
			arg_store.push_back(v.u64);
			break;
		case (arg::DOUBLE):
			arg_store.push_back(v.dbl);
			break;
		case (arg::XDUMP):
			arg_store.push_back("xdump not supported with format strings");
			break;
		case (arg::RAW):
			arg_store.push_back("rawdata data not supported with format strings");
			break;
		default:
			arg_store.push_back(v.u64);
			break;
		}
	}

	try {
		ctx(std::back_inserter(sb), fmt::to_string_view(d.arg_str[d.next_arg]), arg_store).format<arg_fmt>();
	} catch(fmt::format_error& fe) {
		sb.push_back(fe.what());
	}

	sb.push_back('\n');
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

	sb.push_back(str, i);
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

	sb.push_back(str, i);
}

void format_basic::flexi_header(ostrbuf& sb, record_data& d)
{
	const hogl::record &r = *d.record;

	if (_fields & TIMESPEC) {
		_tscache.update(r.timestamp);
		sb.push_back(_tscache.str(), _tscache.len());
		sb.push_back(' ');
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


	switch (t0) {
	case arg::RAW:
		return output_raw(sb, rd);

	case arg::CSTR:
	case arg::GSTR:
		if (t1 != arg::NONE && t1 != arg::XDUMP && t1 != arg::RAW)
			return output_fmt(sb, rd);
	default:
		return output_plain(sb, rd);
	}
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

