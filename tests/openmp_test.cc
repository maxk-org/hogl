#include <stdio.h>
#include <unistd.h>
#include <omp.h>
#include <sys/prctl.h>

#include "hogl/engine.hpp"
#include "hogl/post.hpp"
#include "hogl/area.hpp"
#include "hogl/mask.hpp"
#include "hogl/tls.hpp"
#include "hogl/output-stderr.hpp"
#include "hogl/format-basic.hpp"

// This test is basically an example of how to use HOGL with OpenMP.
// Nothing special needs to be done for the default configuration
// where OpenMP threads just use the default shared ring. However for 
// best performance and contention avoidance each OMP thread needs 
// its own ring. This test demonstrates how that can be achieved.

// Log area for the test
static const hogl::area* log_area;

// Per thread pointer to the hogl::tls object allocated
// by the OMP threads
static __thread hogl::tls *__omp_tls = 0;

// OMP thread init function.
void init_omp_thread()
{
	// Set thread name (not required for HOGL).
	char thread_name[128];
	sprintf(thread_name, "omp-thread-%d", omp_get_thread_num());
	prctl(PR_SET_NAME, thread_name);

	// Ring options.
	// Ring is marked as reusable so that it can be reused if the
	// OMP thread is restarted.
	hogl::ringbuf::options ro = {
		capacity: 2048,
		prio:     0,
		flags:    hogl::ringbuf::REUSABLE
	};

	// Generate ring name based on the OMP thread id
	char ring_name[128];
	sprintf(ring_name, "OMP%d", omp_get_thread_num());

	// Allocate TLS object for this thread.
	// The constructor will allocate a new ringbuffer and replace
	// the default.
	__omp_tls = new hogl::tls(ring_name, ro);

	hogl::post(log_area, log_area->INFO, "initialized %s", thread_name);
}

// OMP thread deinit function.
void deinit_omp_thread()
{
	char thread_name[128];
	prctl(PR_GET_NAME, thread_name);
	hogl::post(log_area, log_area->INFO, "deinitialized %s", thread_name);

	// Delete OMP object allocated in the init function.
	// This will set ring pointer back to default.
	delete __omp_tls;
}

void section(unsigned int s)
{
	hogl::post(log_area, log_area->INFO, "section %u: thread %u", s, omp_get_thread_num());
}

void block(unsigned int b)
{
	hogl::post(log_area, log_area->INFO, "block %u", b);

	#pragma omp parallel sections
 	{
		#pragma omp section 
		{
			section(0);
		}

		#pragma omp section
		{
			section(1);
		}
	}
}

int main(void)
{

	// Basic HOGL init
	hogl::format_basic  logfmt("timespec,timedelta,ring,seqnum,area,section");
	hogl::output_stderr logout(logfmt);

	hogl::activate(logout);
	log_area = hogl::add_area("OMP");

	// OMP settings.
	// Note that HOGL will work with dynamic scheduling as well.
	// However is not possible to ensure that each thread will get
	// its own ring in that case. 
	omp_set_num_threads(4);
	omp_set_dynamic(0);

	// Start OMP threads and run init function in each
	#pragma omp parallel
		init_omp_thread();

	block(0);

	block(1);

	// Run deinit function in each thread.
	// This is required to deallocate the rings properly otherwise
	// HOGL engine will get stuck waiting for the rings to be released.
	#pragma omp parallel
		deinit_omp_thread();

	// Stop HOGL engine
	hogl::deactivate();

	return 0;
}
