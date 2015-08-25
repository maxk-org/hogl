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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/detail/area.hpp"
#include "hogl/detail/mask.hpp"
#include "hogl/detail/format.hpp"
#include "hogl/detail/output.hpp"
#include "hogl/detail/engine.hpp"

#include "hogl/c-api/area.h"
#include "hogl/c-api/args.h"
#include "hogl/c-api/mask.h"
#include "hogl/c-api/format.h"
#include "hogl/c-api/output.h"
#include "hogl/c-api/engine.h"
#include "hogl/c-api/tls.h"

#define BOOST_TEST_MODULE c_api_test 
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(args)
{
	// Make sure argument types match
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_NONE   ==  (unsigned int) hogl::arg::NONE  ); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_UINT32 ==  (unsigned int) hogl::arg::UINT32); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_INT32  ==  (unsigned int) hogl::arg::INT32 ); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_UINT64 ==  (unsigned int) hogl::arg::UINT64); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_INT64  ==  (unsigned int) hogl::arg::INT64 ); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_DOUBLE ==  (unsigned int) hogl::arg::DOUBLE);
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_CSTR   ==  (unsigned int) hogl::arg::CSTR  ); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_GSTR   ==  (unsigned int) hogl::arg::GSTR  ); 
	BOOST_REQUIRE( (unsigned int) HOGL_ARG_POINTER==  (unsigned int) hogl::arg::POINTER);

	// Make sure arg layout is the same
	struct hogl_arg carg;
	carg.type = 0x11111111;
	carg.val  = 0x22222222;	
	carg.len  = 0x33333333;	

	hogl::arg *xarg = (hogl::arg *) &carg;
	BOOST_REQUIRE(xarg->type == 0x11111111);
	BOOST_REQUIRE(xarg->val  == 0x22222222);
	BOOST_REQUIRE(xarg->len  == 0x33333333);
}
