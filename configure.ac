#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([calf], [2.0], [fmang+calf@mg0.fr])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([calf.c])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC_C99
AC_PROG_RANLIB

AC_CHECK_LIB([fcgi], [FCGI_Accept], [], [AC_MSG_ERROR([unable to find libfcgi])])
PKG_CHECK_MODULES([SYSTEMD_DAEMON], [libsystemd-daemon])

AC_CHECK_HEADERS(
	[unistd.h],
	[], [AC_MSG_ERROR([critical header missing])]
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
