/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  IMC API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_NACL_SYSCALLS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_NACL_SYSCALLS_H_

/**
 * @file
 * Defines <a href="group__syscalls.html">Service Runtime Calls</a>.
 * The ABI is implicitly defined.
 *
 * @addtogroup syscalls
 * @{
 */

#include <sys/types.h>  /* off_t */
#include <sys/stat.h>
#include <time.h>  /* clock_t, to be deprecated */
                   /* TODO(bsy): newlib configuration needs to be customized to
                    *            our own ABI. */

#ifdef __cplusplus
extern "C" {
#endif

struct timeval;  /* sys/time.h */
struct timezone;
struct NaClImcMsgHdr;  /* sys/nacl_imc_api.h */

/*
 * TODO(mseaborn): Many of the functions here are defined in other,
 * standard headers, e.g. mmap() is defined in <sys/mman.h>.  We use
 * "#ifndef __GLIBC__" so that these definitions do not conflict with
 * glibc's definitions, but eventually we should remove the
 * conflicting definitions in this file.
 */

/**
 *  @nacl
 *  A stub library routine used to evaluate syscall performance.
 */
extern void null_syscall(void);
/**
 * @nacl
 * Set or get the name service connection capability.
 *
 * @param name_desc_in_out If not -1, the name service connection
 * capability is set to this value; if it is -1, then it is set to the
 * current connection capability value.
 */
extern int nacl_nameservice(int *name_desc_in_out);
/**
 *  @nqPosix
 *  Opens a file descriptor from a given pathname.
 *
 *  @param pathname The posix pathname (e.g., /tmp/foo) to be opened.
 *  @param flags The access modes allowed.  One of O_RDONLY, O_WRONLY, or
 *  O_RDWR is required.  Otherwise, only O_CREAT, O_TRUNC, and O_APPEND are
 *  supported.
 *  @param mode (optional) Specifies the permissions to use if a new file is
 *  created.  Limited only to user permissions.
 *  @return Returns non-negative file descriptor if successful.
 *  If open fails, it returns -1 and sets errno appropriately.
 */
#ifndef __GLIBC__
extern int open(char const *pathname, int flags, ...);
#endif
/**
 *  @posix
 *  Closes the specified file descriptor.
 *  @param desc The file descriptor to be closed.
 *  @return Open returns zero on success.  On error, it returns -1 and sets
 *  errno appropriately.
 */
#ifndef __GLIBC__
extern int close(int desc);
#endif
/**
 *  @posix
 *  Reads count bytes from the resource designated by desc
 *  into the buffer pointed to by buf.
 *  @param desc The file descriptor to be read from
 *  @param buf Where the read data is to be written
 *  @param count The number of bytes to be read
 *  @return On success, read returns the number of bytes read.  On failure,
 *  returns -1 and sets errno appropriately.
 */
#ifndef __GLIBC__
extern int read(int desc, void *buf, size_t count);
#endif
/**
 *  @posix
 *  Writes count bytes from the buffer pointed to by buf to the
 *  resource designated by desc.
 *  @param desc The file descriptor to be written to
 *  @param buf Where the read data is to be read from
 *  @param count The number of bytes to be written
 *  @return On success, write returns the number of bytes written.  On failure,
 *  returns -1 and sets errno appropriately.
 */
#ifndef __GLIBC__
extern int write(int desc, void const *buf, size_t count);
#endif
/**
 *  @posix
 *  Moves the file offset associated with desc.
 *  @param desc The file descriptor
 *  @param offset If whence is SEEK_SET, the descriptor offset is set to this
 *  value.  If whence is SEEK_CUR, the descriptor offset is set to its current
 *  location plus this value.  If whence is SEEK_END, the descriptor offset is
 *  set to the size of the file plus this value.
 *  @param whence Directive for setting the descriptor offset.
 *  @return On success, lseek returns the resulting descriptor offset in bytes
 *  from the beginning of the file.  On failure, it returns -1 and sets errno
 *  appropriately.
 */
#ifndef __GLIBC__
extern off_t lseek(int desc, off_t offset, int whence);
#endif
/**
 *  @posix
 *  Manipulates device parameters of special files.
 *  @param desc The file descriptor
 *  @param request The device-dependent request code.
 *  @param arg An untyped pointer to memory used to convey request information.
 *  @return Ioctl usually returns zero on success.  Some requests return
 *  positive values.  On failure, it returns -1 and sets errno appropriately.
 */
#ifndef __GLIBC__
extern int ioctl(int desc, int request, void *arg);
#endif
/**
 *  @nacl
 *  Sets the system break to the given address and return the address after
 *  the update.  If new_break is NULL, simply returns the current break address.
 *  @param new_break The address to set the break to.
 *  @return On success, sysbrk returns the value of the break address.  On
 *  failure, it returns -1 and sets errno appropriately.
 */
extern void *sysbrk(void *new_break);
/**
 *  @posix
 *  Maps a specified descriptor into the application address space.
 *  @param start The requested start address for the mapping
 *  @param length The number of bytes to be mapped (must be a multiple of 4K)
 *  @param prot The protection
 *  @param flags The modes the descriptor should be mapped under.  The only
 *  allowed flags are MAP_FIXED, MAP_SHARED, MAP_PRIVATE and MAP_ANONYMOUS.
 *  @param desc The descriptor to map
 *  @param offset The offset within the file specified by desc.
 *  @return On success, mmap returns the address the descriptor was mapped
 *  into. On failure it returns MAP_FAILED and sets errno appropriately.
 */
#ifndef __GLIBC__
extern void *mmap(void *start, size_t length, int prot, int flags,
                  int desc, off_t offset);
#endif
/**
 *  @posix
 *  Unmaps the region of memory starting at a given start address with a given
 *  length.
 *  @param start The start address of the region to be unmapped
 *  @param length The length of the region to be unmapped.
 *  @return On success, munmap returns zero.  On failure, it returns -1 and
 *  sets errno appropriately.
 */
#ifndef __GLIBC__
extern int munmap(void *start, size_t length);
#endif
/**
 *  @posix
 *  Terminates the program, returning a specified exit status.
 *  @param status The status to be returned.
 *  @return Exit does not return.
 */
#ifndef __GLIBC__
extern void _exit(int status);
#endif
/**
 *  @nacl
 *  Terminates the current thread.
 *  @return Thread_exit does not return.
 */
extern void thread_exit(void); /**< PENDING: describe */
/**
 *  @posix
 *  Gets the current time of day.
 *  @param tv A pointer to struct timeval, where the time is written.
 *  @param tz Unused.
 *  @return On success, gettimeofday returns zero.  On failure, it
 *  returns -1 and sets errno appropriately.
 */
#ifndef __GLIBC__
extern int gettimeofday(struct timeval *tv, void *tz);
#endif
/**
 *  @posix
 *  Returns approximate processor time used by a program.  (Deprecated.)
 *  @return clock returns a clock_t time value on success.  On failure, it
 *  returns -1 and sets errno appropriately.
 */
#ifndef __GLIBC__
extern clock_t clock(void);
#endif
/**
 *  @posix
 *  Suspends execution of the current thread by the specified time.
 *  @param req A pointer to a struct timespec containing the requested
 *  sleep time.
 *  @param rem A pointer to a struct timespec where, if non-NULL and the
 *  call to nanosleep were interrupted, the remaining sleep time is written
 *  (and nanosleep returns -1, with errno set to EINTR).
 *  @return On success, nanosleep returns 0.  On error, it returns -1 and
 *  sets errno appropriately.
 */
extern int nanosleep(const struct timespec *req, struct timespec *rem);

/**
 *  @nqPosix
 *  Returns information about a file.
 *  @param path The posix pathname of the file.
 *  @param buf The struct stat to store the information about the file.  Only
 *  size (st_size) is valid.
 *  @return On success, stat returns zero.  On error, it returns -1 and sets
 *  errno appropriately.
 */
#ifndef __GLIBC__
extern int stat(const char *path, struct stat *buf);
#endif

/**
 *  @nacl
 *  Creates a bound IMC socket and returns a descriptor for the socket and the
 *  socket address.
 *  @param sock_and_addr An array of two file descriptors.  On successful
 *  return, the first is set to the descriptor of the bound socket, and can be
 *  used for, e.g., imc_accept.  On successful return, the second descriptor is
 *  set to a socket address object that may be sent to other NativeClient
 *  modules or the browser plugin.
 *  @return On success, imcmakeboundsock returns zero.  On failure, it returns
 *  -1 and sets errno appropriately.
 */
extern int imc_makeboundsock(int sock_and_addr[2]);
/**
 *  @nacl
 *  Accepts a connection on a bound IMC socket, returning a connected IMC socket
 *  descriptor.
 *  @param desc The file descriptor of an IMC bound socket.
 *  @return On success, imc_accept returns a non-negative file descriptor for
 *  the connected socket.  On failure, it returns -1 and sets errno
 *  appropriately.
 */
extern int imc_accept(int desc);
/**
 *  @nacl
 *  Connects to an IMC socket address, returning a connected IMC socket
 *  descriptor.
 *  @param desc The file descriptor of an IMC socket address.
 *  @return On success, imc_connect returns a non-negative file descriptor for
 *  the connected socket.  On failure, it returns -1 and sets errno.
 *  The returned descriptor may be used to transfer data and descriptors
 *  but is itself not transferable.
 *  appropriately.
 */
extern int imc_connect(int desc);
/**
 *  @nacl
 *  Sends a message over a specified IMC socket descriptor.
 *  descriptor.
 *  @param desc The file descriptor of an IMC socket.
 *  @param nmhp The message header structure containing information to be sent.
 *  @param flags TBD
 *  @return On success, imc_sendmsg returns a non-negative number of bytes sent
 *  to the socket.  On failure, it returns -1 and sets errno appropriately.
 *  The returned descriptor may be used to transfer data and descriptors
 *  but is itself not transferable.
 */
extern int imc_sendmsg(int desc, struct NaClImcMsgHdr const *nmhp, int flags);
/**
 *  @nacl
 *  Receives a message over a specified IMC socket descriptor.
 *  @param desc The file descriptor of an IMC socket.
 *  @param nmhp The message header structure to be populated when receiving the
 *  message.
 *  @param flags TBD
 *  @return On success, imc_recvmsg returns a non-negative number of bytes
 *  read. On failure, it returns -1 and sets errno appropriately.
 */
extern int imc_recvmsg(int desc, struct NaClImcMsgHdr *nmhp, int flags);
/**
 *  @nacl
 *  Creates an IMC shared memory region, returning a file descriptor.
 *  @param nbytes The number of bytes in the requested shared memory region.
 *  @return On success, imc_mem_obj_create returns a non-negative file
 *  descriptor for the shared memory region.  On failure, it returns -1 and
 *  sets errno appropriately.
 */
extern int imc_mem_obj_create(size_t nbytes);
/**
 *  @nacl
 *  Creates an IMC socket pair, returning a pair of file descriptors.
 *  These descriptors are data-only, i.e., they may be used with
 *  imc_sendmsg and imc_recvmsg, but only if the descriptor count
 *  (desc_length) is zero.
 *  @param pair An array of two file descriptors for the two ends of the
 *  socket.
 *  @return On success imc_socketpair returns zero.  On failure, it returns -1
 *  and sets errno appropriately.
 *  The returned descriptor may only be used to transfer data
 *  and is itself transferable.
 */
extern int imc_socketpair(int pair[2]);

/**
 *  @nacl
 *  Relinquish the processor voluntarily.
 */
#ifndef __GLIBC__
extern int sched_yield(void);
#endif

/**
 *  @nacl
 *  Validates and dynamically loads executable code into an unused address.
 *  @param dest Destination address.  Must be in the code region and
 *  be instruction-bundle-aligned.
 *  @param src Source address.  Does not need to be aligned.
 *  @param size This must be a multiple of the bundle size.
 *  @return Returns zero on success, -1 on failure.
 *  Sets errno to EINVAL if validation fails, if src or size are not
 *  properly aligned, or the destination is invalid or has already had
 *  code loaded into it.
 */
extern int nacl_dyncode_create(void *dest, const void *src, size_t size);

/**
 *  @nacl
 *  Validates and modifies previously loaded dynamic code.  Must
 *  have identical instruction boundaries to existing code.
 *  @param dest Destination address. Must be subregion of one previously created
 *  @param src Source address.
 *  @param size of both dest and src, need not be aligned.
 *  @return Returns zero on success, -1 on failure.
 *  Sets errno to EINVAL if validation fails, if src or size are not
 *  properly aligned, or the destination is invalid or has already had
 *  code loaded into it.
 */
extern int nacl_dyncode_modify(void *dest, const void *src, size_t size);

/**
 *  @nacl
 *  Remove inserted dynamic code or mark it for deletion if threads are
 *  unaccounted for.
 *  @param dest must match a past call to nacl_dyncode_create
 *  @param size must match a past call to nacl_dyncode_create
 *  @return Returns zero on success, -1 on failure.
 *  Fails and sets errno to EAGAIN if deletion is delayed because other
 *  threads have not checked into the nacl runtime.
 */
extern int nacl_dyncode_delete(void *dest, size_t size);


#ifdef __cplusplus
}
#endif

/**
 * @}
 * End of System Calls group
 */

#endif
