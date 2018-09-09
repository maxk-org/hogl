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

#ifndef PROJECT_CONFIG_BUILDER_HPP
#define PROJECT_CONFIG_BUILDER_HPP

#include <utility> // std::move

#include "hogl/config.hpp"

namespace hogl {

    class output_builder;

    class config_builder_base {
    public:
        explicit config_builder_base(config& cfg) : _config(cfg) {}

        operator config() const {
            return std::move(_config);
        }

        // builder facets here
        output_builder output() const;
        /*
        format_builder format() const;
        mask_builder mask() const;
        area_builder area() const;
        engine_options_builder engine_options() const;
        ringbuf_options_builder ringbuf_options() const;
         */
    // protected so derived classes (specific builders)
    // can access it when then need to call _config.xyz = something
    protected:
        config& _config;
    };

    class config_builder final : private config_builder_base {
    public:
        explicit config_builder() : config_builder_base{_config} {}
    private:
        // can create it since config_builder is a friend of config
        // config's default ctor is private
        config _config;
    };

    class output_builder final : public config_builder_base {
        using self = output_builder;
    public:
        explicit output_builder(config &cfg) : config_builder_base{cfg} {}

        self& stdout() {
            _config._log_output = config::outputs::STDOUT;
            return *this;
        }
        self& stderr() {
            _config._log_output = config::outputs::STDERR;
            return *this;
        }
        self& pipe() {
            _config._log_output = config::outputs::PIPE;
            return *this;
        }
        self& textfile() {
            _config._log_output = config::outputs::TEXT_FILE;
            return *this;
        }
    };

} // ns hogl

#endif //PROJECT_CONFIG_BUILDER_HPP
