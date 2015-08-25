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
 * @file hogl/arae.hpp
 * Top level area functions
 */

#ifndef HOGL_AREA_HPP
#define HOGL_AREA_HPP

#include <hogl/detail/area.hpp>
#include <hogl/detail/engine.hpp>

namespace hogl {

extern engine *default_engine;

/**
 * Add a new area to the default engine
 * @param name area name
 * @param sections pointer to section numbers (must be null terminated)
 * @return pointer to the new area
 */
static inline area *add_area(const char *name, const char **sections = 0)
{
	return default_engine->add_area(name, sections);
}

/**
 * Find an area in the default engine
 * @param name area name
 * @return pointer to the area if found, zero otherwise
 */
static inline const area *find_area(const char *name)
{
	return default_engine->find_area(name);
}

/**
 * Get a list of areas from the default engine
 * @param l string list reference
 */
static inline void list_areas(string_list &l)
{
	return default_engine->list_areas(l);
}

} // namespace hogl

#endif // HOGL_AREA_HPP
