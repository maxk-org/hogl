/*
   Copyright (c) 2015, Max Krasnyansky <max.krasnyansky@gmail.com> 
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

#include <stdarg.h>
#include <string.h>

#include "hogl/detail/args.hpp"
#include "hogl/format-basic.hpp"
#include "hogl/output-file.hpp"
#include "hogl/output-stdout.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/output-textfile.hpp"
#include "hogl/output-pipe.hpp"
#include "hogl/post.hpp"
#include "hogl/flush.hpp"
#include "hogl/engine.hpp"
#include "hogl/area.hpp"
#include "hogl/mask.hpp"
#include "hogl/tls.hpp"

#include "hogl/c-api/area.h"
#include "hogl/c-api/mask.h"
#include "hogl/c-api/format.h"
#include "hogl/c-api/output.h"
#include "hogl/c-api/engine.h"
#include "hogl/c-api/tls.h"

// ---- Area wrappers ----
extern "C" bool hogl_area_test(const hogl_area_t _area, unsigned int sect)
{
	hogl::area *area = (hogl::area *) _area;
	return area->test(sect);
}

extern "C" void hogl_area_set(const hogl_area_t _area, unsigned int sect)
{
	hogl::area *area = (hogl::area *) _area;
	area->set(sect);
}

extern "C" void hogl_area_set_all(const hogl_area_t _area)
{
        hogl::area *area = (hogl::area *) _area;
        area->set();
}

extern "C" void hogl_area_reset(const hogl_area_t _area, unsigned int sect)
{
        hogl::area *area = (hogl::area *) _area;
        area->reset(sect);
}
extern "C" void hogl_area_reset_all(const hogl_area_t _area)
{
        hogl::area *area = (hogl::area *) _area;
        area->reset();
}
extern "C" const hogl_area_t hogl_add_area(const char *name, const char **sections)
{
	return (const hogl_area_t) hogl::add_area(name, sections);
}

// ---- Engine wrappers ----
extern "C" void hogl_activate(hogl_output_t _out, struct hogl_engine_options *_opts)
{
	hogl::output *out = (hogl::output *)_out;

	struct hogl::engine::options opts(hogl::engine::default_options);
	if (_opts) {
		opts.cpu_affinity_mask     = _opts->cpu;
		opts.polling_interval_usec = _opts->polling_interval_usec;
		opts.features            = _opts->features;
		opts.tso_buffer_capacity = _opts->tso_buffer_capacity;
		opts.internal_ring_capacity = _opts->internal_ring_capacity;
		if (_opts->default_mask) {
			hogl::mask *mask = (hogl::mask *) _opts->default_mask;
			opts.default_mask = *mask;
		}
	}

	hogl::activate(*out, opts);
}
extern "C" void hogl_deactivate()
{
	hogl::deactivate();
}

// ---- Format wrappers ----
extern "C" hogl_format_t hogl_new_format_basic(const char *fields)
{
	hogl::format *fmt;
	if (fields)
		fmt = new hogl::format_basic(fields);
	else
		fmt = new hogl::format_basic();
	return fmt;
}
extern "C" void hogl_delete_format(hogl_format_t _fmt)
{
	hogl::format *fmt = (hogl::format *) _fmt;
	delete fmt;
}

// ---- Mask wrappers ----
extern "C" void hogl_mask_add(hogl_mask_t _mask, const char *str)
{
	hogl::mask *mask = (hogl::mask *)_mask;
	*mask << str;
}
extern "C" void hogl_mask_clear(hogl_mask_t _mask)
{
	hogl::mask *mask = (hogl::mask *)_mask;
	mask->clear();
}
extern "C" hogl_mask_t hogl_new_mask(const char *str, ...)
{
	hogl::mask *mask = new hogl::mask();
	va_list ap;

	if (!str)
		return mask;

	*mask << str;
	va_start(ap, str);
	while ((str = va_arg(ap, char *)))
		*mask << str;
	va_end(ap);

	return mask;
}
extern "C" void hogl_delete_mask(hogl_mask_t _mask)
{
	hogl::mask *mask = (hogl::mask *)_mask;
	delete mask;
}
extern "C" void hogl_apply_mask(hogl_mask_t _mask)
{
	hogl::mask *mask = (hogl::mask *)_mask;
	hogl::apply_mask(*mask);
}

// ---- Output wrappers ----
extern "C" hogl_output_t hogl_new_output_stdout(hogl_format_t _fmt)
{
	hogl::format *fmt = (hogl::format *) _fmt;
	hogl::output_stdout *out = new hogl::output_stdout(*fmt);
	return out;
}
extern "C" hogl_output_t hogl_new_output_stderr(hogl_format_t _fmt)
{
	hogl::format *fmt = (hogl::format *) _fmt;
	hogl::output_stderr *out = new hogl::output_stderr(*fmt);
	return out;
}

extern "C" hogl_output_t hogl_new_output_file(const char *name, hogl_format_t _fmt, struct hogl_output_options *_opts)
{
	struct hogl::output_file::options opts(hogl::output_file::default_options);
	// we could use memcpy here but there is a risk that these structs
	// would diverge so it is safer to have it one field at a time
	if(_opts) {
		opts.perms = _opts->perms;
		opts.max_size = _opts->max_size;
		opts.max_age = _opts->max_age;
		opts.max_count = _opts->max_count;
		opts.buffer_capacity = _opts->buffer_capacity;
		opts.cpu_affinity_mask = _opts->cpu;
	}
	hogl::format *fmt = (hogl::format *) _fmt;
	hogl::output_file *out = new hogl::output_file(name, *fmt, opts);
	return out;
}


extern "C" hogl_output_t hogl_new_output_textfile(const char *name, hogl_format_t _fmt)
{
	hogl::format *fmt = (hogl::format *) _fmt;
	hogl::output_textfile *out = new hogl::output_textfile(name, *fmt);
	return out;
}

extern "C" hogl_output_t hogl_new_output_pipe(const char *cmd, hogl_format_t _fmt)
{
	hogl::format *fmt = (hogl::format *) _fmt;
	hogl::output_pipe *out = new hogl::output_pipe(cmd, *fmt);
	return out;
}

extern "C" void hogl_delete_output(hogl_output_t _out)
{
	hogl::output *out = (hogl::output *)_out;
	delete out;
}

// ---- TLS wrappers ----
extern "C" hogl_tls_t hogl_new_tls(const char *name, struct hogl_tls_options *_opts)
{
	hogl::ringbuf::options opts;

	opts.capacity = _opts->ring_capacity;
	opts.prio     = _opts->ring_priority;
	opts.record_tailroom = _opts->record_tailroom ? _opts->record_tailroom : 80;
	opts.flags    = 0;

	hogl::tls *tls = new hogl::tls(name, opts);

	return (hogl_tls_t) tls;
}

extern "C" void hogl_delete_tls(hogl_tls_t _tls)
{
	hogl::tls *tls = (hogl::tls *) _tls;
	delete tls;
}

// ---- Post wrappers ----
extern "C" void __hogl_post_1(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0]);
}
extern "C" void __hogl_post_2(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1]);
}
extern "C" void __hogl_post_3(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2]);
}
extern "C" void __hogl_post_4(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3]);
}
extern "C" void __hogl_post_5(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4]);
}
extern "C" void __hogl_post_6(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
}
extern "C" void __hogl_post_7(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]);
}
extern "C" void __hogl_post_8(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7]);
}
extern "C" void __hogl_post_9(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8]);
}
extern "C" void __hogl_post_10(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9]);
}
extern "C" void __hogl_post_11(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9], arg[10]);
}
extern "C" void __hogl_post_12(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9], arg[10], arg[11]);
}
extern "C" void __hogl_post_13(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9], arg[10], arg[11], arg[12]);
}
extern "C" void __hogl_post_14(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9], arg[10], arg[11], arg[12], arg[13]);
}
extern "C" void __hogl_post_15(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14]);
}
extern "C" void __hogl_post_16(const hogl_area_t _area, unsigned int sect, struct hogl_arg *_arg)
{
	hogl::area *area = (hogl::area *) _area;
	hogl::arg  *arg  = (hogl::arg *)  _arg;
	hogl::post(area, sect, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7],
			arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15]);
}

extern "C" void hogl_flush()
{
	hogl::flush();
}
