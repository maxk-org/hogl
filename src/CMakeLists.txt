# Copyright (c) 2015, Max Krasnyansky <max.krasnyansky@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(LIB_NAME hogl)
project(${LIB_NAME})

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

include_directories(${INCLUDE_DIR})

set(SRC_FILES
        area.cc
        c-api.cc
        defring.cc
        deftimesource.cc
        engine.cc
        flush.cc
        format-basic.cc
        format-raw.cc
        internal.cc
#        mask-boost.cc FIXME this depends on CXX11
        mask.cc
        ostrbuf-fd.cc
        ostrbuf-stdio.cc
        ostrbuf.cc
        output-file.cc
        output-pipe.cc
        output-stderr.cc
        output-stdout.cc
        output-textfile.cc
        output.cc
        platform.cc
        post.cc
        ringbuf.cc
        timesource.cc
        tls.cc)

set(HOGL_STATIC_LIB ${LIB_NAME}Static)
set(HOGL_DYNAMIC_LIB ${LIB_NAME})
add_library(${HOGL_STATIC_LIB} STATIC ${SRC_FILES})
add_library(${HOGL_DYNAMIC_LIB} SHARED ${SRC_FILES} ../include/hogl/config.hpp config.cc ../include/hogl/config-builder.hpp config-builders.cc)
install(TARGETS ${LIB_NAME} DESTINATION lib)
