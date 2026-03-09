// SPDX-License-Identifier: CDDL-1.0
/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef _ZFS_BOOTENV_OS_H
#define	_ZFS_BOOTENV_OS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The OS-specific boot environment vendor string used when reading and
 * writing boot environment nvlists stored in the pool.  On illumos this
 * is "illumos" (BE_ILLUMOS_VENDOR from sys/zfs_bootenv.h).
 */
#define	BOOTENV_OS		BE_ILLUMOS_VENDOR

#ifdef __cplusplus
}
#endif

#endif /* _ZFS_BOOTENV_OS_H */
