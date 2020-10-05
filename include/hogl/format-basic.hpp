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

/**
 * @file hogl/format-basic.h
 * Basic format handler.
 */
#ifndef HOGL_FORMAT_BASIC_HPP
#define HOGL_FORMAT_BASIC_HPP

#include <hogl/detail/format.hpp>
#include <hogl/detail/ostrbuf.hpp>
#include <hogl/detail/timestamp.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Basic format handler.
 * Simple and efficient handler for generating text output.
 */
class format_basic : public format {
public:
	enum Fields {
		TIMESPEC  = (1<<0),
		TIMESTAMP = (1<<1),
		TIMEDELTA = (1<<2),
		RING      = (1<<3),
		SEQNUM    = (1<<4),
		AREA      = (1<<5),
		SECTION   = (1<<6),
		DEFAULT = (TIMESTAMP | RING | SEQNUM | AREA | SECTION),
	        FAST0   = (TIMESPEC | RING | SEQNUM | AREA | SECTION),
	        FAST1   = (TIMESPEC | TIMEDELTA | RING | SEQNUM | AREA | SECTION)
	};

	explicit format_basic(uint32_t fields = DEFAULT);
	explicit format_basic(const char *fields);
	explicit format_basic(const std::string &fields) : format_basic(fields.c_str()) {}

	virtual void process(ostrbuf &s, const format::data &d);

	// Timespec cache
	struct tscache {
		enum {
			NSEC_SPLIT  = 10000,
			NSEC_HI_LEN = 5,
			NSEC_LEN    = 9,
			NSEC_LO_LEN = NSEC_LEN - NSEC_HI_LEN,
			SEC_LEN     = 11,
			TS_LEN      = SEC_LEN + 1 + NSEC_LEN,
			NSEC_HI_OFFSET = SEC_LEN + 1,
			NSEC_LO_OFFSET = NSEC_HI_OFFSET + NSEC_HI_LEN
		};

		time_t   _sec;
		uint32_t _nsec_hi;
		uint8_t  _str[TS_LEN];

		tscache(): _sec(0), _nsec_hi(0)
		{
			for (unsigned i=0; i<TS_LEN; i++) _str[i]='0';
			_str[SEC_LEN] = '.';
		}

		void update(hogl::timestamp t);
		const char *str() const  { return (const char *) _str; }
		unsigned int len() const { return TS_LEN; }
	};

	// Expanded record data
	struct record_data {
		const hogl::record* record;
		const char*  area_name;
		const char*  sect_name;
		const char*  ring_name;
		const char*  arg_str[record::NARGS];
		unsigned int next_arg;
	};

protected:
	uint32_t  _fields;
	timestamp _last_timestamp;
	tscache   _tscache;

	virtual void output_plain(hogl::ostrbuf& sb, record_data& d);
	virtual void output_raw(hogl::ostrbuf& sb, record_data& d);
	virtual void output_fmt(hogl::ostrbuf& sb, record_data& d);

	void default_header(ostrbuf& sb, record_data& d);
	void flexi_header(ostrbuf& sb, record_data& d);
	void fast0_header(ostrbuf& sb, record_data& d);
	void fast1_header(ostrbuf& sb, record_data& d);
};

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_FORMAT_BASIC_HPP
