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
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2013, 2018 by Delphix. All rights reserved.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <libnvpair.h>
#include <libzutil.h>
#include <limits.h>
#include <sys/spa.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mntent.h>

#include "zpool_util.h"
#include <sys/zfs_context.h>

/*
 * Validate a device. On illumos, just delegate to the generic file check.
 */
int
check_device(const char *name, boolean_t force, boolean_t isspare,
    boolean_t iswholedisk)
{
	(void) iswholedisk;
	return (check_file(name, force, isspare));
}

boolean_t
check_sector_size_database(char *path, int *sector_size)
{
	(void) path;
	(void) sector_size;
	return (B_FALSE);
}

void
after_zpool_upgrade(zpool_handle_t *zhp)
{
	(void) zhp;
}

int
check_file(const char *file, boolean_t force, boolean_t isspare)
{
	return (check_file_generic(file, force, isspare));
}

int
zpool_power_current_state(zpool_handle_t *zhp, char *vdev)
{
	(void) zhp;
	(void) vdev;
	return (-1);
}

int
zpool_power(zpool_handle_t *zhp, char *vdev, boolean_t turn_on)
{
	(void) zhp;
	(void) vdev;
	(void) turn_on;
	return (ENOTSUP);
}
