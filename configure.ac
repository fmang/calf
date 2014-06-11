#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([calf], [2.0], [fmang+calf@mg0.fr])
AM_INIT_AUTOMAKE([foreign no-exeext])
AC_CONFIG_SRCDIR([src/calf.c])
AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([timers],
	AS_HELP_STRING([--enable-timers], [measure performance]),
	[AC_DEFINE([USE_TIMERS], [1], [Display timings.])])

AC_ARG_WITH([fcgi],
	AS_HELP_STRING([--with-fcgi], [support for FastCGI]),
	[], [with_fcgi=check])

AC_ARG_WITH([systemd],
	AS_HELP_STRING([--with-systemd], [support for systemd's socket activation]),
	[], [with_systemd=check])

AC_PROG_CC_C99
AC_PROG_LN_S
AC_PROG_RANLIB

AC_CHECK_HEADERS(
	[unistd.h obstack.h],
	[], [AC_MSG_ERROR([critical header missing])]
)

PKG_CHECK_MODULES([URIPARSER], [liburiparser])

have_fcgi=no
AS_IF([test "x$with_fcgi" != xno], [
	have_fcgi=yes
	AC_CHECK_LIB([fcgi], [FCGI_Accept], [], [have_fcgi=no])
	AC_CHECK_HEADERS([fcgi_stdio.h], [], [have_fcgi=no])
])
AS_IF([test "x$with_fcgi" = xyes -a "x$have_fcgi" = xno], [
	AC_MSG_ERROR([cannot find libfcgi])
])
AM_CONDITIONAL(FASTCGI, [test "x$have_fcgi" = xyes])

have_systemd=no
AS_IF([test "x$have_fcgi" = xno -a "x$with_systemd" = xyes], [
	AC_MSG_ERROR([cannot enable socket activation without FastCGI support (please use --with-fcgi)])
])
AS_IF([test "x$have_fcgi" = xyes -a "x$with_systemd" != xno], [
	PKG_CHECK_MODULES([SYSTEMD_DAEMON], [libsystemd],
		[have_systemd=yes
		AC_DEFINE([HAVE_SYSTEMD], [1], [Support for systemd's socket activation.])],
		[])
])
AS_IF([test "x$with_systemd" = xyes -a "x$have_systemd" = xno], [
	AC_MSG_ERROR([cannot find libsystemd])
])
AM_CONDITIONAL([SYSTEMD], [test "x$have_systemd" = xyes])

AC_CONFIG_FILES([Makefile src/Makefile share/Makefile systemd/Makefile])
AC_OUTPUT

echo
echo '--- Summary ---'
echo "FastCGI support: $have_fcgi"
echo "systemd support: $have_systemd"
echo
