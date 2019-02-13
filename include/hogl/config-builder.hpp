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

#include "hogl/format-basic.hpp"
#include "hogl/format-raw.hpp"

#include "hogl/output-stdout.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/output-textfile.hpp"
#include "hogl/output-pipe.hpp"

#include "hogl/config.hpp"

namespace hogl {

    // fwd decl needed for config_builder_base accessors
    class output_builder;
    class format_builder;
    class mask_builder;

    /**
     * @class config_builder_base just holds a reference to the actual config instance
     * and provide:
     * 1. accessors to all the builders - returning each by value
     * 2. cast operator to the actual config
     */
    class config_builder_base {
    public:
        explicit config_builder_base(config& cfg) : _config(cfg) {}

        operator config() const {
            return std::move(_config);
        }

        // builder facets here
        output_builder output() const;
        format_builder format() const;
        mask_builder mask() const;

        /*
        area_builder area() const;
        engine_options_builder engine_options() const;
        ringbuf_options_builder ringbuf_options() const;
         */
    // protected so derived classes (specific builders)
    // can access it when then need to call _config.xyz = something
    protected:
        config& _config;
    };

    /**
     * @class config_builder is actually holding the config instance
     * and since it is inheritting from base, once you have an instance
     * of config_builder, you can access each builder individually and invoke functions
     */
    class config_builder final : public config_builder_base {
    public:
        explicit config_builder() : config_builder_base{_c} {}
    private:
        // can create it since config_builder is a friend of config
        // config's default ctor is private
        config _c;
    };

    /**
     * Individual builders
     * each builder inherits from base and instantiate base with the config
     * When we call config::create() we create a config_builder instance. It holds a config instance
     * Now, when we chain calls like .output() or .format() (defined in base)
     * then we create an instance of the builder and pass it a reference of the actual config instance
     * this specific builder will work directly on the config instance. builders are friends in config class
     */
    class output_builder final : public config_builder_base {
        using self = output_builder;
    public:
        explicit output_builder(config &cfg) : config_builder_base{cfg} {}

        self& stdout(uint32_t output_bufsize = 0) {
            _config._log_output.reset(new hogl::output_stdout(*_config._log_format,
                    _config.get_output_bufsize(output_bufsize)));
            return *this;
        }
        self& stderr(uint32_t output_bufsize = 0) {
            _config._log_output.reset(new hogl::output_stderr(*_config._log_format,
                    _config.get_output_bufsize(output_bufsize)));
            return *this;
        }
        self& pipe(const std::string& filename, uint32_t output_bufsize = 0) {
            _config._log_output.reset(new hogl::output_pipe(filename.c_str(),
                    *_config._log_format, _config.get_output_bufsize(output_bufsize)));
            return *this;
        }
        self& textfile(const std::string& filename, uint32_t output_bufsize = 0) {
            _config._log_output.reset(new hogl::output_textfile(filename.c_str(),
                    *_config._log_format, _config.get_output_bufsize(output_bufsize)));
            return *this;
        }
    };

    class format_builder final : public config_builder_base {
        using self = format_builder;
    public:
        explicit format_builder(config& cfg) : config_builder_base{cfg} {}

        self& raw(){
            _config._log_format.reset(new hogl::format_raw);
            return *this;
        }

        self& basic(){
            _config._log_format.reset(new hogl::format_basic);
            return *this;
        }
    };

    class mask_builder final : public config_builder_base {
        using self = mask_builder;
    public:
        explicit mask_builder(config& cfg) : config_builder_base{cfg} {}

        template <typename Arg, typename ... Args>
        self& set(Arg mask, Args ... masks) {
            print(mask);
            this->set(masks...);
            return *this;
        }
    private:
        void set() {}
        template <typename Arg>
        void print(Arg mask) {
            _config._mask << mask;
        }
    };

} // ns hogl

#endif //PROJECT_CONFIG_BUILDER_HPP
