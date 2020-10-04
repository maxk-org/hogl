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
 * @file hogl/detail/mask.h
 * Mask implementation details.
 */
#ifndef HOGL_DETAIL_MASK_HPP
#define HOGL_DETAIL_MASK_HPP

#include <stdint.h>
#include <iostream>
#include <string>
#include <list>

#include <hogl/detail/area.hpp>

__HOGL_PRIV_NS_OPEN__
namespace hogl {

/**
 * Logging mask.
 */
class mask {
private:
	struct data;
	typedef std::list<data> data_list;
	data_list  *_list;

public:
	/**
	 * Construct new mask
	 */
	mask();

	/**
	 * Construct new mask (legacy interface)
	 * @param str pointer to the array of mask strings. Must be null terminated.
	 */
	mask(const char *str,...);

	/**
	 * Copy constructor
	 */
	mask(const mask &m);
	mask(mask &m);

	~mask();

	/**
 	 * Add string.
 	 * @param str area name + section name or POSIX regex.
 	 */
	void add(const std::string &str);

	void clear();

	/**
 	 * Apply mask to a specific area.
 	 */
	void apply(area &a) const;
	void apply(area *a) const { apply(*a); };

	/**
 	 * Add string.
 	 * @param str area name + section name or POSIX regex.
 	 */
	mask& operator<< (const std::string &str);

	void operator>> (std::ostream &s) const;

	void operator= (const mask &m);
};

std::ostream& operator<< (std::ostream& s, const mask& mask);
std::istream& operator>> (std::istream &s, mask &mask);

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__


#endif // HOGL_DETAIL_MASK_HPP
