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

#include "hogl/detail/area.hpp"

#define BOOST_TEST_MODULE area_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

enum sect_id {
	DEBUG,
	INFO,
	ERROR,
	EXTRA_DEBUG,
	EXTRA_INFO,
	EXTRA_ERROR,
	NSECT
};

static const char *sect_names[] = {
	"DEBUG",
	"INFO",
	"ERROR",
	"EXTRA:DEBUG",
	"EXTRA:INFO",
	"EXTRA:ERROR",
	0,
};

BOOST_AUTO_TEST_CASE(basic)
{
	hogl::area area("DEF", sect_names);

	BOOST_REQUIRE (area.name() == std::string("DEF"));
	BOOST_REQUIRE (area.count() == NSECT);
}

BOOST_AUTO_TEST_CASE(mask_ops)
{
	unsigned int i;

	hogl::area area("DEF", sect_names);

	// Disable all
	area.reset();
	for (i=0; i < area.size(); i++) 
		BOOST_REQUIRE(area.test(i) == false);

	std::cout << "---" << std::endl;
	std::cout << area;

	// Enable all
	area.set();
	for (i=0; i < area.size(); i++) 
		BOOST_REQUIRE(area.test(i) == true);

	std::cout << "---" << std::endl;
	std::cout << area;

	// Disable all but INFO
	area.reset();
	area.set(INFO);
	area.enable(EXTRA_INFO);
	BOOST_REQUIRE(area.test(INFO) == true);
	BOOST_REQUIRE(area.test(EXTRA_INFO) == true);

	std::cout << "---" << std::endl;
	std::cout << area;

	// Enable all but DEBUG and EXTRA_INFO
	area.set();
	area.reset(DEBUG);
	area.disable(EXTRA_INFO);
	BOOST_REQUIRE(area.test(DEBUG) == false);
	BOOST_REQUIRE(area.test(EXTRA_DEBUG) == true);
	BOOST_REQUIRE(area.test(EXTRA_INFO) == false);

	std::cout << "---" << std::endl;
	std::cout << area;
}

BOOST_AUTO_TEST_CASE(compare)
{
	// Compare areas
	const char *a0_sect[] = { "X", "Y", "Z", 0 };
	const char *a1_sect[] = { "A", "B", "C", 0 };
	const char *a2_sect[] = { "X", "Y", "Z", 0 };
	const char *a3_sect[] = { "X", "Y", "Z", "A", 0 };

	hogl::area a0("XYZ", a0_sect);
	hogl::area a1("XYZ", a1_sect);
	hogl::area a2("XYZ", a2_sect);
	hogl::area a3("XYZ", a3_sect);

	BOOST_REQUIRE(a0 != a1);
	BOOST_REQUIRE(a0 == a2);
	BOOST_REQUIRE(a0 != a3);
	BOOST_REQUIRE(a1 != a3);
}

BOOST_AUTO_TEST_CASE(default_sections)
{
	hogl::area d("DEFSECT");

	BOOST_REQUIRE(d.size() == 6);

	std::cout << "---" << std::endl;
	std::cout << d;
}
