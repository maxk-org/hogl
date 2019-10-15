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
 * @file hogl/c-api/mask.h
 * C-API mask wrappers.
 */
#ifndef HOGL_CAPI_MASK_H
#define HOGL_CAPI_MASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Mask handle
 */
typedef void* hogl_mask_t;

/**
 * Add a new regex string to the mask.
 * @param m mask handle
 * @param str pointer to the string to add. The string is copied to the internal buffer 
 * and need not be global accessible.
 */
void hogl_mask_add(hogl_mask_t m, const char *str);

/**
 * Clear the mask. Removes all regex strings.
 * @param m mask handle
 */
void hogl_mask_clear(hogl_mask_t m);

/**
 * Allocate new mask.
 * @param str pointer to the regex string to add to the mask.
 * string list must be null terminated.
 */
hogl_mask_t hogl_new_mask(const char *str, ...);

/**
 * Delete a mask.
 * @param m mask handle
 */
void hogl_delete_mask(hogl_mask_t m);

/**
 * Apply mask to the default engine.
 * The mask is applied to all areas in the default engine.
 * @param m mask handle
 */
void hogl_apply_mask(hogl_mask_t m);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOGL_CAPI_MASK_H
