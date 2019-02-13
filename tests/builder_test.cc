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

#include <memory>

#include "hogl/post.hpp"

#include "hogl/config-builder.hpp"

enum test_sect_id {
    TEST_DEBUG,
    TEST_INFO,
    TEST_WARN,
    TEST_ERROR,
    TEST_EXTRA_DEBUG,
    TEST_EXTRA_INFO,
    TEST_TRACE,
    TEST_HEXDUMP,
};

static const char *test_sect_names[] = {
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "EXTRA:DEBUG",
        "EXTRA:INFO",
        "TRACE",
        "HEXDUMP",
        0,
};

int main(int argc, char *argv[])
{
    std::string log_output("stdout");
    std::string log_format("fast1");
    constexpr unsigned int output_bufsize = 10 * 1024 * 1024;
    hogl::area *test_area = nullptr;

    hogl::config config = hogl::config::create()
            .format().basic()
            .output().stdout()
            .mask().set(".*", "DEBUG:.*")
            .activate()
            .area().add(test_area, "TEST-AREA", test_sect_names);

    std::clog << config.mask() << std::endl;
    config.apply_mask();
    assert(test_area);

    hogl::post(test_area, TEST_INFO, "some string\n%s", "hello world");

    hogl::deactivate();

    printf("Passed\n");
    return 0;
}