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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <bfd.h>

#include <iostream>

#include "tools/coredump.hpp"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

coredump::section::section(uint64_t _vma, unsigned long _size, bfd *b) :
	vma(_vma), addr(0), size(_size), abfd(b)
{}

void coredump::section::alloc()
{
	// Allign each section to the page size to avoid unaligned access
	int err = posix_memalign((void **) &addr, sysconf(_SC_PAGESIZE), size);
	if (err)
		addr = 0;
}

coredump::section::~section()
{
	free(addr);
}

static bfd_boolean bfd_match_vma(bfd *b, asection *sect, void *vma)
{
	return (uint64_t) vma == bfd_get_section_vma(b, sect);
}

static asection *bfd_get_section_by_vma(bfd *b, uint64_t vma)
{
	return bfd_sections_find_if(b, bfd_match_vma, (void *) vma);
}

void coredump::load_section(coredump::section *s)
{
	this->failed = true;

	s->alloc();
	if (!s->addr) {
		fprintf(stderr, "error: failed to allocate section with vma 0x%lu size %lu\n", 
			(unsigned long) s->vma, (unsigned long) s->size);
		return;
	}

	unsigned long vma  = s->vma;
	unsigned long size = s->size;
	uint8_t      *addr = s->addr;

	while (size > 0) {
		asection *sect = bfd_get_section_by_vma(s->abfd, vma);
		if (!sect) {
			fprintf(stderr, "error: section with vma 0x%lu not found\n", vma);
			return;
		}

		unsigned long ss = bfd_section_size(s->abfd, sect);
		if (ss > size) {
			// This basically means coalescer bug
			fprintf(stderr, "error: section with vma 0x%lx is too large\n", vma);
			return;
		}

		if (!bfd_get_section_contents(s->abfd, sect, addr, 0, ss)) {
			fprintf(stderr, "error: failed to load section content. vma 0x%lu size %lu\n", vma, size);
			return;
		}

		addr += ss;
		size -= ss;
		vma  += ss;
	}

	this->failed = false;
	return;
}

// Go through the sections and merge those
// that form contigious chunks of memory.
// Note that only sections from the same core/object file are merged.
void coredump::coalesce_and_load_sections()
{
	section_set nset;

	// Start and end of the current section
	uint64_t start = 0;
	uint64_t end   = 0;
	bfd    *abfd   = 0;

	section_set::const_iterator it;
	for (it = sections.begin(); it != sections.end(); ++it) {
		const section *s = *it;
		if (!start) {
			// New section
			start = s->vma;
			end   = s->vma + s->size;
			abfd  = s->abfd;
			continue;
		}

		if (s->abfd == abfd && s->vma == end) {
			// Continuation of the current section
			end += s->size;
			continue;
		}

		// New non-contigious chunk
		// Allocate current section and start the new one
		section *ns = new section(start, end - start, abfd);
		load_section(ns);
		nset.insert(ns);

		start = s->vma;
		end   = s->vma + s->size;
		abfd  = s->abfd;
	}

	if (start) {
		// Flush the last chunk 
		section *ns = new section(start, end - start, abfd);
		load_section(ns);
		nset.insert(ns);
	}

	// Release the original set and replace it with the new one
	while (!sections.empty()) {
		section_set::iterator it = sections.begin();
		section *s = *it; delete s;
		sections.erase(it);
	}
	sections = nset;
}

void coredump::add_section(bfd *abfd, asection *sect, void *_self)
{
	coredump &self = *(coredump *) _self;

	unsigned long vma  = bfd_get_section_vma(abfd, sect);
	unsigned long size = bfd_section_size(abfd, sect);

	// Skip sections with no data
	if (!vma || !size)
		return;

	coredump::section *s = new coredump::section(vma, size, abfd);

	std::pair<section_set::iterator, bool> i;
	i = self.sections.insert(s);

	if (i.second == false) {
		fprintf(stderr, "warning: section with vma %p already exists.\n", (void *) vma);
		//self.failed = true;
	}
}

void coredump::load(bfd *abfd)
{
	bfd_map_over_sections(abfd, add_section, this);
}

coredump::coredump(const char *corefile, const char *execfile)
{
	failed = false;

	char **match;

	_exec_bfd = 0;
	_core_bfd = 0;

	// Hopefully we can call this more than once if needed
	bfd_init();

	// Load the corefile first
	bfd* core_bfd = bfd_openr(corefile, "default");
	if (!core_bfd) {
		fprintf(stderr, "error: failed to open %s. %s(%d)\n",
				 corefile, strerror(errno), errno);
		failed = true;
		return;
	}

	if (!bfd_check_format_matches(core_bfd, bfd_core, &match)) {
		fprintf(stderr, "error: input file is not a coredump\n");
		failed = true;
		return;
	}

	const char *cmd = bfd_core_file_failing_command(core_bfd);
	fprintf(stderr, "info: core file generated by: %s\n", cmd);

	load(core_bfd);
	_core_bfd = core_bfd;

	// Now load the executable file (if any).
	// Exec files are optional but we won't have complete memory image unless
	// verbose coredump was used.
	if (execfile) {
		bfd* exec_bfd = bfd_openr(execfile, "default");
		if (!exec_bfd) {
			fprintf(stderr, "error: failed to open %s. %s(%d)\n",
				 execfile, strerror(errno), errno);
			failed = true;
			return;
		}

		if (!bfd_check_format_matches (exec_bfd, bfd_object, &match)) {
			fprintf(stderr, "error: input file is not an executable\n");
			failed = true;
			return;
		}

		if (!core_file_matches_executable_p(core_bfd, exec_bfd))
			fprintf(stderr, "warning: core file does not match the executable\n");

		load(exec_bfd);

		// FIXME: In addition to the exec file itself we also need to load
		// all the dependencies (ie shared libs) in order to have access to 
		// static strings storred in those objects.

		_exec_bfd = exec_bfd;
	}

	if (!failed)
		coalesce_and_load_sections();
}

coredump::~coredump()
{
	// Release core sections 
	while (!sections.empty()) {
		section_set::iterator it = sections.begin();
		section *s = *it; delete s;
		sections.erase(it);
	}

	if (_core_bfd)
		bfd_close(_core_bfd);
	if (_exec_bfd)
		bfd_close(_exec_bfd);
}

void* coredump::remap(const void *ptr, unsigned long n)
{
	uint64_t start = (uint64_t) ptr;
	uint64_t end   = start + n;

	section_set::const_iterator it;
	for (it = sections.begin(); it != sections.end(); ++it) {
		const section *s = *it;
		if (start >= s->vma && end <= (s->vma + s->size))
			return s->addr + (start - s->vma);
	}
	return 0;
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

