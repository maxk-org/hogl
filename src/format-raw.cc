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

#include "hogl/area.hpp"
#include "hogl/format-raw.hpp"

namespace hogl {

class raw_packer {
private:
	ostrbuf &_sb;

	template <typename T>
	void add_uint(T v)
	{
		_sb.put((uint8_t *) &v, sizeof(T));
	}

	template <typename T>
	void add_blob(const void* data, T len)
	{
		_sb.put((uint8_t *) &len, sizeof(len));
		_sb.put((uint8_t *) data, len);
	}

	template <typename T>
	void add_str(const char *str)
	{
		add_blob<T>(str, strlen(str));
	}

	void add_data(const uint8_t *data, unsigned int len)
	{
		add_blob<uint16_t>(data, len);
	}

	void add_args(const record &r)
	{
		add_uint<uint64_t>(r.argtype);
		unsigned int i;
		for (i=0; i < record::NARGS; i++) {
			unsigned int type = r.get_arg_type(i);
			unsigned int len; const uint8_t *data;

			if (type == arg::NONE)
				return;

			switch (type) {
			case arg::CSTR:
			case arg::HEXDUMP:
			case arg::RAW:
				data = r.get_arg_data(i, len);
				add_data(data, len);
				break;
			case arg::GSTR:
				if (arg::is_32bit(arg::GSTR))
					add_str<uint16_t>((const char *) (unsigned long) r.get_arg_val32(i));
				else
					add_str<uint16_t>((const char *) r.get_arg_val64(i));
				break;
			case arg::INT32:
			case arg::UINT32:
				add_uint<uint32_t>(r.get_arg_val32(i));
				break;
			default:
				add_uint<uint64_t>(r.get_arg_val64(i));
				break;
			}
		}
	}

public:
	raw_packer(ostrbuf &sb) : _sb(sb) {}

	void pack(const format::data &d)
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

		add_uint<uint64_t>(r.timestamp);
		add_uint<uint64_t>(r.seqnum);
		add_str<uint8_t>(ring_name);
		add_str<uint8_t>(area_name);
		add_str<uint8_t>(sect_name);
		add_args(r);
	}
};

format_raw::format_raw()
{ }

void format_raw::process(ostrbuf &sb, const format::data &d)
{
	raw_packer rp(sb);
	rp.pack(d);
}

} // namespace hogl
