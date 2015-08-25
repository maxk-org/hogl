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
 * @file hogl/detail/area.hpp
 * Details of the area handling
 */
#ifndef HOGL_DETAIL_AREA_HPP
#define HOGL_DETAIL_AREA_HPP

#include <stdint.h>
#include <iostream>

#include <hogl/detail/bitmap.hpp>
#include <hogl/detail/magic.hpp>

namespace hogl {

/**
 * Logging area.
 * Defines a logging area with sections. Sections are identified either 
 * by name or by id (zero based index). 
 * Area contains a bitmap with one bit per section. If the section bit is 
 * set to 1 then this section is enabled, otherwise it's disabled and records 
 * with that section id are discarded.
 * This class mimics std::bitset interface where set(...) and reset(...) set 
 * or clear section bits. size() or count() returns number of sections.
 */
class area {
protected:
	magic          _magic;   /// Magic number 
	char          *_name;    /// Area name
	const char   **_section; /// Section names
	bitmap _bitmap;  /// Bitmap (one bit per section)

public:
	/**
	 * Default section IDs
	 */
	enum default_section_ids {
		INFO,
		WARN,
		ERROR,
		FATAL,
		DEBUG,
		TRACE
	};

	/**
 	 * Construct new area
 	 * @param name area name 
 	 * @param section pointer to the array of section names. Must be null terminated.
 	 * If section names pointer is null the area will contain default sections. 
 	 */
	area(const char *name, const char **section = 0);
	~area();

	/**
 	 * Get area name
 	 * @return section name
 	 */
	const char *name() const { return _name; }

	/**
 	 * Get section names
 	 * @return pointer to an array of section names
 	 */
	const char *section_name(unsigned int i) const
	{
		if (i >= _bitmap.size())
			return "INVALID";
		return _section[i];
	}

	/**
 	 * Get number of sections
 	 * @return number of sections
 	 */
	unsigned int count() const { return _bitmap.size(); }

	/**
 	 * Get number of sections (same as count)
 	 * @return number of sections
 	 */
	unsigned int size() const { return _bitmap.size(); }

	/**
 	 * Test if specific section is enabled. 
 	 * @param s section number
 	 * @return true if section is enabled, false otherwise
 	 */
	bool test(unsigned int s) const
	{
		return _bitmap.test(s);
	}

	/**
 	 * Set all section bits to 1 (enable all sections)
 	 */
	void set() { _bitmap.set(); }

	/**
 	 * Clear all section bits (disable all sections)
 	 */
	void reset() { _bitmap.reset(); }

	/**
 	 * Set section bit value
 	 * @param s section number
 	 */
	void set(unsigned int s, bool v = true)
	{
		_bitmap.set(s, v);
	}

	/**
 	 * Clear section bit value
 	 * @param s section number
 	 */
	void reset(unsigned int s)
	{
		_bitmap.reset(s);
	}

	/**
 	 * Enable all sections (set all sections bits to 1)
 	 */ 
	void enable() { _bitmap.set(); }

	/**
 	 * Enable a section. Sets section bit value
 	 * @param s section number
 	 */
	void enable(unsigned int s, bool v = true)
	{
		_bitmap.set(s, v);
	}

	/**
 	 * Disable all sections (clear all section bits)
 	 */
	void disable() { _bitmap.reset(); }

	/**
 	 * Disable a section. Clears section bit
 	 * @param s section number
 	 */
	void disable(unsigned int s)
	{
		_bitmap.reset(s);
	}

	bool operator==(const area &a) const;

	bool operator!=(const area &a) const { return !(*this == a); }

private:
	// No copies
	area(const area&);
	area& operator=( const area& );
};

std::ostream& operator<< (std::ostream& s, const area& area);

} // namespace hogl

#endif // HOGL_DETAIL_AREA_HPP
