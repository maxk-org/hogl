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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <sstream>

#include "hogl/detail/area.hpp"
#include "hogl/fmt/printf.h"

#ifdef HOGL_DEBUG
#define dprint(fstr, args...) fmt::fprintf(stderr, "hogl: " fstr "\n", ##args)
#else
#define dprint(a...)
#endif

__HOGL_PRIV_NS_OPEN__
namespace hogl {

static const char *default_section_names[] = {
	"INFO", "WARN", "ERROR", "FATAL", "DEBUG", "TRACE", 0
};

area::area(const char *name, const char **section) :
	_magic(hogl::area_magic),
	_name(strdup(name))
{
	if (section) {
		// Count number of sections and resize the bitmap
		unsigned int i;
		for (i=0; section[i]; ++i) /* noop */;
		_bitmap.resize(i);

		_section = new const char* [_bitmap.size()];
		for (i=0; i < _bitmap.size(); ++i)
			_section[i] = strdup(section[i]);
	} else {
		_section = default_section_names;
		_bitmap.resize(6);
	}

	_bitmap.reset();

	dprint("created area %p. name %s size %u", (void*)this, _name, _bitmap.size());	
}

area::~area()
{
	unsigned int i;
	dprint("destroyed area %p. name %s", (void*)this, _name);

	free(_name);

	if (_section != default_section_names) {
		for (i=0; i < _bitmap.size(); ++i)
			free((void *) _section[i]);
		delete [] _section;
	}
}

bool area::operator== (const area &area) const
{
	if (strcmp(name(), area.name()))
		return false;

	if (size() != area.size())
		return false;

	unsigned int i;
	for (i=0; i < _bitmap.size(); ++i)
		if (strcmp(_section[i], area._section[i]))
			return false;
	return true;
}

// Dump area info into a stream
std::ostream& operator<< (std::ostream& s, const area& area)
{
	unsigned int i, w = s.width();
	for (i=0; i < area.count(); i++) {
		s.width(w);
		s << (area.test(i) ? "" : "!") << area.name() << ":" << area.section_name(i) << std::endl;
	}
	return s;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

