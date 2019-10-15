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

/**
 * @file hogl/c-api/tls.h
 * C-API wrappers for allocating TLS objects.
 */
#ifndef HOGL_CAPI_TLS_H
#define HOGL_CAPI_TLS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TLS handle
 */
typedef void* hogl_tls_t;

/**
 * TLS options.
 */
struct hogl_tls_options {
	/** Capacity of the ring buffer */
	unsigned int ring_capacity;
	/** Priority of the ring buffer */
	unsigned int ring_priority;
	/** Tailroom for the records */
	unsigned int record_tailroom;
};

/**
 * Allocate new TLS object.
 * Must be called as very first thing from the thread entry function.
 * This function allocates new ring buffer for the calling thread.
 * @param name name of this TLS, actually name of the ring buffer
 * @param opts pointer to the TLS options structure
 * @return new TLS handle
 */
hogl_tls_t hogl_new_tls(const char *name, struct hogl_tls_options *opts);

/**
 * Delete TLS object.
 * Must be called before the thread exits, otherise TLS object and ring 
 * buffer will be leaked.
 * @param tls TLS handle
 */
void hogl_delete_tls(hogl_tls_t tls);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOGL_CAPI_TLS_H
