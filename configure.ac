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

AC_ARG_ENABLE(debug,
     AC_HELP_STRING([--enable-debug], [Enable internal HOGL debug]),
     AC_DEFINE(HOGL_DEBUG)
)

AC_ARG_ENABLE(openmp,
     AC_HELP_STRING([--enable-openmp], [Enable OpenMP unit-test]),
     openmp=true
)
AM_CONDITIONAL([OPENMP], [test x$openmp = xtrue])

AC_CHECK_HEADER(bfd.h,
		[],
		[AC_MSG_ERROR([Couldn't find or include bfd.h, please install binutils-dev])])

PKG_CHECK_MODULES([LIBFFI], [libffi >= 3.0.5])

AC_SEARCH_LIBS(dlopen, dl)

AC_SEARCH_LIBS(bfd_openr, bfd, [], [
	unset ac_cv_search_bfd_openr
	AC_SEARCH_LIBS(bfd_openr, bfd,
			[LIBS="$LIBS -L/usr/local/lib -liberty -lz"],
			[AC_MSG_ERROR(Couldn't find usable libbfd)],
			[-L/usr/local/lib -liberty -lz])
])

AC_SEARCH_LIBS(inotify_init, inotify)

AX_CODE_COVERAGE()

AX_BOOST_BASE(1.40.0)
dnl AX_BOOST_UNIT_TEST_FRAMEWORK

AC_SUBST(prefix)

AC_OUTPUT([
    Makefile
])
