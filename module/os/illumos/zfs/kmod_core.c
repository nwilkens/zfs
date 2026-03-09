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
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * Kernel module init/fini for ZFS on illumos.
 * Uses the standard illumos _init/_fini/_info + modlinkage pattern,
 * integrated with the OpenZFS zfs_kmod_init/zfs_kmod_fini framework.
 */

#include <sys/zfs_context.h>
#include <sys/spa.h>
#include <sys/spa_impl.h>
#include <sys/dmu.h>
#include <sys/zap.h>
#include <sys/vdev_impl.h>
#include <sys/metaslab.h>
#include <sys/dmu_objset.h>
#include <sys/dsl_dir.h>
#include <sys/dsl_dataset.h>
#include <sys/dsl_prop.h>
#include <sys/dsl_pool.h>
#include <sys/dsl_scan.h>
#include <sys/fs/zfs.h>
#include <sys/arc.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_ioctl_impl.h>
#include <sys/zfs_ctldir.h>
#include <sys/zvol.h>
#include <sys/zfeature.h>
#include <sys/abd.h>
#include <sys/sunddi.h>
#include <sys/modctl.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/zone.h>

/*
 * Forward declarations for driver entry points.
 */
static int zfs_attach(dev_info_t *, ddi_attach_cmd_t);
static int zfs_detach(dev_info_t *, ddi_detach_cmd_t);
static int zfs_info(dev_info_t *, ddi_info_cmd_t, void *, void **);

/*
 * Forward declarations for /dev/zfs device operations.
 * These will be implemented in zfs_ioctl_os.c.
 */
extern int zfsdev_open(dev_t *devp, int flag, int otyp, cred_t *cr);
extern int zfsdev_close(dev_t dev, int flag, int otyp, cred_t *cr);
extern int zfsdev_ioctl(dev_t dev, int cmd, intptr_t arg, int flag,
    cred_t *cr, int *rvalp);

static dev_info_t *zfs_dip;

extern uint_t rrw_tsd_key;

/*
 * /dev/zfs is the control node, i.e. minor 0.
 * /dev/zvol/[r]dsk/pool/dataset are the zvols, minor > 0.
 */
static struct cb_ops zfs_cb_ops = {
	zfsdev_open,		/* open */
	zfsdev_close,		/* close */
	nodev,			/* strategy */
	nodev,			/* print */
	nodev,			/* dump */
	nodev,			/* read */
	nodev,			/* write */
	zfsdev_ioctl,		/* ioctl */
	nodev,			/* devmap */
	nodev,			/* mmap */
	nodev,			/* segmap */
	nochpoll,		/* poll */
	ddi_prop_op,		/* prop_op */
	NULL,			/* streamtab */
	D_NEW | D_MP | D_64BIT,	/* driver compatibility flag */
	CB_REV,			/* version */
	nodev,			/* async read */
	nodev,			/* async write */
};

static struct dev_ops zfs_dev_ops = {
	DEVO_REV,		/* version */
	0,			/* refcnt */
	zfs_info,		/* info */
	nulldev,		/* identify */
	nulldev,		/* probe */
	zfs_attach,		/* attach */
	zfs_detach,		/* detach */
	nodev,			/* reset */
	&zfs_cb_ops,		/* driver operations */
	NULL,			/* no bus operations */
	NULL,			/* power */
	ddi_quiesce_not_needed,	/* quiesce */
};

static struct modldrv zfs_modldrv = {
	&mod_driverops,
	"ZFS storage pool",
	&zfs_dev_ops
};

static struct modlinkage modlinkage = {
	MODREV_1,
	(void *)&zfs_modldrv,
	NULL
};

static int
zfs_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	if (cmd != DDI_ATTACH)
		return (DDI_FAILURE);

	if (ddi_create_minor_node(dip, "zfs", S_IFCHR, 0,
	    DDI_PSEUDO, 0) == DDI_FAILURE)
		return (DDI_FAILURE);

	zfs_dip = dip;
	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}

static int
zfs_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	if (cmd != DDI_DETACH)
		return (DDI_FAILURE);

	zfs_dip = NULL;
	ddi_prop_remove_all(dip);
	ddi_remove_minor_node(dip, NULL);

	return (DDI_SUCCESS);
}

/*ARGSUSED*/
static int
zfs_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		*result = zfs_dip;
		return (DDI_SUCCESS);

	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)0;
		return (DDI_SUCCESS);
	}

	return (DDI_FAILURE);
}

int
_init(void)
{
	int error;

	if ((error = zfs_kmod_init()) != 0) {
		cmn_err(CE_WARN, "ZFS: Failed to initialize ZFS, "
		    "error %d", error);
		return (error);
	}

	if ((error = mod_install(&modlinkage)) != 0) {
		zfs_kmod_fini();
		return (error);
	}

	return (0);
}

int
_fini(void)
{
	int error;

	if ((error = mod_remove(&modlinkage)) != 0)
		return (error);

	zfs_kmod_fini();

	return (0);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}
