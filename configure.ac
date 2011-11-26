#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([zerovm], [0.6], [http://github.com/Dazo-org])
AM_INIT_AUTOMAKE([0.6])

AC_MSG_CHECKING([for debug flags])
debug="yes"
AC_ARG_ENABLE([debug], 
	[AS_HELP_STRING(--enable-debug, [Enable debugging symbols and assertions (yes)])],
	[debug="$enableval"])
if test "x$debug" = "xyes"; then
         CFLAGS=" -g -DDEBUG"
         CXXFLAGS=" -g -DDEBUG"
AC_MSG_RESULT([yes])
else
	CFLAGS=" -DNDEBUG -O2"
	CXXFLAGS=" -DNDEBUG -O2"
AC_MSG_RESULT([no])
fi
AC_CANONICAL_SYSTEM
UNSUPPORTED_OS="zerovm is currently supported on linux x86_64 only"
case "${target}" in
  x86_64-*-linux-gnu)
    echo ============================================================
    echo Setting up build environment for ${target_cpu} - ${target_os}
    echo ============================================================
    ;;
  *)
    AC_MSG_ERROR($UNSUPPORTED_OS)
  esac


#AC_CONFIG_SRCDIR([include/elf_constants.h])
#AC_CONFIG_HEADERS([config.h])
AH_TEMPLATE([NACL_LINUX],[Nacl Linux flavor])
AH_TEMPLATE([NACL_BLOCK_SHIFT],[Gio mem layout block shift])
AH_TEMPLATE([NACL_BLOCK_SIZE],[Gio mem layout block size])
AH_TEMPLATE([NACL_BUILD_ARCH],[Build architecture (arm, x86)])
AH_TEMPLATE([NACL_BUILD_SUBARCH],[Build architecture flavor (32, 64)])
AH_TEMPLATE([NACL_TARGET_ARCH],[Target architecture (arm, x86)])
AH_TEMPLATE([NACL_TARGET_SUBARCH],[Target architecture flavor (32, 64)])
AH_TEMPLATE([NACL_STANDALONE],[Standalone executable])
AH_TEMPLATE([__STDC_LIMIT_MACROS],[])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
#AC_PROG_RANLIB
AM_PROG_AS

# Checks for libraries.
AC_CHECK_LIB([crypto], [main],, [AC_MSG_ERROR([Required library "libcrypto" was not found, try: sudo apt-get install libssl-dev])])
AC_CHECK_LIB([dl], [main],, [AC_MSG_ERROR([Required library "libdl" was not found])])
#AC_CHECK_LIB([gtest], [main],, [AC_MSG_ERROR([Required library "libgtest" was not found, try: sudo apt-get install libgtest-dev])])
AC_CHECK_LIB([pthread], [main],, [AC_MSG_ERROR([Required library "libpthread" was not found])])
AC_CHECK_LIB([rt], [main],, [AC_MSG_ERROR([Required library "librt" was not found])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h malloc.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([dup2 ftruncate gettimeofday localtime_r memmove memset mkdir munmap rmdir strcasecmp strchr strdup strerror strndup strpbrk strrchr strstr strtol strtoul uname])
#AC_SYS_LARGEFILE

AC_DEFINE([NACL_LINUX])
AC_DEFINE([NACL_BLOCK_SHIFT],[5])
AC_DEFINE([NACL_BLOCK_SIZE],[32])
AC_DEFINE([NACL_BUILD_ARCH],[x86])
AC_DEFINE([NACL_BUILD_SUBARCH],[64])
AC_DEFINE([NACL_TARGET_ARCH],[x86])
AC_DEFINE([NACL_TARGET_SUBARCH],[64])
AC_DEFINE([NACL_STANDALONE],[1])
AC_DEFINE([__STDC_LIMIT_MACROS],[1])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
