// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * Copyright 2019 Joyent, Inc.
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef ZFS_CONTEXT_OS_H_
#define	ZFS_CONTEXT_OS_H_

/*
 * illumos-specific additions to zfs_context.h.
 *
 * The bulk of what illumos needs (sys/note.h, sys/types.h, sys/t_lock.h,
 * sys/atomic.h, sys/sysmacros.h, sys/kmem.h, sys/taskq.h, sys/list.h,
 * sys/uio.h, sys/time.h, sys/debug.h, sys/byteorder.h, sys/cmn_err.h, etc.)
 * is already pulled in by the platform-neutral zfs_context.h or by the
 * illumos kernel headers themselves.  We add only the headers that are
 * specific to the illumos OS layer and not provided elsewhere.
 */

#include <sys/note.h>
#include <sys/taskq_impl.h>
#include <sys/buf.h>
#include <sys/cpuvar.h>
#include <sys/kobj.h>
#include <sys/conf.h>
#include <sys/cyclic.h>
#include <sys/callo.h>
#include <sys/fm/util.h>
#include <sys/fcntl.h>
#include <sys/ccompat.h>
#include <sys/bitmap.h>
#include <sys/mount.h>
#include <sys/utsname.h>

/*
 * OpenZFS calls utsname()->nodename; illumos has the global struct utsname.
 * A function-like macro only matches utsname() — plain utsname still names
 * the extern variable declared in <sys/utsname.h>.
 */
#define	utsname()	(&utsname)

/*
 * MNT_FORCE — Linux/FreeBSD name for forced unmount flag.
 * illumos uses MS_FORCE (0x0400) from <sys/mount.h>.
 */
#ifndef MS_FORCE
#define	MS_FORCE	0x0400
#endif
#ifndef MNT_FORCE
#define	MNT_FORCE	MS_FORCE
#endif

/*
 * SEEK_SET / SEEK_CUR / SEEK_END — lseek whence values.
 * Not always available in illumos kernel headers; define if missing.
 */
#ifndef SEEK_SET
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

/*
 * On illumos, CPU_SEQID is obtained from the current CPU structure.
 */
#define	CPU_SEQID	(CPU->cpu_seqid)

/*
 * Compiler branch-prediction hints.  GCC / clang provide __builtin_expect;
 * other compilers fall back to the plain expression.
 */
#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#define	_zfs_expect(expr, value)	(__builtin_expect((expr), (value)))
#else
#define	_zfs_expect(expr, value)	(expr)
#endif

#define	likely(x)	_zfs_expect((x) != 0, 1)
#define	unlikely(x)	_zfs_expect((x) != 0, 0)

/*
 * AVL tree comparison helpers added by OpenZFS that are not yet in the
 * illumos sys/avl.h header.
 */
#define	TREE_ISIGN(a)	(((a) > 0) - ((a) < 0))
#define	TREE_CMP(a, b)	(((a) > (b)) - ((a) < (b)))
#define	TREE_PCMP(a, b)	\
	(((uintptr_t)(a) > (uintptr_t)(b)) - ((uintptr_t)(a) < (uintptr_t)(b)))

#endif	/* ZFS_CONTEXT_OS_H_ */
