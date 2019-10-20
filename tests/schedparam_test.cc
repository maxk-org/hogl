/*
   Copyright (c) 2019, Max Krasnyansky <max.krasnyansky@gmail.com> 
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

#include "hogl/detail/schedparam.hpp"

#define BOOST_TEST_MODULE schedparam_test
#include <boost/test/included/unit_test.hpp>

__HOGL_PRIV_NS_USING__;

BOOST_AUTO_TEST_CASE(basic_other)
{
	printf("basic sched_other test\n");
	const hogl::schedparam sp;
	BOOST_ASSERT(sp.policy == 0);
	BOOST_ASSERT(sp.priority == 0);
	BOOST_ASSERT(sp.cpu_affinity.empty());
	BOOST_ASSERT(sp.apply("test"));
}

BOOST_AUTO_TEST_CASE(basic_fifo)
{
	printf("basic sched_fifo test\n");
	const hogl::schedparam sp(SCHED_FIFO, 50);
	BOOST_ASSERT(sp.policy == SCHED_FIFO);
	BOOST_ASSERT(sp.priority == 50);
	BOOST_ASSERT(sp.cpu_affinity.empty());
	BOOST_ASSERT(!sp.apply("test"));
}

BOOST_AUTO_TEST_CASE(validation_good)
{
	// Good params and copy
	hogl::schedparam sp(0,0,"0x3");
	BOOST_ASSERT(sp.validate());

	sp = hogl::schedparam(0,0, "list:0-3,10-13");
	BOOST_ASSERT(sp.validate());

	sp = hogl::schedparam(SCHED_RR, 99, "list:1-3,10-13");
	BOOST_ASSERT(sp.policy == SCHED_RR);
	BOOST_ASSERT(sp.priority == 99);
	BOOST_ASSERT(sp.validate());
	BOOST_ASSERT(!sp.apply("test"));
}

BOOST_AUTO_TEST_CASE(validation_bad)
{
	// Bad params
	const hogl::schedparam sp0(0,0,"list:12345"); // crazy CPU number
	BOOST_ASSERT(!sp0.validate());
}
