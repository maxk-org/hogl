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

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <map>
#include <string>
#include <sstream>

#include "hogl/detail/mask.hpp"
#include "hogl/detail/area.hpp"

#ifdef HOGL_DEBUG
#define dprint(fmt, args...) fprintf(stderr, "hogl: " fmt "\n", ##args)
#else
#define dprint(a...)
#endif

#include <boost/xpressive/xpressive.hpp>
namespace bx = boost::xpressive;

__HOGL_PRIV_NS_OPEN__
namespace hogl {

struct mask::data {
	const std::string str;
	const bx::sregex  area;
	const bx::sregex  sect;
	bool  on;

	data(const std::string &_str, const bx::sregex &_area, const bx::sregex &_sect, bool _on) :
		str(_str), area(_area), sect(_sect), on(_on) { }
};

void mask::clear()
{
	_list->clear();
}

void mask::add(const std::string &str)
{
	bool on = (str[0] != '!');
	size_t abeg = !on;
	size_t aend = str.find(':', abeg);
	std::string areg, sreg;
	if (aend == str.npos) {
		areg = ".*";
		sreg = str.substr(abeg);
	} else {
		areg = str.substr(abeg, aend - abeg);
		sreg = str.substr(aend + 1);
	}

	if (areg.empty()) areg = ".*";
	if (sreg.empty()) sreg = ".*";

	_list->push_back(mask::data(str, bx::sregex::compile(areg), bx::sregex::compile(sreg), on));
}

static void __apply(area &area, const bx::sregex &re, bool on)
{
	unsigned int i;
	for (i=0; i < area.size(); i++) {
		const std::string str(area.section_name(i));
		if (bx::regex_match(str, re))
			area.set(i, on);
	}
}

void mask::apply(area &area) const
{
	data_list::const_iterator it;
	for (it=_list->begin(); it != _list->end(); ++it) {
		const std::string str(area.name());
		if (bx::regex_match(str, it->area)) {
			dprint("applying mask %p to area %p [%s]", this, &area, area.name());
			__apply(area, it->sect, it->on);
		}
	}
}

mask& mask::operator<< (const std::string &str)
{
	mask::add(str);
	return *this;
}

std::istream& operator>> (std::istream &s, mask &m)
{
	std::stringbuf sb;

	// Read lines from the stream and feed them into the mask
	while (s.good()) {
		sb.str("");
		s.get(sb);
		m << sb.str();
	}

	return s;
}

// Dump mask info into a stream
void mask::operator>> (std::ostream& s) const
{
	unsigned int w = s.width();
	data_list::const_iterator it;
	for (it=_list->begin(); it != _list->end(); ++it) {
		s.width(w);
		s << "" << it->str << std::endl;
	}
}

// Dump mask info into a stream
std::ostream& operator<< (std::ostream& s, const mask& mask)
{
	mask >> s;
	return s;
}

mask::mask(const char *str,...)
{
	_list = new data_list;

	dprint("created mask %p list %p", this, _list);

	if (!str)
		return;

        va_list ap;
        va_start(ap, str);

        const char *s = str;
	while (s && *s) {
		mask::add(s);
                s = va_arg(ap, const char *);
	};

        va_end(ap);
}

mask::mask(const mask &m)
{
	_list = new data_list(*m._list);
	dprint("created mask %p (ro copy of %p) list %p", this, &m, _list);
}

mask::mask(mask &m)
{
	_list = new data_list(*m._list);
	dprint("created mask %p (rw copy of %p) list %p", this, &m, _list);
}

void mask::operator=(const mask &m)
{
	delete _list;
	_list = new data_list(*m._list);
}

mask::~mask()
{
	dprint("deleted mask %p list %p", this, _list);
	delete _list;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

