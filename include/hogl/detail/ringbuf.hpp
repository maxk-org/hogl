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
 * @file hogl/detail/ringbuf.h
 * Ring buffer. Single reader/single writer, circular fifo buffer.
 */

#ifndef HOGL_DETAIL_RINGBUF_HPP
#define HOGL_DETAIL_RINGBUF_HPP

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#include <iostream>

#include <hogl/detail/compiler.hpp>
#include <hogl/detail/barrier.hpp>
#include <hogl/detail/refcount.hpp>
#include <hogl/detail/record.hpp>
#include <hogl/detail/magic.hpp>
#include <hogl/detail/args.hpp>
#include <hogl/detail/preproc.hpp>
#include <hogl/detail/timesource.hpp>

namespace hogl {

class engine;
class recovery_engine;

/**
 * Ring buffer. Simple and efficient circular fifo.
 * Ring capacity is restricted to the powers of two. One item is always 
 * reserved for internal operation.
 * Size of the ring record is also restricted to the powers of two and
 * is adjusted automatically.
 */
class ringbuf {
protected:
	// Engine needs to muck with some fields like priority
	// that are not normally exposed.
	friend class engine;
	friend class recovery_engine;

	typedef volatile unsigned int vo_uint;

	// Magic number
	magic           _magic;
	char           *_name;
	unsigned int    _flags;
	int             _prio;

	// Record buffers (top addr, index shift, tailroom)
	uint8_t        *_rec_top;
	unsigned int    _rec_shift;
	unsigned int    _rec_tailroom;

	// Ring capacity
	// After initialization this is set to the total number of records - 1
	unsigned int    _capacity;

	// The number of references to this object
   	mutable refcount _refcnt;

	// Protects push in shared rings
   	pthread_mutex_t _mutex;

	// Timesource
	hogl::timesource* volatile _timesource;

	// R/W access by the writer
	// R/O access by the reader
	vo_uint         _tail __attribute__ ((aligned(64)));
	uint64_t        _seqnum;
	uint64_t        _dropcnt;

	// R/W access by the reader
	// R/O access by the writer
	vo_uint         _head;

	/**
 	 * Get a pointer the record with a specific index 
 	 * @param i record index.
 	 * @return record pointer
 	 */
	record* get_record(unsigned int i)
	{
		return (record *) (_rec_top + (i << _rec_shift));
	}

	/**
 	 * Get index from the record pointer
 	 * @param r record pointer
 	 * @return record index
 	 */
	unsigned int get_index(record *r)
	{
		return ((uint8_t *) r - _rec_top) >> _rec_shift;
	}

	/**
	 * Commit head index.
	 * Readers interface.
	 */
	void commit_head(unsigned int head, bool bar = true)
	{
		// Barrier is needed to make sure that we finished reading the records
		// before moving the head.
		// if() statement is evaluated at the compile time.
		if (bar)
			barrier::memr();

		_head = head;
	}

	/**
	 * Commit tail index.
	 * Writer's interface.
	 */
	void commit_tail(unsigned int tail, bool bar = true)
	{
		// Barrier is needed to make sure that records are updated
		// before they are made available to the reader.
		// if() statement is evaluated at the compile time.
		if (bar)
			barrier::memw();

		_tail = tail;
	}

public:
	/**
	 * Ring flags.
	 * Shared rings can be shared by multiple threads. Push operations
	 * will take a mutex in that case.
	 *
	 * Immortal rings will not be deleted after the last reference goes
	 * away. This is primarily needed for the deault ring that can never
	 * be deleted. 
	 *
	 * Reusable rings can be reused after the original owner releases the
	 * reference. This is useful in the case where a frequently restarted 
	 * thread wants to create TLS ring with the name that it used before, 
	 * which means that the original ring may still be registered with 
	 * the engine.
	 */
	enum flags {
		/** Shared ring */
		SHARED     = (1<<0),

		/** Must not be deleted */
		IMMORTAL   = (1<<1),

		/** Can be reused */
		REUSABLE   = (1<<2)
	};

	enum {
		NOBARRIER = false,
		BARRIER   = true
	};

	/**
 	 * Priority ceiling
 	 */
	enum {
		PRIORITY_CEILING = 9999
	};

	/**
 	 * Ring options
 	 */
	struct options {
		unsigned int capacity;
		unsigned int prio;
		unsigned int flags;
		unsigned int record_tailroom;
	};

	static options default_options;

	/**
	 * Allocate the ring
	 */
	ringbuf(const char *name, const options &opts = default_options);

	/**
	 * Destroy the ring
	 * @warn Call release() instead. Destructor will assert if 
	 * it's called for the ringbuf that is still in use.
	 */
	~ringbuf();

	/**
	 * Reset the ring.
	 * This resets ring indices (dropping all records), sequence number and
	 * drop count.
	 */
	void reset(void);

	/**
	 * Get ring name
	 * @return ring name
	 */
	const char *name() const { return _name; }

	/**
	 * Get record size
	 * @return size of the record in bytes
	 */
	unsigned int record_size() const { return (1 << _rec_shift); }

	/**
	 * Get number of bytes allocated for user data at the tail of each record
	 * @return record tail room in bytes
	 */
	unsigned int record_tailroom() const { return _rec_tailroom; }

	/**
	 * Get ring capacity
	 * @return max number of ring items
	 */
	unsigned int capacity() const { return _capacity + 1; }

	/**
	 * Get number of items in the ring
	 * @return number of items in the ring
	 */
	unsigned int size() const { return (_tail - (_head + 1)) & _capacity; }

	/**
	 * Get available room.
	 * @return number that can be ring
	 */
	unsigned int room() const { return (_head - _tail) & _capacity; }

	/**
	 * Check if the ring is empty.
	 * @return true if ring is empty, false otherwise.
	 */
	bool empty() const { return ((_head + 1) & _capacity) == _tail; }

	/**
         * Get priority of this ring
	 */
	int prio() const { return _prio; }

	/**
         * Get number of dropped messages 
	 */
	uint64_t dropcnt() const { return _dropcnt; }

	/**
         * Get current sequence number
	 */
	uint64_t seqnum() const { return _seqnum; }

	/**
	 * Check if the ring is an orphan
	 */
	bool orphan() const { return _refcnt.get() == 1; }

	/**
	 * Check if the ring is shared 
	 */
	bool shared() const { return _flags & SHARED; }

	/**
	 * Check if the ring must never be deleted
	 */
	bool immortal() const { return _flags & IMMORTAL; }

	/**
	 * Check if the ring must never be deleted
	 */
	bool reusable() const { return _flags & REUSABLE; }

	/**
	 * Get the number of references to this ring
	 */
	int refcnt() const { return _refcnt.get(); }

	/**
	 * Increment sequence number and return the original value
	 */ 
	uint64_t inc_seqnum() { return _seqnum++; }

	/**
	 * Increment drop count
	 */ 
	void inc_dropcnt() { _dropcnt++; }

	/**
	 * Get the timestamp
	 */
	hogl::timestamp timestamp() const 
	{
		hogl::timesource *ts = _timesource;
		return ts->timestamp();
	}

	/**
	 * Set timesource for this ring
	 */
	void timesource(hogl::timesource *ts);

	/**
	 * Get timesource for this ring
	 */
	hogl::timesource* timesource() const
	{
		return _timesource;
	}

	/**
	 * Hold a reference to ringbuf. Increments refcount.
	 */
	ringbuf* hold()
	{
		_refcnt.inc();
		return this;
	}

	/**
	 * Release a reference. Decrements refcount and deletes
	 * the object if needed.
	 */ 
	void release() const
	{
		int r = _refcnt.dec();
		if (r < 0) abort();
		if (r > 0) return;
		if (!immortal())
			delete this;
	}

	/**
	 * Lock the ring buffer for exclusive access.
	 * This operation is a noop for non-shared rings.
	 */
	void lock()
	{
		if (shared())
			pthread_mutex_lock(&_mutex);
	}

	/**
	 * Unlock the ring buffer.
	 * This operation is a noop for non-shared rings.
	 */
	void unlock()
	{
		if (shared())
			pthread_mutex_unlock(&_mutex);
	}

	// -------- Writer interface ---------
	/**
	 * Begin push transaction. Grabs tail item. Writer's interface.
	 * @param data pointer to the data
	 * @return pointer to the tail element
	 */
	record *push_begin()
	{
		return get_record(_tail);
	}

	/**
	 * Commit push transaction. Increments drop count if the ring is full.
	 * Writer's interface.
	 */
	void push_commit(bool bar = true)
	{
		unsigned int t = _tail;
		if (hogl_unlikely(_head == t)) {
			inc_dropcnt();
			return;
		}
		commit_tail((t + 1) & _capacity, bar);
	}

	// -------- Reader interface ---------

	/**
	 * Begin pop transaction. 
	 * Get the pointer to the next item from the head of the ring without removing it. 
	 * Reader's interface.
	 * @return pointer to the head record
	 */
	record *pop_begin()
	{
		unsigned int h = (_head + 1) & _capacity;
		if (h == _tail)
			return 0;
		return get_record(h);
	}

	/**
	 * Commit pop transaction. Reader's interface.
	 */
	void pop_commit(bool bar = true)
	{
		commit_head((_head + 1) & _capacity, bar);
	}

	friend class pop_iterator;

	/**
	 * Simple ring iterator.
	 * Iterates over ring items from head to tail.
	 * Reader interface.
	 */
	class pop_iterator {
	private:
		unsigned int _head;
		unsigned int _tail;
		ringbuf     *_ring;

	public:
		/**
		 * Get the pointer to the next record.
		 * @return null if there are no more records to pop
		 */
		record *next()
		{
			unsigned int h = (_head + 1) & _ring->_capacity;
			if (h == _tail)
				return 0;
			record *r = _ring->get_record(h);
			_head = h;
			return r;
		}

		/**
 		 * Rewind iterator to a particular location.
 		 * param r record pointer. The iterator will point to the next record after r.
 		 */
		void rewind(record *r)
		{
			_head = _ring->get_index(r);
		}

		/**
		 * Commits iterator actions (moves head index). 
		 */
		void commit(bool bar = true)
		{
			_ring->commit_head(_head, bar);
		}

		/**
		 * Reset iterator to the current ring state
		 */	 
		void reset()
		{
			_head = _ring->_head;
			_tail = _ring->_tail;
		}

		/**
		 * Get ring pointer
		 */
		ringbuf *ring() { return _ring; }

		/**
		 * Get const ring pointer
		 */
		const ringbuf *const_ring() const { return _ring; }

		/**
		 * Set ring pointer. Resets the iterator.
		 */
		void ring(ringbuf *r)
		{ 
			_ring = r;
			if (!r)
				_head = _tail = 0;
			else
				reset();
		}

		/**
		 * Check if iterator is valid
		 */
		bool valid() const { return _ring; }

		/**
		 * Invalidate the iterator
		 */
		void invalidate() { ring(0); }

		/**
		 * Allocate and initialize the interator.
		 * @param r pointer to the ring object to iterate over
		 */
		pop_iterator(ringbuf *r) : _ring(r) { reset(); }
		pop_iterator() : _head(0), _tail(0), _ring(0) { }
	};

	// Custom new/delete to make make sure ringbufs are always aligned to 64 bytes (most common cacheline size)
	static void* operator new(size_t s);
	static void operator delete(void *p);

private:
	// No copies
	ringbuf(const ringbuf&);
	ringbuf& operator=( const ringbuf& );
};

std::ostream& operator<< (std::ostream& s, const ringbuf& ring);

} // namespace hogl

#endif // HOGL_DETAIL_RING_HPP
