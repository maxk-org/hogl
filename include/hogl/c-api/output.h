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

/**
 * @file hogl/c-api/output.h
 * C-API output wrappers.
 */
#ifndef HOGL_CAPI_OUTPUT_H
#define HOGL_CAPI_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Output handle
 */
typedef void* hogl_output_t;

/**
 * Allocate new stdout output handler
 * @param fmt format handle @see hogl_new_format_basic
 * @return new format handle, or zero on failure
 */
hogl_output_t hogl_new_output_stdout(hogl_format_t fmt);

/**
 * Allocate new stderr output handler
 * @param fmt format handle @see hogl_new_format_basic
 * @return new format handle, or zero on failure
 */
hogl_output_t hogl_new_output_stderr(hogl_format_t fmt);

/**
 * File output options
 * @note -  this is copied from output-file.hpp.
 *          so everything that is changed in output-file.hpp needs
 *          to be reflected also here
 */
struct hogl_output_options {
	unsigned int perms;           /// File permissions
	size_t       max_size;        /// Max size of each file chunk (bytes)
	unsigned int max_age;         /// Max age of each file chunk (seconds)
	unsigned int max_count;       /// Max file count. Index goes back to zero after it reaches max_count.
	unsigned int buffer_capacity; /// Max capacity of the output buffer (bytes)
	int          cpu;             /// The CPU number to be used
};

/**
 * Allocate new generic output handler.
 * @param name filename
 * @param fmt format handle @see hogl_new_format_basic
 * @param opts options to use for output file. if null, defaults will be used
 * @return new format handle, or zero on failure
 */
hogl_output_t hogl_new_output_file(const char *name, hogl_format_t _fmt, struct hogl_output_options *opts);

/**
 * Allocate new textfile output handler.
 * @param name filename
 * @param fmt format handle @see hogl_new_format_basic
 * @return new format handle, or zero on failure
 */
hogl_output_t hogl_new_output_textfile(const char *name, hogl_format_t fmt);

/**
 * Allocate new pipe output handler.
 * @param cmd command to pipe the output to
 * @param fmt format handle @see hogl_new_format_basic
 * @return new format handle, or zero on failure
 */
hogl_output_t hogl_new_output_pipe(const char *cmd, hogl_format_t fmt);

/**
 * Delete output handle.
 * @param output handle
 */
void hogl_delete_output(hogl_output_t out);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOGL_CAPI_OUTPUT_H
