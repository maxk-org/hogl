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

#define _XOPEN_SOURCE 700

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#include "hogl/detail/tsobuf.hpp"

#include <boost/format.hpp>

#define BOOST_TEST_MODULE tso_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

BOOST_AUTO_TEST_CASE(basic)
{
	hogl::tsobuf tso(1024);

	BOOST_REQUIRE (tso.capacity() == 1024);
	BOOST_REQUIRE (tso.size() == 0);
}

BOOST_AUTO_TEST_CASE(tso_ops)
{
	hogl::tsobuf tso(1024);

	hogl::record rec[1024];

	// Init records with descending timestamps
	// and push them into the tso
	unsigned int i;
	for (i=0; i < 1024; i++) {
		rec[i].timestamp = 1024 - i;

		hogl::tsobuf::entry te;
		te.tag = 0;
		te.rec = &rec[i];
		te.timestamp = rec[i].timestamp;
		tso.push(te);
	}

	tso.sort();

	BOOST_REQUIRE (tso.full() == true);
	BOOST_REQUIRE (tso.size() == 1024);

	// Verify that records come out in the ascending 
	// order
	hogl::timestamp last;

	hogl::tsobuf::entry te = {};
	tso.pop(te);

	last = te.rec->timestamp;
	while (tso.pop(te))
		BOOST_REQUIRE(te.rec->timestamp == ++last);

	BOOST_REQUIRE(te.rec->timestamp == hogl::timestamp(1024));
}

BOOST_AUTO_TEST_CASE(weird_sort)
{
	unsigned int i;

	hogl::tsobuf tso(1024);
	hogl::record rec_a[128];
	hogl::record rec_b[128];

	hogl::tsobuf::entry te;

	te.tag = 0;
	for (i=0; i < 100; i++) {
		rec_a[i].timestamp = i;
		rec_a[i].seqnum    = i;
		te.rec = &rec_a[i];
		te.timestamp = i;
		tso.push(te);
	}

	rec_a[i].timestamp = 10;
	rec_a[i].seqnum    = i;
	te.rec = &rec_a[i];
	te.timestamp = 10;
	tso.push(te);

	te.tag = 1;
	for (i=0; i < 100; i++) {
		rec_b[i].timestamp = i+1;
		rec_b[i].seqnum    = i;
		te.rec = &rec_b[i];
		te.timestamp = i+1;
		tso.push(te);
	}

	rec_b[i].timestamp = 50;
	rec_b[i].seqnum    = i;
	te.rec = &rec_b[i];
	te.timestamp = 51;
	tso.push(te);

	tso.sort();
	while (tso.pop(te)) {
		// FIXME: validate expected order
		// std::cout << boost::format("ts %u seq %u tag: %u")
		// 		% te.rec->timestamp
		// 		% te.rec->seqnum
		// 		% te.tag
		// 	<< std::endl;
	}
}
