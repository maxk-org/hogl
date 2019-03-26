/*
   Copyright (c) 2019, Max Krasnyansky <max.krasnyansky@gmail.com> 
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

#include <stdint.h> // uint64_t
#include <stdio.h> // stderr
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <climits>

#ifdef HOGL_DEBUG
#define dprint(fmt, args...) fprintf(stderr, "hogl: " fmt "\n", ##args)
#else
#define dprint(a...)
#endif

namespace hogl
{

cpu_set_t set_cpu_masks(cpu_set_t cpuset, uint64_t core_id_mask) {
   dprint("hogl::setaffinity core mask %lu", core_id_mask);
   int bit = sizeof(core_id_mask) * CHAR_BIT - 1;
   while (bit >= 0) {
      if (core_id_mask & (1ULL << bit)) {
         dprint("hogl::setaffinity core id is %d", bit);
         CPU_SET(bit, &cpuset);
      }
      --bit;
   }
   return cpuset;
}

int setaffinity(pthread_t thread_id, uint64_t core_id_mask) {
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   cpuset = set_cpu_masks(cpuset, core_id_mask);
   if (CPU_COUNT(&cpuset)) {
      return pthread_setaffinity_np(thread_id, sizeof(cpu_set_t), &cpuset);
   }
   return 0;
}

} // ns hogl