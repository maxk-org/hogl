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
 * @file hogl/c-api/engine.h
 * Top level C-API interface for the engine.
 */
#ifndef HOGL_CAPI_ENGINE_H
#define HOGL_CAPI_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#include <hogl/c-api/mask.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Engine feature flags
 */ 	
enum hogl_engine_features {
	HOGL_DISABLE_TSO = (1<<0)
};

/**
 * Engine options.
 */
struct hogl_engine_options {
	unsigned int features;
	hogl_mask_t  default_mask;
	unsigned int polling_interval_usec;
	unsigned int tso_buffer_capacity;
	int          cpu;
	unsigned int internal_ring_capacity; /* obsolete */
};

/**
 * Activate default holg engine.
 */
void hogl_activate(hogl_output_t out, struct hogl_engine_options *opts);

/**
 * Deactivate default holg engine
 */
void hogl_deactivate();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOGL_CAPI_ENGINE_H
