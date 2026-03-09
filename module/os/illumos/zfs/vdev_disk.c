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
 * Copyright (c) 2012, 2018 by Delphix. All rights reserved.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * Physical disk vdev for illumos.
 * Stubs for now -- the full LDI-based disk vdev implementation
 * will be ported from the illumos-joyent vdev_disk.c.
 */

#include <sys/zfs_context.h>
#include <sys/spa.h>
#include <sys/spa_impl.h>
#include <sys/vdev_impl.h>
#include <sys/vdev_os.h>
#include <sys/fs/zfs.h>
#include <sys/zio.h>
#include <sys/sunddi.h>
#include <sys/abd.h>

/*
 * Virtual device operations for physical disks on illumos.
 * Uses LDI (Layered Device Interface) for device access.
 */

static int
vdev_disk_open(vdev_t *vd, uint64_t *psize, uint64_t *max_psize,
    uint64_t *logical_ashift, uint64_t *physical_ashift)
{
	(void) vd;
	(void) psize;
	(void) max_psize;
	(void) logical_ashift;
	(void) physical_ashift;
	return (SET_ERROR(ENOTSUP));
}

static void
vdev_disk_close(vdev_t *vd)
{
	(void) vd;
}

static void
vdev_disk_io_start(zio_t *zio)
{
	zio->io_error = SET_ERROR(ENOTSUP);
	zio_interrupt(zio);
}

static void
vdev_disk_io_done(zio_t *zio)
{
	(void) zio;
}

static void
vdev_disk_io_fini(zio_t *zio)
{
	(void) zio;
}

vdev_ops_t vdev_disk_ops = {
	.vdev_op_init = NULL,
	.vdev_op_fini = NULL,
	.vdev_op_open = vdev_disk_open,
	.vdev_op_close = vdev_disk_close,
	.vdev_op_psize_to_asize = vdev_default_asize,
	.vdev_op_asize_to_psize = NULL,
	.vdev_op_min_asize = vdev_default_min_asize,
	.vdev_op_min_alloc = NULL,
	.vdev_op_io_start = vdev_disk_io_start,
	.vdev_op_io_done = vdev_disk_io_done,
	.vdev_op_state_change = NULL,
	.vdev_op_need_resilver = NULL,
	.vdev_op_hold = NULL,
	.vdev_op_rele = NULL,
	.vdev_op_remap = NULL,
	.vdev_op_xlate = vdev_default_xlate,
	.vdev_op_rebuild_asize = NULL,
	.vdev_op_metaslab_init = NULL,
	.vdev_op_config_generate = NULL,
	.vdev_op_nparity = NULL,
	.vdev_op_ndisks = NULL,
	.vdev_op_type = VDEV_TYPE_DISK,
	.vdev_op_leaf = B_TRUE,
};
