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

#ifndef HOGL_COREDUMP_H
#define HOGL_COREDUMP_H

#include <stdint.h>

#include <string>
#include <list>
#include <map>
#include <set>

#include <bfd.h>

namespace hogl {

/**
 * Core dump file
 */
class coredump {
public:
	/**
	 * Core dump section 
	 */ 	
	class section {
	public:
		uint64_t      vma;
		uint8_t      *addr;
		unsigned long size;
		bfd          *abfd;

		bool inside(void *ptr, unsigned long n)
		{
			const uint8_t *start = (uint8_t *) ptr;
			const uint8_t *end   = start + n;
			if (start >= addr && end <= addr + size)
				return true;
			return false;
		}

		section(uint64_t vma, unsigned long size, bfd *b);
		~section();
		void alloc();
	};

	struct section_sorter {
		bool operator()(const section* s1, const section* s2) const
		{
			return s1->vma < s2->vma;
		}
	};
	typedef std::set<section *, section_sorter> section_set;

	section_set         sections;
	bool                failed;

	coredump(const char *corefile, const char *execfile = 0, unsigned int flags = 0);
	~coredump();

	void* remap(const void *ptr, unsigned long n = 0);

private:
	void load(bfd *b);
	void load_section(coredump::section *s);
	void coalesce_and_load_sections();
	static void add_section(bfd *b, asection *sect, void *_self);

	bfd *_core_bfd;
	bfd *_exec_bfd;
};

} // namespace hogl

#endif
