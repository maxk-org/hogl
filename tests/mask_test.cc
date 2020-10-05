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

#define _XOPEN_SOURCE 700

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/detail/area.hpp"
#include "hogl/detail/mask.hpp"

#define BOOST_TEST_MODULE mask_test 
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

	hogl::mask mask;
	mask << ".*:.*";

	BOOST_REQUIRE (area.name() == std::string("DEF"));
	BOOST_REQUIRE (area.count() == NSECT);

	hogl::mask empty;

	hogl::mask copy(mask);

	empty = mask;
}

BOOST_AUTO_TEST_CASE(mask_ops)
{
	unsigned int i;

	hogl::area area("DEF", sect_names);

	hogl::mask mask;

	mask << "";
	mask << ":";
	mask << "!:";
	mask << "!:";
	mask << "A:S";
	mask << "!A:S";
	mask << "!:S";
	mask << "!A:";
	mask << "!A";

	std::cout << mask;

	// Disable all
	mask.clear();
	mask << "!.*:.*";
	mask.apply(area);
	for (i=0; i < area.size(); i++) 
		BOOST_REQUIRE(area.test(i) == false);

	std::cout << "---" << std::endl;
	std::cout << mask;
	std::cout << std::setw(4) << area;

	// Enable all
	mask.clear();
	mask << ".*:.*";
	mask.apply(area);
	for (i=0; i < area.size(); i++) 
		BOOST_REQUIRE(area.test(i) == true);

	std::cout << "---" << std::endl;
	std::cout << mask;
	std::cout << std::setw(4) << area;

	// Disable all but INFO
	mask.clear();
	mask << "!.*:.*" << ".*:.*INFO";
	mask.apply(area);
	BOOST_REQUIRE(area.test(INFO) == true);
	BOOST_REQUIRE(area.test(EXTRA_INFO) == true);

	std::cout << "---" << std::endl;
	std::cout << mask;
	std::cout << std::setw(4) << area;

	// Enable all but DEBUG
	mask.clear();
	mask << ".*:.*" << "!.*:.*DEBUG";
	mask.apply(area);
	BOOST_REQUIRE(area.test(INFO) == true);
	BOOST_REQUIRE(area.test(EXTRA_INFO) == true);
	BOOST_REQUIRE(area.test(DEBUG) == false);
	BOOST_REQUIRE(area.test(EXTRA_DEBUG) == false);

	std::cout << "---" << std::endl;
	std::cout << mask;
	std::cout << std::setw(4) << area;

	// Enable all but EXTRA:... 
	mask.clear();
	mask << ".*:.*" << "!.*:EXTRA.*";
	mask.apply(area);
	BOOST_REQUIRE(area.test(DEBUG) == true);
	BOOST_REQUIRE(area.test(EXTRA_DEBUG) == false);

	std::cout << "---" << std::endl;
	std::cout << mask;
	std::cout << std::setw(4) << area;
}

BOOST_AUTO_TEST_CASE(twice)
{
	hogl::mask m1("XYZ:DEBUG", "!HOGL:.*DEBUG", NULL);
	hogl::mask m2("XYZ:DEBUG", "ABC:DEBUG", "SOME_AREA:DEBUG", "OTHER:DEBUG", "!.*:DEBUG", "");
}
