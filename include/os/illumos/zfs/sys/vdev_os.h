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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2020 Joyent, Inc.
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef _SYS_VDEV_OS_H
#define	_SYS_VDEV_OS_H

#include <sys/sunldi.h>
#include <sys/ddi.h>
#include <sys/list.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * illumos per-vdev OS state.
 *
 * On illumos, disk vdevs are opened through the Layer Driver Interface (LDI).
 * vdev_os_t is stored in vdev_t.vdev_tsd and holds the LDI handle plus
 * auxiliary state for hotplug event callbacks.
 */
typedef struct vdev_os {
	ddi_devid_t	vd_devid;	/* device ID for matching */
	char		*vd_minor;	/* minor name string */
	ldi_handle_t	vd_lh;		/* LDI handle for the open device */
	list_t		vd_ldi_cbs;	/* list of LDI event callbacks */
	boolean_t	vd_ldi_offline;	/* offline event received */
} vdev_os_t;

/*
 * Read the pool label from all disks matching the given pool name.
 * Used during pool import to discover member vdevs.
 */
extern int vdev_disk_read_rootlabel(const char *devpath, const char *devid,
    nvlist_t **config);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_VDEV_OS_H */
