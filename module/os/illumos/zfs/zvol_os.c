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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2017 by Delphix. All rights reserved.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ZFS volume (zvol) emulation driver for illumos.
 * Stubs for now -- the full zvol block device implementation
 * will be ported from the illumos-joyent zvol.c.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/zfs_context.h>
#include <sys/dmu.h>
#include <sys/spa.h>
#include <sys/zap.h>
#include <sys/dmu_objset.h>
#include <sys/zfs_rlock.h>
#include <sys/dataset_kstats.h>
#include <sys/zvol.h>
#include <sys/zvol_impl.h>
#include <sys/sunddi.h>

void
zvol_os_free(zvol_state_t *zv)
{
	(void) zv;
	/* TODO: free illumos-specific zvol state */
}

int
zvol_os_rename_minor(zvol_state_t *zv, const char *newname)
{
	(void) zv;
	(void) newname;
	return (SET_ERROR(ENOTSUP));
}

int
zvol_os_create_minor(const char *name)
{
	(void) name;
	return (SET_ERROR(ENOTSUP));
}

int
zvol_os_update_volsize(zvol_state_t *zv, uint64_t volsize)
{
	(void) zv;
	(void) volsize;
	return (SET_ERROR(ENOTSUP));
}

boolean_t
zvol_os_is_zvol(const char *path)
{
	(void) path;
	return (B_FALSE);
}

void
zvol_os_remove_minor(zvol_state_t *zv)
{
	(void) zv;
}

void
zvol_os_set_disk_ro(zvol_state_t *zv, int flags)
{
	(void) zv;
	(void) flags;
}

void
zvol_os_set_capacity(zvol_state_t *zv, uint64_t capacity)
{
	(void) zv;
	(void) capacity;
}
