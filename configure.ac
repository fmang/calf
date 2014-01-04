#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([calf], [2.0], [fmang+calf@mg0.fr])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/calf.c])
AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([timers],
	AS_HELP_STRING([--enable-timers], [measure performance]),
	[AC_DEFINE([USE_TIMERS], [1], [Display timings.])])

AC_ARG_WITH([fcgi],
	AS_HELP_STRING([--without-fcgi], [support for FastCGI]),
	[], [with_fcgi=yes])

AC_ARG_WITH([systemd],
	AS_HELP_STRING([--with-systemd], [support for systemd's socket activation]),
	[], [with_systemd=no])

AS_IF([test "x$with_fcgi" = 'xno' -a "x$with_systemd" = 'xyes'], [
	AC_MSG_ERROR([cannot enable socket activation without FastCGI support (please use --with-fcgi)])
])

AC_PROG_CC_C99
AC_PROG_RANLIB

PKG_CHECK_MODULES([URIPARSER], [liburiparser])

AS_IF([test "x$with_fcgi" = 'xyes'], [
	AC_CHECK_LIB([fcgi], [FCGI_Accept], [], [AC_MSG_ERROR([unable to find libfcgi])])
	AC_CHECK_HEADERS([fcgi_stdio.h], [], [AC_MSG_ERROR([missing fcgi_stdio.h])])
])

AS_IF([test "x$with_systemd" = 'xyes'], [
	PKG_CHECK_MODULES([SYSTEMD_DAEMON], [libsystemd-daemon], [AC_DEFINE([HAVE_SYSTEMD], [1], [Support for systemd's socket activation.])])
])

AC_CHECK_HEADERS(
	[unistd.h],
	[], [AC_MSG_ERROR([critical header missing])]
)

AC_CONFIG_FILES([Makefile src/Makefile share/Makefile])
AC_OUTPUT
