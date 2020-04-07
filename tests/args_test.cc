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

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/detail/args.hpp"

#define BOOST_TEST_MODULE area_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

BOOST_AUTO_TEST_CASE(basic)
{
	hogl::arg a_cstr("string");
	hogl::arg a_gstr(hogl::arg_gstr("string"));
	hogl::arg a_uint((unsigned int) 10);
	hogl::arg a_int((int) 10);
	hogl::arg a_dbl(20.1234);
	hogl::arg a_ptr(&a_cstr);

	BOOST_REQUIRE(a_cstr.type == hogl::arg::CSTR);
	BOOST_REQUIRE(a_cstr.len  == strlen("string"));
	BOOST_REQUIRE(a_gstr.type == hogl::arg::GSTR);

	BOOST_REQUIRE(a_uint.type == hogl::arg::UINT32);
	BOOST_REQUIRE(a_int.type  == hogl::arg::INT32);
	BOOST_REQUIRE(a_dbl.type  == hogl::arg::DOUBLE);
	BOOST_REQUIRE(a_ptr.type  == hogl::arg::POINTER);
}
