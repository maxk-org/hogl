dnl Process this file with autoconf to produce a configure script.

AC_INIT([hogl],[1.0])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([silent-rules subdir-objects])
AM_SILENT_RULES([yes])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
LT_INIT

dnl C++11 is manadatory
AX_CXX_COMPILE_STDCXX_11(noext, mandatory)

AC_SEARCH_LIBS(clock_gettime, rt)
AX_PTHREAD()

dnl Save cflags libs required for libhogl itself
dnl All cflags and libs added further down are needed only for tools and tests
HOGL_CXXFLAGS="$PTHREAD_CFLAGS"
HOGL_LIBS="$LIBS $PTHREAD_LIBS"
AC_SUBST(HOGL_LIBS)
AC_SUBST(HOGL_CXXFLAGS)

AC_ARG_ENABLE(debug,
     AS_HELP_STRING([--enable-debug],[Enable internal HOGL debug]),
     AC_DEFINE(HOGL_DEBUG)
)

AC_ARG_ENABLE(openmp,
     AS_HELP_STRING([--enable-openmp],[Enable OpenMP unit-test]),
     openmp=true
)
AM_CONDITIONAL([OPENMP], [test "$openmp" = "true"])

AC_ARG_WITH([tools],
    AS_HELP_STRING([--without-tools], [Do not build tools]))
AM_CONDITIONAL([BUILD_TOOLS], [test "$with_tools" != "no"])

AC_ARG_WITH([tests],
    AS_HELP_STRING([--without-tests], [Do not build tests]))
AM_CONDITIONAL([BUILD_TESTS], [test "$with_tests" != "no"])


AS_IF([test "$with_tools" != "no"], [
AC_CHECK_HEADER(bfd.h, [],
		[AC_MSG_ERROR([Couldn't find or include bfd.h, please install binutils-dev])])
])

AS_IF([test "$with_tools" != "no"], [ AC_SEARCH_LIBS(dlopen, dl) ])
AS_IF([test "$with_tools" != "no"], [ AC_SEARCH_LIBS(inotify_init, inotify) ])
AS_IF([test "$with_tools" != "no"], [ AC_SEARCH_LIBS(bfd_openr, bfd) ])

AX_CODE_COVERAGE()

dnl AX_BOOST_UNIT_TEST_FRAMEWORK
AS_IF([test "$with_tests" != "no"], [ AX_BOOST_BASE(1.40.0) ])

AC_SUBST(prefix)

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
