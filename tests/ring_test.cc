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

#include "hogl/detail/ringbuf.hpp"
#include "hogl/engine.hpp"
#include "hogl/timesource.hpp"
#include "hogl/post.hpp"

#define BOOST_TEST_MODULE ring_test 
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

BOOST_AUTO_TEST_CASE(basic)
{
	hogl::ringbuf::options opts = {
			.capacity = 1024,
			.prio = 123,
			.flags = hogl::ringbuf::REUSABLE,
			.record_tailroom = 128
		};

	hogl::ringbuf ring("DUMMY", opts);

	BOOST_REQUIRE (ring.name() == std::string("DUMMY"));
	BOOST_REQUIRE (ring.capacity() == 1024);
	BOOST_REQUIRE (ring.prio() == 123);
	BOOST_REQUIRE (ring.reusable() == true);

	BOOST_REQUIRE (hogl::default_ring.name() == std::string("DEFAULT"));
	BOOST_REQUIRE (hogl::default_ring.prio() == 0);
	BOOST_REQUIRE (hogl::default_ring.shared() == true);
}

BOOST_AUTO_TEST_CASE(ring_ops)
{
	hogl::ringbuf::options opts = { };
	opts.capacity = 64;

	hogl::ringbuf *ring = new hogl::ringbuf("DUMMY", opts);
	ring->hold();

	ring->timesource(&hogl::default_timesource);

	BOOST_REQUIRE (ring->empty() == true);

	unsigned int i;
	for (i = 0; i < 100; i++)
		hogl::push_unlocked(ring, 0, 1, "push #%u", i);

	std::cout << *ring;

	BOOST_REQUIRE (ring->size() == 63);
	BOOST_REQUIRE (ring->room() == 0);
	BOOST_REQUIRE (ring->dropcnt() == (100 - 63));
	BOOST_REQUIRE (ring->seqnum() == 100);
	BOOST_REQUIRE (ring->empty() != true);

	ring->reset();

	ring->release();
}

void *run_blocking_thread(void *priv)
{
	hogl::ringbuf *ring = (hogl::ringbuf *) priv;
	for (unsigned int i = 0; i < 100; i++)
		hogl::push(ring, 0, 1, "push #%u", i);
	return 0;
}

BOOST_AUTO_TEST_CASE(ring_block)
{
	hogl::ringbuf::options opts = { };
	opts.capacity = 64;
	opts.flags = hogl::ringbuf::BLOCKING;

	hogl::ringbuf *ring = new hogl::ringbuf("DUMMY", opts);
	ring->hold();

	BOOST_REQUIRE (ring->empty() == true);

	pthread_t t;
	if (pthread_create(&t, 0, run_blocking_thread, ring) < 0) {
		perror("Failed to create test thread");
		exit(1);
	}

	while (ring->room() != 0) 
		usleep(100);

	// At this point the test thread will have blocked

	std::cout << *ring;

	BOOST_REQUIRE (ring->size() == 63);
	BOOST_REQUIRE (ring->dropcnt() == 0);
	BOOST_REQUIRE (ring->seqnum() == 64);
	BOOST_REQUIRE (ring->empty() != true);

	// Call unblock without poping records (blocked thread must retry)
	for (unsigned int i = 0; i < 100; i++)
		ring->unblock();

	// Pop records and unblock
	unsigned int n = 0;
	while (n != 100) {
		hogl::record *r = ring->pop_begin();
		if (!r)
			continue;
		ring->pop_commit();
		ring->unblock();
		n++;
	}

	pthread_join(t, NULL);

	BOOST_REQUIRE (ring->dropcnt() == 0);
	BOOST_REQUIRE (ring->seqnum() == 100);

	ring->release();
}

#pragma GCC diagnostic ignored "-Wfree-nonheap-object"

BOOST_AUTO_TEST_CASE(immortal_ring)
{
	hogl::ringbuf::options opts = { };
	opts.capacity = 64;
	opts.flags    = hogl::ringbuf::IMMORTAL;

	hogl::ringbuf ring("DUMMY", opts);

	ring.hold();
	ring.release();
}
