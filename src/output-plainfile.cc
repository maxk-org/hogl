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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "hogl/detail/ostrbuf-fd.hpp"
#include "hogl/output-plainfile.hpp"
#include "hogl/fmt/printf.h"

__HOGL_PRIV_NS_OPEN__
namespace hogl {

output_plainfile::output_plainfile(const char *name, format &fmt, unsigned int buffer_capacity) :
	output(fmt)
{
	int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fmt::fprintf(stderr, "hogl::output_plainfile: failed to open %s for writing. %s(%d)\n", name, strerror(errno), errno);
		abort();
	}

	init(new ostrbuf_fd(fd, ostrbuf_fd::CLOSE_ON_DELETE, buffer_capacity));

	output::header(name);
}

output_plainfile::~output_plainfile()
{
	output::footer();
}

} // namespace hogl
__HOGL_PRIV_NS_CLOSE__

