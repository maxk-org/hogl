## Overview

HOGL stands for "HOly Grail of Logging". It is a scalable, high performance and low overhead logging library for C++ and C applications.

HOGL is designed from the ground up for low overhead and deterministic behavior. It's also designed to scale very well on the latest 
multi-core systems with multi-threaded applications.

## What's wrong with existing libraries?

There are many things wrong with existing libraries when it comes to using them in realtime applications or in general where low 
overhead / high-volume logging is required. In fact most existing libraries are not suitable for realtime applications at all due 
to their overhead and non-deterministic behavior. This includes libraries like syslog, log4cpp, original ACE logging, Apache log 
library, Boost logging library, etc.

Existing libraries that are suitable for realtime are often not flexible and do not scale well on the multi-core systems.
Often different macros and/or functions must be used depending on the realtime requirements.
Developers often end up using the wrong macros and introduce latencies. Also those special macros usually do not support 
strings and other basic data types like double and float.

Most existing libraries use their own printf formatters with slightly incompatible notations.

## Design motivations

* Fix everything that is wrong with existing libraries, or at least the most of it. See examples above.
* Use latest compiler and platform features:
** GCC and libc provide excellent, low overhead support for TLS (thread local storage)
** Latest C++ compilers can optimize out all redundant code with just a little bit of help
** LibFFI allows for generating stack frames and calling sprintf() directly
* Provide ultimate performance, scalability and flexibility:
** Minimal overhead
** All logging methods are safe for realtime code
** Support for all native C/C++ types like strings, double, float, pointers, etc
* Minimal external dependencies

## Feature highlights
* Efficient per thread ring buffers are used for storing log records, which removes all contention between the threads that produce log records (support for shared ring buffers is also provided).
* All HOGL objects can be created and registered dynamically. This includes: log areas, log masks, per thread ring buffers, etc.
Log areas can contain unlimited number of sections (aka levels). HOGL does not limit log area to standard log levels like: debug, info, error, etc. Instead each area can define its own sections.
* The library is designed to avoid memory allocations and system calls. Client side functions (posting log records) never allocate any memory and use no system calls. Log record processing engine does memory allocations only when new objects are registered.
* Printf string formating is done using native sprintf() function. All basic C/C++ types are supported.
* Log masks are regular expression strings that can be applied to multiple areas and sections in one shot.
* Logging output can be piped to an external program. This can be used for things like: compressing logs on the fly (gzip -1 is often faster than the disk), simple post processing on the fly (grepping through the records).
* Default log output can compute timestamp deltas between consecutive log records, which is very useful for profiling.
* Run time library has minimal external dependencies. The only required components are libc, libstdc++ and libffi. All those libraries are available on most modern platforms and architectures.
* Unsaved log records can be extracted from coredumps. HOGL structures are designed to be easy to locate in the core dump files. Even if the application crashes due to memory corruption HOGL tools can usually recover unsaved log messages.

## Performance numbers
Posting a log record with a format string and three arguments takes about 55 nanoseconds on average, with the best case of about 
20 nanoseconds and the worst case of about 200 nanoseconds.
The logging engine is able to process 600 log records per millisecond at 99% CPU utilization.
With string formatting disabled the logging engine is able to process 1000 log records per millisecond at 6% CPU utilization.
The numbers were collected on a Linux machine with 8-core 2.4Gz 64bit x86 processor (dual Intel Xeon E5530).
Default HOGL ring buffer and engine configuration was used. The test starts 10 threads.
Each thread generates N records per millisecond.
Log output file was set to /dev/null. Default clock_gettime timesource was used.

See Wiki pages for more details
