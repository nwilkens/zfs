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

#ifndef _SPL_SYS_MISC_H
#define	_SPL_SYS_MISC_H

#include <sys/types.h>
#include <sys/param.h>

/*
 * Miscellaneous macros used by OpenZFS.
 *
 * Most of these are already provided by illumos system headers, but we
 * include them here as a convenience/compatibility shim.
 */

/*
 * MAXUID - maximum user ID value.
 */
#ifndef MAXUID
#define	MAXUID	UID_MAX
#endif

/*
 * ACL type flags.
 */
#define	_ACL_ACLENT_ENABLED	0x1
#define	_ACL_ACE_ENABLED	0x2

/*
 * F_SEEK_DATA / F_SEEK_HOLE - illumos defines these in <sys/fcntl.h> or
 * <sys/file.h>.  Provide them here if not already defined.
 */
#ifndef F_SEEK_DATA
#define	F_SEEK_DATA	23
#endif
#ifndef F_SEEK_HOLE
#define	F_SEEK_HOLE	24
#endif

/*
 * task_io_account_* - Linux-specific I/O accounting, no-op on illumos.
 */
#define	task_io_account_read(n)		do {} while (0)
#define	task_io_account_write(n)	do {} while (0)

#endif	/* _SPL_SYS_MISC_H */
