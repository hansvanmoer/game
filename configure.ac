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
AC_SEARCH_LIBS([png_create_write_struct], [png], [], [AC_MSG_ERROR([unable to find libpng library])])
AC_SEARCH_LIBS([pthread_create], [pthread], [], [AC_MSG_ERROR([unable to find POSIX compatible threading library])])
AC_SEARCH_LIBS([iconv_open], [iconv], [], [AC_MSG_ERROR([unable to find iconv library])])

# Checks for header files.
AC_CHECK_HEADERS([assert.h dirent.h getopt.h iconv.h limits.h netdb.h netinet/in.h stdbool.h stdio.h stdlib.h png.h pthread.h signal.h string.h sys/socket.h  sys/types.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
