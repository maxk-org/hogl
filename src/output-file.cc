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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <string>
#include <sstream>
#include <iomanip>

#include "hogl/detail/utils.hpp"
#include "hogl/detail/ostrbuf-fd.hpp"
#include "hogl/platform.hpp"
#include "hogl/output-file.hpp"

namespace hogl {

class ostrbuf_file : public ostrbuf
{
private:
	output_file &_of;

	void do_flush(const uint8_t *data, size_t len)
	{
		struct iovec iov[2];
		unsigned int i = 0;

		// Flush buffered data
		if (_size) {
			iov[i].iov_base = (void *) _data;
			iov[i].iov_len  = _size;
			i++;
		}

		// Flush un-buffered data
		if (len) {
			iov[i].iov_base = (void *) data;
			iov[i].iov_len  = len;
			i++;
		}

		if (i) {
			int r = _of.writev(iov, i);
			if (r < 0)
				ostrbuf::failure(strerror(errno));
		}

		this->reset();
	}

public:
	ostrbuf_file(output_file &of, unsigned int buffer_capacity) :
		ostrbuf(buffer_capacity), _of(of)
	{ }
};

output_file::options output_file::default_options = {
	.perms = 0666,
	.max_size = 1 * 1024 * 1024 * 1024, /// 1GB
	.max_age = 0, /// Unlimited
	.max_count = 128,
	.buffer_capacity = 8192,
	.cpu = -1
};

std::string output_file::name() const
{
	// Actuall file name of the current file is unstable
	// because rotation thread can change it at any time.
	// We'd need to lock rotation mutex make a copy, etc.
	// For now just return the symlink name which should
	// be good enough for most apps.
	return _symlink;
}

unsigned int output_file::index() const
{
	return _name_index;
}

// Generate file name using prefix, index and suffix.
void output_file::update_name()
{
	std::ostringstream ss;
	ss << _name_pref
		<< std::setfill('0') << std::setw(_index_width) << _name_index
		<< std::setw(0) << _name_sufx;
	_name = ss.str();
}

// Update symlink.
// Creates a temp symlink that points to the new file and renames it
// on top of the old symlink.
void output_file::update_link()
{
	std::string link = _symlink + "$";
	remove(link.c_str());

	int err = symlink(_name.c_str(), link.c_str());
	if (err < 0)
		fprintf(stderr, "hogl::output_file: failed to create symlink %s -> %s. %s(%d)\n", 
			_name.c_str(), _symlink.c_str(), strerror(errno), errno);

	rename(link.c_str(), _symlink.c_str());
}

// Read the symlink and return the index it is pointing to.
// This is used to initialize the index when the file is reopened.
// If the symlink does not exist or is invalid the index starts from zero.
unsigned int output_file::read_link()
{
	unsigned int len = _symlink.size() + _index_width + 1;
	char cstr[len];

	ssize_t rlen = readlink(_symlink.c_str(), cstr, len);
	if (rlen < 0) {
		if (errno == ENOENT)
			return 0;
		fprintf(stderr, "hogl::output_file: failed to read symlink %s. %s(%d)\n",
			_symlink.c_str(), strerror(errno), errno);
		return 0;
	}

	std::string str(cstr, rlen);

	size_t pos;

	// Validate and strip prefix
	pos = str.find(_name_pref);
	if (pos != 0) {
		// Prefix does not match.
		// Looks like the link is pointing to something else.
		// Ignore, we'll fix it during update.
		return 0;
	}
	str = str.substr(_name_pref.size());

	// Validate and strip suffix
	if (_name_sufx.size()) {
		pos = str.find(_name_sufx);
		if (_name_sufx.size() && pos == str.npos) {
			// Suffix does not match.
			// Ignore, we'll fix it during update.
			return 0;
		}
		str = str.substr(0, pos);
	}

	// Convert to integer
	unsigned long index = strtoul(str.c_str(), NULL, 10);
	if (index == ULONG_MAX) {
		// Hmm, conversion failed. Ignore for now.
		return 0;
	}

	if (index >= _max_count) {
		// Index is outside valid range. User must've reused the 
		// name with different settings. Ignore and restart numbering.
		return 0;
	}

	if (++index >= _max_count)
		index = 0;

	return index;
}

// Format of the filename
//   /location/preffix.#.suffix
// # will be replaced by the file index
// Currently active file will have the following name
//   /location/prefix.suffix
output_file::output_file(const char *filename, format &fmt, const options &opts) :
		output(fmt),
		_max_size(opts.max_size), _max_count(opts.max_count), _mode(opts.perms),
		_fd(-1), _size(0),
		_running(false), _killed(false), _rotate_pending(false)
{
	pthread_mutex_init(&_write_mutex, NULL);
	pthread_mutex_init(&_rotate_mutex, NULL);
	pthread_cond_init(&_rotate_cond, NULL);

	// Start helper thread
	int err = pthread_create(&_rotate_thread, NULL, thread_entry, (void *) this);
	if (err) {
		fprintf(stderr, "hogl::output_file: failed to start helper thread. %d\n", err);
		abort();
	}
	err = setaffinity(_rotate_thread, opts.cpu);
	if(err) {
		fprintf(stderr, "hogl::output_file: failed to set affinity for helper thread. %d\n", err);
		abort();
	}

	const std::string str(filename);

	// Split file name into prefix and suffix
	size_t split = str.find('#');
	if (split != str.npos) {
		_name_sufx = str.substr(split + 1);
		_name_pref = str.substr(0, split);

		// Remove duplicate (if any) character from symlink
		if (split > 0 && str[split - 1] == str[split + 1])
			--split;
		_symlink = str.substr(0, split) + _name_sufx;
	} else {
		// No suffix
		_name_pref = str + ".";
		_symlink = str;
	}

	// Figure out index width for zero-padding
	unsigned int step = 10;
	_index_width = 1;
	while (step < _max_count) {
		step *= 10;
		_index_width++;
	}

	_name_index = read_link();
	update_name();

	_fd = open(_name.c_str(), O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, _mode);
	if (_fd < 0) {
		fprintf(stderr, "hogl::output_file: failed open file %s. %s (%d)\n", 
			_name.c_str(), strerror(errno), errno);
		abort();
	}

	output::init(new ostrbuf_file(*this, opts.buffer_capacity));

	// Write format header
	output::header(_name.c_str());

	update_link();
}

output_file::~output_file()
{
	_killed = true;

	// Wakeup rotation thread and wait for it to exit
	pthread_mutex_lock(&_rotate_mutex);
	pthread_cond_signal(&_rotate_cond);
	pthread_mutex_unlock(&_rotate_mutex);
	pthread_join(_rotate_thread, NULL);

	// Write format footer and flush the buffer
	output::footer();

	// Delete and zero out ostrbuf explicitly.
	// Just to make sure that parent doesn't touch this instance.
	delete _ostrbuf; _ostrbuf = 0;

	// Close current descriptor
	close(_fd);

	// Delete everything else
	pthread_mutex_destroy(&_write_mutex);
	pthread_mutex_destroy(&_rotate_mutex);
	pthread_cond_destroy(&_rotate_cond);
}

ssize_t output_file::writev(const struct iovec *iov, int iovcnt)
{
	ssize_t n;

	pthread_mutex_lock(&_write_mutex);

	n = ::writev(_fd, iov, iovcnt);
	if (n > 0) {
		_size += n;
		if (_size >= _max_size && !_rotate_pending) {
			// Try to wakeup rotation thread.
			// If we can't lock the mutex that means the rotation thread 
			// is still busy rotating things. So we'll just retry next 
			// time arround.
			if (pthread_mutex_trylock(&_rotate_mutex) == 0) {
				_rotate_pending = true;
				pthread_cond_signal(&_rotate_cond);
				pthread_mutex_unlock(&_rotate_mutex);
			}
		}
	}

	pthread_mutex_unlock(&_write_mutex);

	return n;
}

ssize_t output_file::write(const void *data, size_t size)
{
	struct iovec iv;
	iv.iov_base = (void *) data;
	iv.iov_len  = size;
	return this->writev(&iv, 1);
}

void output_file::do_rotate()
{
	int ofd = _fd;

	_name_index++;
	if (_name_index >= _max_count)
		_name_index = 0;
	update_name();

	// Open a new chunk
	int nfd = open(_name.c_str(), O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, _mode);
	if (nfd < 0) {
		// Retry later. Let the writer thread wake us up again.
		return;
	}

	// Generate header and write it out directly. We can't use our own write()
	// path here in the rotation thread, and we need to allocate a temporary 
	// buffer for the same reason.
	ostrbuf_fd nsb(nfd, 0, 128);
	_format.header(nsb, _name.c_str(), false /* not first */);
	nsb.flush();
	if (nsb.failed()) {
		// Failed to write. Retry later.
		close(nfd);
		return;
	}

	// Swap the fd
	pthread_mutex_lock(&_write_mutex);
	_fd   = nfd;
	_size = 0;
	pthread_mutex_unlock(&_write_mutex);

	//printf("rotate: switched fds - new %u old %u\n", nfd, ofd);

	update_link();

	// Generate footer, write it out and close the old chunk.
	// Note that we don't check for write errors here because we 
	// can't do much about them.
	ostrbuf_fd osb(ofd, 0, 128);
	_format.footer(osb, _name.c_str());
	osb.flush();
	close(ofd);
}

void *output_file::thread_entry(void *_self)
{
	output_file *self = (output_file *) _self;

	std::ostringstream ss;
	ss << "hogl::output_file helper (" << self->_symlink << ")";
	platform::set_thread_title(ss.str().c_str());

	// Run the loop
	self->thread_loop();
	return 0;
}

void output_file::thread_loop()
{
	_running = true;

	pthread_mutex_lock(&_rotate_mutex);

	while (1) {
		pthread_cond_wait(&_rotate_cond, &_rotate_mutex);
		if (_killed)
			break;

		if (_rotate_pending) {
			do_rotate();
			_rotate_pending = false;
		}
	}

	pthread_mutex_unlock(&_rotate_mutex);

	_running = false;
}

} // namespace hogl
