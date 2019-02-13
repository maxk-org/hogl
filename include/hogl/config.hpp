/*
   Copyright (c) 2018, Max Krasnyansky <max.krasnyansky@gmail.com>
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

#ifndef PROJECT_CONFIG_HPP
#define PROJECT_CONFIG_HPP

#include <string>

#include "hogl/detail/ringbuf.hpp"
#include "hogl/detail/engine.hpp"

namespace hogl {

    class config_builder;

    class config final {
    public:
        // need this so config_builder can aggr/hold config instance
        // so it needs access to private ctor
        friend class config_builder;
        // all specific builders would directly access the below members
        friend class output_builder;
        friend class format_builder;

        // creation static function
        static config_builder create();
    private:
        enum struct formats : uint8_t {
            RAW,
            BASIC
        };
        enum struct outputs : uint8_t  {
            STDOUT,
            STDERR,
            PIPE,
            TEXT_FILE
        };
        config() = default;
        static constexpr uint32_t    _output_bufsize = 10 * 1024 * 1024;
        formats     _log_format = formats::BASIC;
        outputs     _log_output = outputs::STDOUT;
        hogl::ringbuf::options _ring_options;
        hogl::engine::options  _mask_options;
    };

} // ns hogl

#endif //PROJECT_CONFIG_HPP
