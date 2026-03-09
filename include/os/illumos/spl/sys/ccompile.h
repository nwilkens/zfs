// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

#ifndef _SPL_SYS_CCOMPILE_H
#define	_SPL_SYS_CCOMPILE_H

/*
 * illumos already provides <sys/ccompile.h> with compiler compatibility
 * macros.  Pull it in first, then add anything extra needed by OpenZFS.
 */
#include_next <sys/ccompile.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * ZFS debug flag handling.
 */
#ifdef ZFS_DEBUG
#undef NDEBUG
#endif
#if !defined(ZFS_DEBUG) && !defined(NDEBUG)
#define	NDEBUG
#endif

/*
 * EXPORT_SYMBOL - no-op on illumos (not a Linux kernel module).
 */
#ifndef EXPORT_SYMBOL
#define	EXPORT_SYMBOL(x)
#endif

/*
 * __init / __exit - Linux kernel section attributes, no-op on illumos.
 */
#ifndef __cplusplus
#ifndef __init
#define	__init
#endif
#ifndef __exit
#define	__exit
#endif
#endif

/*
 * Error code compatibility.
 *
 * illumos defines most standard POSIX errnos natively.  The following
 * are ZFS-specific or have different names on different platforms.
 */

/* ECKSUM - checksum error.  illumos defines this natively. */
#ifndef ECKSUM
#define	ECKSUM		EBADE
#endif

/* EFRAGS - fragmentation error, map to ENOSPC. */
#ifndef EFRAGS
#define	EFRAGS		ENOSPC
#endif

/* ENOTACTIVE - cancelled operation. */
#ifndef ENOTACTIVE
#define	ENOTACTIVE	ECANCELED
#endif

/* EREMOTEIO - remote I/O error. */
#ifndef EREMOTEIO
#define	EREMOTEIO	EREMOTE
#endif

/* ECHRNG - channel number out of range. */
#ifndef ECHRNG
#define	ECHRNG		ENXIO
#endif

/*
 * param_set_charp - no-op on illumos (Linux module param helper).
 */
#if defined(_KERNEL) || defined(_STANDALONE)
#define	param_set_charp(a, b)	(0)
#endif

/*
 * ATTR_* — OpenZFS vattr mask names (Linux convention).
 * illumos uses AT_* names.  Map them here (following FreeBSD).
 */
#ifndef ATTR_UID
#define	ATTR_UID	AT_UID
#define	ATTR_GID	AT_GID
#define	ATTR_MODE	AT_MODE
#define	ATTR_CTIME	AT_CTIME
#define	ATTR_MTIME	AT_MTIME
#define	ATTR_ATIME	AT_ATIME
#define	ATTR_XVATTR	AT_XVATTR
#define	ATTR_SIZE	AT_SIZE
#endif

/*
 * PAGE_SIZE — Linux name for page size.
 * illumos uses PAGESIZE (no underscore).
 */
#ifndef PAGE_SIZE
#define	PAGE_SIZE	PAGESIZE
#endif

/*
 * utsname_t — OpenZFS typedef for struct utsname.
 * illumos doesn't typedef this.
 */
typedef struct utsname utsname_t;

/*
 * zfs_fallthrough — C17 [[fallthrough]] or compiler-specific equivalent.
 * Used in switch statements to suppress implicit fallthrough warnings.
 */
#ifndef zfs_fallthrough
#if defined(__has_attribute) && __has_attribute(__fallthrough__)
#define	zfs_fallthrough		__attribute__((__fallthrough__))
#else
#define	zfs_fallthrough		((void)0)
#endif
#endif

/*
 * noinline — prevent compiler inlining.
 */
#ifndef noinline
#define	noinline		__attribute__((noinline))
#endif

/*
 * fstrans_cookie_t — Linux file system transaction cookie.
 * No equivalent on illumos; stub it out.
 */
#ifndef _FSTRANS_COOKIE_T
#define	_FSTRANS_COOKIE_T
typedef int fstrans_cookie_t;
#define	spl_fstrans_mark()	(0)
#define	spl_fstrans_unmark(x)	((void)(x))
#define	spl_fstrans_check()	(0)
#endif

/*
 * defclsyspri — default class system priority for taskq threads.
 * Not defined on illumos; set to minclsyspri.
 */
#ifndef defclsyspri
#define	defclsyspri		minclsyspri
#endif

/*
 * wtqclsyspri — write taskq class system priority.
 * Not defined on illumos; use maxclsyspri.
 */
#ifndef wtqclsyspri
#define	wtqclsyspri		maxclsyspri
#endif

/*
 * CPU_SEQID_UNSTABLE — relaxed CPU sequence ID (no preempt disable).
 * Not defined on illumos; map to CPU_SEQID.
 */
#ifndef CPU_SEQID_UNSTABLE
#define	CPU_SEQID_UNSTABLE	CPU_SEQID
#endif

/*
 * O_RDONLY, O_WRONLY, O_RDWR, O_TRUNC, O_SYNC — file open flags.
 * On illumos these are defined in <sys/fcntl.h> with POSIX values.
 * The kernel gets them via various system headers; OpenZFS neutral
 * code just uses the standard names.  No redefinition needed.
 */

/*
 * issig — OpenZFS calls issig() with 0 args (Linux style).
 * illumos issig() takes 1 arg (the reason: FORREAL or JUSTLOOKING).
 * This is handled in the proc.h SPL header instead of here to
 * avoid circular dependency issues.
 */

/*
 * KSTAT_FLAG_NO_HEADERS — flag for kstat with no header line.
 * Not present in all illumos versions; define if missing.
 */
#ifndef KSTAT_FLAG_NO_HEADERS
#define	KSTAT_FLAG_NO_HEADERS	0x10
#endif

/*
 * MUTEX_NOLOCKDEP / RW_NOLOCKDEP - lockdep is Linux-specific, no-op here.
 */
#ifndef MUTEX_NOLOCKDEP
#define	MUTEX_NOLOCKDEP	0
#endif
#ifndef RW_NOLOCKDEP
#define	RW_NOLOCKDEP	0
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SPL_SYS_CCOMPILE_H */
