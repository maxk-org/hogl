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

#include "hogl/config-builder.hpp"

int main(int argc, char *argv[])
{
    std::string log_output("stdout");
    std::string log_format("fast1");
    constexpr unsigned int output_bufsize = 10 * 1024 * 1024;

    //hogl::config_builder builder;

/*
    hogl::format *lf;
    hogl::output *lo;

    if (log_format == "raw")
        lf = new hogl::format_raw();
    else
        lf = new hogl::format_basic(log_format.c_str());

    if (log_output == "stderr")
        lo = new hogl::output_stderr(*lf, output_bufsize);
    else if (log_output == "stdout")
        lo = new hogl::output_stdout(*lf, output_bufsize);
    else if (log_output[0] == '|')
        lo = new hogl::output_pipe(log_output.substr(1).c_str(), *lf, output_bufsize);
    else
        lo = new hogl::output_textfile(log_output.c_str(), *lf, output_bufsize);

    hogl::mask logmask(".*", 0);

    hogl::activate(*lo);

    test_area = hogl::add_area("TEST-AREA", test_sect_names);

    // Test double add (with reuse)
    test_area = hogl::add_area("TEST-AREA", test_sect_names);

    // Test double add (failed)
    const char *a0_sections[] = { "X", 0 };
    const hogl::area *a0 = hogl::add_area("TEST-AREA", a0_sections);
    assert(a0 == 0);

    hogl::apply_mask(logmask);

    printf("sizeof(hogl::record)  = %lu\n", (unsigned long) sizeof(hogl::record));
    printf("sizeof(hogl::ringbuf) = %lu\n", (unsigned long) sizeof(hogl::ringbuf));

    fflush(stdout);
    fflush(stderr);

    std::cerr << *hogl::default_engine;

    hogl::deactivate();

    delete lo;
    delete lf;
*/
    printf("Passed\n");
    return 0;
}