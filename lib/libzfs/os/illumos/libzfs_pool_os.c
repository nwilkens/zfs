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
 * Copyright 2025 Oxide Computer Company
 */

/*
 * illumos-specific pool operations for libzfs.
 *
 * On illumos, disk labeling is typically handled by the kernel or by
 * format(1M).  For now these are stubs that return success, similar
 * to the FreeBSD implementation.
 */

#include <errno.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libzfs.h>
#include <libzutil.h>

#include "../../libzfs_impl.h"

/*
 * If the device has been dynamically expanded then we need to relabel
 * the disk to use the new unallocated space.
 *
 * On illumos, this is a no-op for now.  The native ZFS handles this
 * internally via the kernel.
 */
int
zpool_relabel_disk(libzfs_handle_t *hdl, const char *path, const char *msg)
{
	(void) hdl, (void) path, (void) msg;
	return (0);
}

/*
 * Label an individual disk.  The name provided is the short name,
 * stripped of any leading /dev path.
 *
 * On illumos, disk labeling is handled differently than on Linux.
 * For now this is a no-op stub, similar to FreeBSD.
 */
int
zpool_label_disk(libzfs_handle_t *hdl, zpool_handle_t *zhp, const char *name)
{
	(void) hdl, (void) zhp, (void) name;
	return (0);
}
