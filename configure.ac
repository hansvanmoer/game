#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([game], [0.0.1], [hansvm.projects@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([deque.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([sqrt], [m], [], [AC_MSG_ERROR([unable to find math library])])

# Checks for header files.
AC_CHECK_HEADERS([stdbool.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
