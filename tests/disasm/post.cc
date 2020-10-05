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

#include "hogl/detail/area.hpp"
#include "hogl/post.hpp"

__HOGL_PRIV_NS_USING__;

extern hogl::area *test_area;

void __attribute__((noinline)) post_cstr_no_args()
{
	hogl::post(test_area, 0xe1e, "cstr");
}

void __attribute__((noinline)) post_gstr_no_args()
{
	hogl::post(test_area, 0xe1e, hogl::arg_gstr("gstr"));
}

void __attribute__((noinline)) post_unlocked_gstr_no_args()
{
	hogl::post_unlocked(test_area, 0xe1e, hogl::arg_gstr("gstr"));
}

void __attribute__((noinline)) post_tls_gstr_no_args()
{
	hogl::post(hogl::tls::ring(), test_area, 0xe1e, hogl::arg_gstr("gstr"));
}

void __attribute__((noinline)) post_gstr_2x()
{
	hogl::post(test_area, 0xe1e, hogl::arg_gstr("gstr1"));
	hogl::post(test_area, 0xe1f, hogl::arg_gstr("gstr2"));
}

void __attribute__((noinline)) printf_cstr_3_uints(hogl::ringbuf *ring)
{
	// The call includes ring, area, and section for apples-2-apples comparison
        printf("gstr %p %p %u %u %u %u", ring, test_area, 0xe1e,
			0x1f1f1f, 0x2f2f2f, 0x3f3f3f);
}

void __attribute__((noinline)) push_cstr_3_uints(hogl::ringbuf *ring)
{
	hogl::push(ring, test_area, 0xe1e, "cstr %u %u %u",
			0x1f1f1f, 0x2f2f2f, 0x3f3f3f);
}

void __attribute__((noinline)) printf_cstr_3_uints_vp(hogl::ringbuf *ring, unsigned int *a, unsigned int *b, unsigned int *c)
{
	// The call includes ring, area, and section for apples-2-apples comparison
        printf("gstr %p %p %u %u %u %u", ring, test_area, 0xe1e, *a, *b, *c);
}

void __attribute__((noinline)) push_cstr_3_uints_vp(hogl::ringbuf *ring, unsigned int *a, unsigned int *b, unsigned int *c)
{
	hogl::push(ring, test_area, 0xe1e, "cstr %u %u %u", *a, *b, *c);
}

void __attribute__((noinline)) post_cstr_3_uints()
{
	hogl::post(test_area, 0xe1e, "cstr %u %u %u",
			0x1f1f1f, 0x2f2f2f, 0x3f3f3f);
}

void __attribute__((noinline)) post_gstr_3_uints()
{
	hogl::post(test_area, 0xe1e, hogl::arg_gstr("gstr %u %u %u"),
			0x1f1f1f, 0x2f2f2f, 0x3f3f3f);
}

void __attribute__((noinline)) post_gstr_8_uints()
{
	hogl::post(test_area, 0xe1e, hogl::arg_gstr("fmt %u %u %u"), 
			0x1f1f1f, 0x2f2f2f, 0x3f3f3f, 0x4f4f4f4f4f4f4f4fUL, 
			0x5f5f5f, 0x6f6f6f6f, 0x7f7f7f7f7);
}

void __attribute__((noinline)) post_gstr_9_uints()
{
	hogl::post(test_area, 0xe1e, hogl::arg_gstr("fmt %u %u %u"),
			0x1f1f1f, 0x2f2f2f, 0x3f3f3f, 0x4f4f4f4f4f4f4f4fUL,
			0x5f5f5f, 0x6f6f6f6f, 0x7f7f7f7f7, 0x8f8f8f8f);
}

void __attribute__((noinline)) post_gstr_14_uints()
{
	hogl::post(test_area, 0xe1e, hogl::arg_gstr("fmt %u %u %u"),
			0x1111111, 0x22222222, 0x33333333, 0x44444444,
			0x5555555, 0x66666666, 0x77777777, 0x88888888,
			0x9999999, 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc,
			0xddddddd, 0xeeeeeeee);
}

void __attribute__((noinline)) post_cstr_ext_cstr(const char *arg)
{
	hogl::post(test_area, 0xe1e, "cstr [%s]", arg);
}

void __attribute__((noinline)) post_cstr_cstr()
{
	hogl::post(test_area, 0xe1e, "cstr [%s]", "local arg");
}

static uint8_t data[256];
void __attribute__((noinline)) post_hexdump()
{
	hogl::post_unlocked(test_area, 1, hogl::arg_xdump(data, 128, 'H', 1, 0, 40));
}

static bool post_init;
void init_post(hogl::record *)
{
        post_init = true;
}

