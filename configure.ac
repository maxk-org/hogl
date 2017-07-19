dnl Process this file with autoconf to produce a configure script.

AC_INIT(hogl, 1.0)

AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([silent-rules subdir-objects])
AM_SILENT_RULES([yes])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AX_CXX_COMPILE_STDCXX_11(noext, optional)
AM_CONDITIONAL([CXX11_ENABLED], [test x$HAVE_CXX11 = x1])

AC_ARG_ENABLE(debug,
     AC_HELP_STRING([--enable-debug], [Enable internal HOGL debug]),
     AC_DEFINE(HOGL_DEBUG)
)

AC_ARG_ENABLE(openmp,
     AC_HELP_STRING([--enable-openmp], [Enable OpenMP unit-test]),
     openmp=true
)
AM_CONDITIONAL([OPENMP], [test x$openmp = xtrue])

AC_ARG_WITH([tools],
    AS_HELP_STRING([--without-tools], [Do not build tools]))
AM_CONDITIONAL([BUILD_TOOLS], [test "x$with_tools" != "xno"])

AC_ARG_WITH([tests],
    AS_HELP_STRING([--without-tests], [Do not build tests]))
AM_CONDITIONAL([BUILD_TESTS], [test "x$with_tests" != "xno"])

AS_IF([test "x$with_tools" != "xno"], [
AC_CHECK_HEADER(bfd.h,
		[],
		[AC_MSG_ERROR([Couldn't find or include bfd.h, please install binutils-dev])])
])

PKG_CHECK_MODULES([LIBFFI], [libffi >= 3.0.5])

AC_SEARCH_LIBS(dlopen, dl)

AS_IF([test "x$with_tools" != "xno"], [
AC_SEARCH_LIBS(bfd_openr, bfd, [], [
	unset ac_cv_search_bfd_openr
	AC_SEARCH_LIBS(bfd_openr, bfd,
			[LIBS="$LIBS -L/usr/local/lib -liberty -lz"],
			[AC_MSG_ERROR(Couldn't find usable libbfd)],
			[-L/usr/local/lib -liberty -lz])
])
])

AC_SEARCH_LIBS(inotify_init, inotify)

AX_CODE_COVERAGE()

AS_IF([test "x$with_tests" != "xno"], [
AX_BOOST_BASE(1.40.0)
dnl AX_BOOST_UNIT_TEST_FRAMEWORK
])

AC_SUBST(prefix)

AC_OUTPUT([
    Makefile
])
