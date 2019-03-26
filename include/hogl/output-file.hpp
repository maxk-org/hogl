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
 * @file hogl/output-file.h
 * File output handler.
 */
#ifndef HOGL_OUTPUT_FILE_HPP
#define HOGL_OUTPUT_FILE_HPP

#include <hogl/detail/output.hpp>

#include <stdint.h>
#include <pthread.h>
#include <string>
#include <cstring>

namespace hogl {

/**
 * Generic file output handler with support for file rotation and custom header and footer.
 * Rotation is performed by the helper thread outside of the writer's context.
 * The helper opens new file, updates the symlink and then swaps the file descriptor,
 * so the writter gets virtually no interruption during the rotation.
 */
class output_file : public output {
public:
	/**
	 * Write data into the file. Triggers rotation if current file size exceeds max size.
	 * @param data pointer to the data
	 * @param size number of bytes
	 * @return number of bytes written or -1 on error
	 */
	ssize_t write(const void *data, size_t size);

	/**
	 * Write data into the file. Triggers rotation if current file size exceeds max size.
	 * @param iov pointer to the iovec
	 * @param iovcnt number of entries in the iovec
	 * @return number of bytes written or -1 on error
	 */
	ssize_t writev(const struct iovec *iov, int iovcnt);

	/**
 	 * Get current filename
 	 */
	std::string name() const;

	/**
 	 * Get current file index
 	 */
	unsigned int index() const;

	/**
 	 * File output options
 	 */
	struct options {
		unsigned int perms;             /// File permissions
		size_t       max_size;          /// Max size of each file chunk (bytes)
		unsigned int max_age;           /// Max age of each file chunk (seconds)
		unsigned int max_count;         /// Max file count. Index goes back to zero after it reaches max_count.
		unsigned int buffer_capacity;   /// Max capacity of the output buffer (bytes)
		uint64_t     cpu_affinity_mask; /// The CPU number to be used
	};

	// Default options
	static options default_options;

	/**
 	 * File output constuctor. Open the file and set things up for writing.
 	 * @param name file name. Format 'prefix.#.suffix'. '#' will be replaced with the file sequence number.
 	 * @param fmt format handler.
 	 */
	output_file(const char *name, format &fmt, const options &opts = default_options);

	/**
 	 * Close file rotator.
 	 */
	virtual ~output_file();

private:
	std::string     _symlink;     /// Name of the symlink file
	std::string     _name;        /// Name of the current file
	std::string     _name_pref;   /// File name prefix
	std::string     _name_sufx;   /// File name suffix
	unsigned int    _name_index;  /// Current file index
	unsigned int    _index_width; /// Number of digits in the file index

	size_t          _max_size;    /// Max size in bytes
	size_t          _max_age;     /// Max age in seconds
	unsigned int    _max_count;   /// Max number of files
	unsigned int    _mode;        /// File mode

	int             _fd;          /// Current file descriptor
	size_t          _size;        /// Current size
	pthread_mutex_t _write_mutex; /// Mutex that protects fd and size

	volatile bool   _running;
	volatile bool   _killed;

	pthread_t       _rotate_thread;  /// Rotate thread handle
	pthread_mutex_t _rotate_mutex;   /// Mutex that protects rotate state
	pthread_cond_t  _rotate_cond;    /// Rotate signaling
	volatile bool   _rotate_pending; /// Rotate is pending

	// No copies
	output_file(const output_file&);
	output_file& operator=( const output_file& );

	/**
 	 * Update name of the current file
 	 */
	void update_name();

	/**
 	 * Update symlink to the current file
 	 */
	void update_link();

	/**
 	 * Read symlink to the current file and return corresponding index.
 	 */
	unsigned int read_link();

	/**
 	 * Performe rotation
 	 */
	void do_rotate();

	/**
	 * Helper thread entry point
	 */
	static void *thread_entry(void *_self);

	/**
	 * Helper thread entry loop
	 */
	void thread_loop();
};

} // namespace hogl

#endif // HOGL_OUTPUT_FILE_HPP
