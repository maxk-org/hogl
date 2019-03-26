include $(top_srcdir)/acversion.mk
include $(top_srcdir)/acsubst.mk

ACLOCAL_AMFLAGS = -I.

pkgconfdir = $(libdir)/pkgconfig
plugindir = $(libdir)/plugin
pkgconf_DATA = hogl.pc

topsrc   = @abs_top_srcdir@
debbuild = $(topsrc)/deb-build
rpmbuild = $(topsrc)/rpm-build
rpmdir   = $(top_srcdir)/pkg
instdir  = $(top_srcdir)/usr/sbin

CODE_COVERAGE_OUTPUT_DIRECTORY = coverage
CODE_COVERAGE_BRANCH_COVERAGE = 1
@CODE_COVERAGE_RULES@

rpm: hogl.spec
	rm -rf $(rpmbuild)
	mkdir -p $(rpmdir)
	make DESTDIR=$(rpmbuild) install
	rpmbuild --buildroot=$(rpmbuild)     \
		--define='_rpmdir $(rpmdir)' \
		--define='_unpackaged_files_terminate_build 0' \
		-bb hogl.spec
	rm -rf $(rpmbuild)

builddeb: builddeb.in Makefile
	@echo Generating $@
	@$(acsubst) $< > $@

deb: builddeb
	sh builddeb $(debbuild)

#
# Headers
#
incdir    = $(includedir)/hogl
incdetdir = $(includedir)/hogl/detail
incplugdir = $(includedir)/hogl/plugin
inccapidir = $(includedir)/hogl/c-api
incdet_HEADERS = \
	include/hogl/detail/utils.hpp \
	include/hogl/detail/types.hpp \
	include/hogl/detail/magic.hpp \
	include/hogl/detail/preproc.hpp \
	include/hogl/detail/args.hpp \
	include/hogl/detail/argpack.hpp \
	include/hogl/detail/area.hpp \
	include/hogl/detail/mask.hpp \
	include/hogl/detail/bitmap.hpp \
	include/hogl/detail/barrier.hpp \
	include/hogl/detail/barrier-x86.hpp \
	include/hogl/detail/compiler.hpp \
	include/hogl/detail/record.hpp \
	include/hogl/detail/ringbuf.hpp \
	include/hogl/detail/post.hpp \
	include/hogl/detail/ostrbuf.hpp \
	include/hogl/detail/ostrbuf-fd.hpp \
	include/hogl/detail/ostrbuf-stdio.hpp \
	include/hogl/detail/ostrbuf-null.hpp \
	include/hogl/detail/tsobuf.hpp \
	include/hogl/detail/refcount.hpp \
	include/hogl/detail/engine.hpp \
	include/hogl/detail/internal.hpp \
	include/hogl/detail/timesource.hpp \
	include/hogl/detail/timestamp.hpp \
	include/hogl/detail/format.hpp \
	include/hogl/detail/output.hpp

incplug_HEADERS = \
	include/hogl/plugin/format.hpp
	
inccapi_HEADERS = \
	include/hogl/c-api/args.h \
	include/hogl/c-api/output.h \
	include/hogl/c-api/post.h \
	include/hogl/c-api/area.h \
	include/hogl/c-api/format.h \
	include/hogl/c-api/engine.h \
	include/hogl/c-api/mask.h \
	include/hogl/c-api/tls.h

inc_HEADERS = \
	include/hogl/area.hpp \
	include/hogl/mask.hpp \
	include/hogl/flush.hpp \
	include/hogl/post.hpp \
	include/hogl/ring.hpp \
	include/hogl/tls.hpp \
	include/hogl/timesource.hpp \
	include/hogl/engine.hpp \
	include/hogl/platform.hpp \
	include/hogl/output-null.hpp \
	include/hogl/output-stdout.hpp \
	include/hogl/output-stderr.hpp \
	include/hogl/output-textfile.hpp \
	include/hogl/output-file.hpp \
	include/hogl/output-pipe.hpp \
	include/hogl/format-null.hpp \
	include/hogl/format-basic.hpp \
	include/hogl/format-raw.hpp

# HOGL library dependencies
HOGL_LIBS = $(LIBFFI_LIBS) -lpthread -lrt

#
# Main library
#
lib_LTLIBRARIES = src/libhogl.la

src_libhogl_la_CXXFLAGS = -I include $(LIBFFI_CFLAGS) -pthread $(CODE_COVERAGE_CXXFLAGS)
src_libhogl_la_LIBADD   = $(HOGL_LIBS)
src_libhogl_la_LDFLAGS  = -version-info $(LIBRARY_VERSION) $(CODE_COVERAGE_LDFLAGS)
src_libhogl_la_SOURCES  = \
	src/utils.cc \
	src/area.cc \
	src/internal.cc \
	src/ringbuf.cc \
	src/tls.cc \
	src/engine.cc \
	src/timesource.cc \
	src/defring.cc \
	src/deftimesource.cc \
	src/platform.cc \
	src/post.cc \
	src/flush.cc \
	src/output.cc \
	src/ostrbuf.cc \
	src/ostrbuf-fd.cc \
	src/ostrbuf-stdio.cc \
	src/format-basic.cc \
	src/format-raw.cc \
	src/output-stdout.cc \
	src/output-stderr.cc \
	src/output-textfile.cc \
	src/output-file.cc \
	src/output-pipe.cc \
	src/c-api.cc

if CXX11_ENABLED
src_libhogl_la_SOURCES += src/mask.cc
else
src_libhogl_la_SOURCES += src/mask-boost.cc
endif

#
# Tests
#
if BUILD_TESTS
TESTS = \
	tests/affinity_test \
	tests/args_test \
	tests/area_test \
	tests/mask_test \
	tests/ring_test \
	tests/tso_test \
	tests/engine_test \
	tests/engine_cpu_core_test \
	tests/basic_test \
	tests/aux_engine_test \
	tests/rawrec_test \
	tests/defring_test \
	tests/stress_test \
	tests/ooo_test \
	tests/untouch_test \
	tests/c-api_test \
	tests/basic_test_c-api \
	tests/basic_test_c-api_cc \
	tests/format_basic_test \
	tests/ostrbuf_test

noinst_PROGRAMS = tests/disasm/test \
	$(TESTS)

plugin_LTLIBRARIES = tests/libfmt-plugin_test.la

AM_CPPFLAGS = -I include $(BOOST_CPPFLAGS)
AM_CXXFLAGS = -pthread
AM_CFLAGS   = -pthread
LDADD = src/libhogl.la $(HOGL_LIBS) $(BOOST_LDFLAGS) -lboost_regex $(CODE_COVERAGE_LDFLAGS)

tests_affinity_test_SOURCES  = tests/affinity_test.cc 
tests_args_test_SOURCES  = tests/args_test.cc 
tests_area_test_SOURCES  = tests/area_test.cc
tests_mask_test_SOURCES  = tests/mask_test.cc
tests_ring_test_SOURCES  = tests/ring_test.cc
tests_tso_test_SOURCES   = tests/tso_test.cc
tests_engine_test_SOURCES= tests/engine_test.cc
tests_engine_cpu_core_test_SOURCES= tests/engine_cpu_core_test.cc
tests_basic_test_SOURCES = tests/basic_test.cc
tests_aux_engine_test_SOURCES = tests/aux_engine_test.cc
tests_rawrec_test_SOURCES = tests/rawrec_test.cc
tests_defring_test_SOURCES = tests/defring_test.cc
tests_stress_test_SOURCES = tests/stress_test.cc
tests_ooo_test_SOURCES   = tests/ooo_test.cc
tests_untouch_test_SOURCES   = tests/untouch_test.cc
tests_c_api_test_SOURCES = tests/c-api_test.cc
tests_format_basic_test_SOURCES = tests/format_basic_test.cc
tests_ostrbuf_test_SOURCES = tests/ostrbuf_test.cc
tests_basic_test_c_api_cc_SOURCES = tests/basic_test_c-api_cc.cc

tests_basic_test_c_api_SOURCES = tests/basic_test_c-api.c
nodist_EXTRA_tests_basic_test_c_api_SOURCES = dummy.cc # force CXXLINK

if OPENMP
TESTS += tests/openmp_test
tests_openmp_test_SOURCES = tests/openmp_test.cc
tests_openmp_test_CXXFLAGS = $(CXXFLAGS) -fopenmp -pthread
tests_openmp_test_LDFLAGS = -fopenmp
endif

tests_disasm_test_SOURCES = \
	tests/disasm/record.cc \
	tests/disasm/post.cc \
	tests/disasm/c-api-post.c \
	tests/disasm/bitmap.cc \
	tests/disasm/ring.cc \
	tests/disasm/tls.cc \
	tests/disasm/main.cc

tests_libfmt_plugin_test_la_CXXFLAGS = -I include $(LIBFFI_CFLAGS) -pthread
tests_libfmt_plugin_test_la_LIBADD   = $(HOGL_LIBS)
tests_libfmt_plugin_test_la_SOURCES  = \
	tests/fmt-plugin_test.cc
endif

#
# Tools
#
if BUILD_TOOLS
bin_PROGRAMS = tools/hogl-recover tools/hogl-cook
bin_SCRIPTS  = tools/hogl-grep

tools_hogl_recover_SOURCES  = \
		tools/coredump.cc tools/coredump.hpp tools/ring-validator.hpp \
		tools/recovery-engine.hpp tools/recovery-engine.cc \
		tools/recover.cc
tools_hogl_recover_CPPFLAGS = -I include
tools_hogl_recover_LDADD    = src/libhogl.la -lboost_regex $(HOGL_LIBS) $(LIBS)

tools_hogl_cook_SOURCES  = \
		tools/rdbuf.hpp tools/rdbuf.cc \
		tools/raw-parser.hpp tools/raw-parser.cc \
		tools/cook.cc
tools_hogl_cook_CPPFLAGS = -I include
tools_hogl_cook_LDADD    = src/libhogl.la -lboost_regex $(HOGL_LIBS) $(LIBS)
endif
