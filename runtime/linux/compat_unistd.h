/* -*- linux-c -*- 
 * Syscall compatibility defines.
 * Copyright (C) 2013 Red Hat Inc.
 *
 * This file is part of systemtap, and is free software.  You can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License (GPL); either version 2, or (at your option) any
 * later version.
 */

#ifndef _COMPAT_UNISTD_H_
#define _COMPAT_UNISTD_H_

#if defined(__x86_64__)

// On older kernels (like RHEL5), we have to define our own 32-bit
// syscall numbers.
#ifndef __NR_ia32_close
#define __NR_ia32_close 6
#endif
#ifndef __NR_ia32_dup3
#define __NR_ia32_dup3 330
#endif
#ifndef __NR_ia32_faccessat
#define __NR_ia32_faccessat 307
#endif
#ifndef __NR_ia32_fchmodat
#define __NR_ia32_fchmodat 306
#endif
#ifndef __NR_ia32_fchownat
#define __NR_ia32_fchownat 298
#endif
#ifndef __NR_ia32_getpgid
#define __NR_ia32_getpgid 132
#endif
#ifndef __NR_ia32_inotify_init1
#define __NR_ia32_inotify_init1 332
#endif
#ifndef __NR_ia32_linkat
#define __NR_ia32_linkat 303
#endif
#ifndef __NR_ia32_pipe2
#define __NR_ia32_pipe2 331
#endif
#ifndef __NR_ia32_readlinkat
#define __NR_ia32_readlinkat 305
#endif
#ifndef __NR_ia32_rt_sigprocmask
#define __NR_ia32_rt_sigprocmask 175
#endif
#ifndef __NR_ia32_symlinkat
#define __NR_ia32_symlinkat 304
#endif
#ifndef __NR_ia32_umount2
#define __NR_ia32_umount2 52
#endif

#define __NR_compat_close		__NR_ia32_close
#define __NR_compat_dup3		__NR_ia32_dup3
#define __NR_compat_faccessat		__NR_ia32_faccessat
#define __NR_compat_fchmodat		__NR_ia32_fchmodat
#define __NR_compat_fchownat		__NR_ia32_fchownat
#define __NR_compat_getpgid		__NR_ia32_getpgid
#define __NR_compat_inotify_init1	__NR_ia32_inotify_init1
#define __NR_compat_linkat		__NR_ia32_linkat
#define __NR_compat_pipe2		__NR_ia32_pipe2
#define __NR_compat_readlinkat		__NR_ia32_readlinkat
#define __NR_compat_rt_sigprocmask	__NR_ia32_rt_sigprocmask
#define __NR_compat_symlinkat		__NR_ia32_symlinkat
#define __NR_compat_umount2		__NR_ia32_umount2

#endif	/* __x86_64__ */

#if defined(__powerpc64__) || defined (__s390x__)

// On the ppc64 and s390x, the 32-bit syscalls use the same number
// as the 64-bit syscalls.

#define __NR_compat_close		__NR_close
#define __NR_compat_dup3		__NR_dup3
#define __NR_compat_faccessat		__NR_faccessat
#define __NR_compat_fchmodat		__NR_fchmodat
#define __NR_compat_fchownat		__NR_fchownat
#define __NR_compat_getpgid		__NR_getpgid
#define __NR_compat_inotify_init1	__NR_inotify_init1
#define __NR_compat_linkat		__NR_linkat
#define __NR_compat_pipe2		__NR_pipe2
#define __NR_compat_readlinkat		__NR_readlinkat
#define __NR_compat_rt_sigprocmask	__NR_rt_sigprocmask
#define __NR_compat_symlinkat		__NR_symlinkat
#define __NR_compat_umount2		__NR_umount2

#endif	/* __powerpc64__ || __s390x__ */

#if defined(__ia64__)

// On RHEL5 ia64, __NR_umount2 doesn't exist. So, define it in terms
// of __NR_umount.

#ifndef __NR_umount2
#define __NR_umount2 __NR_umount
#endif

#endif	/* __ia64__ */

#endif /* _COMPAT_UNISTD_H_ */
