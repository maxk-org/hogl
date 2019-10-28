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

#include "hogl/detail/record.hpp"
#include "hogl/detail/area.hpp"

__HOGL_PRIV_NS_USING__;

extern hogl::area *test_area;

void __attribute__((noinline)) record_3_ints(hogl::record *r)
{
	r->set_args(64,	1, 0x1f1f1f, -125);
}

void __attribute__((noinline)) record_1_double(hogl::record *r, double *)
{
	r->set_args(64,	1.1);
}

void __attribute__((noinline)) record_gstr(hogl::record *r)
{
	r->set_args(64,	hogl::arg_gstr("gstr no args"));
}

void __attribute__((noinline)) record_local_cstr_short(hogl::record *r)
{
	r->set_args(64,	"cstr short");
}

void __attribute__((noinline)) record_set_local_cstr_long(hogl::record *r)
{
	r->set_args(64,	"cstr long string that does not fit into a register");
}

void __attribute__((noinline)) record_set_ext_cstr(hogl::record *r, const char *str)
{
	r->set_args(64,	str);
}

void __attribute__((noinline)) record_gstr_3_uint_args(hogl::record *r)
{
	r->set_args(64,	hogl::arg_gstr("fmt %u %u %u"),
					0x1f1f1f, 0x2f2f2f, 0x3f3f3f);
}

void __attribute__((noinline)) record_gstr_8_args(hogl::record *r)
{
	r->set_args(128,	hogl::arg_gstr("fmt %u %u %u %u %u %u"),
				0x1f1f1f, 0x2f2f2f, 0x3f3f3f, 0x4f4f4f4f4f4f4f4fUL,
				0x5f5f5f, 0x6f6f6f6f, 0x7f7f7f7f7);
}

void __attribute__((noinline)) record_local_cstr_3_args(hogl::record *r)
{
	r->set_args(64, 	"fmt %u %u %u", 
				0x1f1f1f, 0x2f2f2f, 0x3f3f3f);
}

void __attribute__((noinline)) record_local_cstr_14_args(hogl::record *r)
{
	r->set_args(256, 	"fmt %u %u %u %u %u %u",
				0x1111111, 0x2222222, 0x33333333, 0x4444444, 0x55555555, 
				0x6666666, 0x7777777, 0x88888888, 0x9999999, 0xaaaaaaaa, 
				0xbbbbbbb, 0xccccccc, 0xdddddddd);
}

static uint8_t data[128] = {};

void __attribute__((noinline)) record_hexdump(hogl::record *r)
{
	r->set_args(256, hogl::arg_xdump(data, sizeof(data)));
}

static bool record_init;
void init_record(hogl::record *)
{
        record_init = true;
}
