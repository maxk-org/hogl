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
 * @file hogl/capi/area.h
 * Top level C-API area functions
 */
#ifndef HOGL_CAPI_AREA_H
#define HOGL_CAPI_AREA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Area handle.
 */
typedef void* hogl_area_t;

/**
 * Test if a section is enabled
 * @param area area handle
 * @param sect section id
 * @return true if section is enabled, false otherwise
 */
bool hogl_area_test(const hogl_area_t area, unsigned int sect);

/**
 * Enable a section
 * @param area area handle
 * @param sect section id
 */
void hogl_area_set(const hogl_area_t area, unsigned int sect);

/**
 * Enable all sections in the area
 * @param area area handle
 */
void hogl_area_set_all(const hogl_area_t area);

/**
 * Disable a sections in the area
 * @param area area handle
 * @param sect section id
 */
void hogl_area_reset(const hogl_area_t area, unsigned int sect);

/**
 * Disable all sections in the area
 * @param area area handle
 */
void hogl_area_reset_all(const hogl_area_t area);

/**
 * Add new area to the default engine
 * @param name area name
 * @param sections array of pointers to the section names (must be null terminated)
 * @return pointer to the new area, or null if allocation failed
 */
const hogl_area_t hogl_add_area(const char *name, const char **sections);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOGL_CAPI_AREA_H
