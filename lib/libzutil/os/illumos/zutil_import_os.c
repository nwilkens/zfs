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
 * Copyright (c) 2012, 2017 by Delphix. All rights reserved.
 */

/*
 * Pool import support functions for illumos.
 *
 * Scans /dev/dsk/ for devices with ZFS labels to discover importable pools.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dkio.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/efi_partition.h>
#include <sys/vdev_impl.h>

#include <libzutil.h>

#include "zutil_import.h"

void
update_vdev_config_dev_strs(nvlist_t *nv)
{
	/*
	 * On illumos, devid and phys_path are handled natively.
	 * For now, strip them to match FreeBSD behavior.
	 */
	(void) nvlist_remove_all(nv, ZPOOL_CONFIG_DEVID);
	(void) nvlist_remove_all(nv, ZPOOL_CONFIG_PHYS_PATH);
}

void
zpool_open_func(void *arg)
{
	rdsk_node_t *rn = arg;
	struct stat64 statbuf;
	nvlist_t *config;
	int num_labels;
	int fd;

	if ((fd = open(rn->rn_name, O_RDONLY | O_NONBLOCK | O_LARGEFILE)) < 0)
		return;

	if (fstat64(fd, &statbuf) != 0)
		goto out;

	if (S_ISREG(statbuf.st_mode)) {
		if (statbuf.st_size < SPA_MINDEVSIZE)
			goto out;
	} else if (S_ISBLK(statbuf.st_mode) || S_ISCHR(statbuf.st_mode)) {
		struct dk_minfo dkm;
		if (ioctl(fd, DKIOCGMEDIAINFO, &dkm) != 0)
			goto out;
		if ((uint64_t)dkm.dki_capacity * dkm.dki_lbsize <
		    SPA_MINDEVSIZE)
			goto out;
	} else {
		goto out;
	}

	if (zpool_read_label(fd, &config, &num_labels) != 0)
		goto out;
	if (num_labels == 0) {
		nvlist_free(config);
		goto out;
	}

	rn->rn_config = config;
	rn->rn_num_labels = num_labels;

out:
	(void) close(fd);
}

static const char * const
zpool_default_import_path[] = {
	"/dev/dsk"
};

const char * const *
zpool_default_search_paths(size_t *count)
{
	*count = 1;
	return (zpool_default_import_path);
}

int
zpool_find_import_blkid(libpc_handle_t *hdl, pthread_mutex_t *lock,
    avl_tree_t **slice_cache)
{
	DIR *dirp;
	struct dirent *dp;
	char path[MAXPATHLEN];
	rdsk_node_t *slice;
	avl_index_t where;

	*slice_cache = zutil_alloc(hdl, sizeof (avl_tree_t));
	avl_create(*slice_cache, slice_cache_compare, sizeof (rdsk_node_t),
	    offsetof(rdsk_node_t, rn_node));

	dirp = opendir("/dev/dsk");
	if (dirp == NULL)
		return (0);

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;

		(void) snprintf(path, sizeof (path), "/dev/dsk/%s",
		    dp->d_name);

		slice = zutil_alloc(hdl, sizeof (rdsk_node_t));
		slice->rn_name = zutil_strdup(hdl, path);
		slice->rn_vdev_guid = 0;
		slice->rn_lock = lock;
		slice->rn_avl = *slice_cache;
		slice->rn_hdl = hdl;
		slice->rn_labelpaths = B_FALSE;
		slice->rn_order = IMPORT_ORDER_DEFAULT;

		pthread_mutex_lock(lock);
		if (avl_find(*slice_cache, slice, &where)) {
			free(slice->rn_name);
			free(slice);
		} else {
			avl_insert(*slice_cache, slice, where);
		}
		pthread_mutex_unlock(lock);
	}

	(void) closedir(dirp);
	return (0);
}

int
zfs_dev_flush(int fd)
{
	(void) fd;
	return (0);
}

void
update_vdev_config_dev_sysfs_path(nvlist_t *nv, const char *path,
    const char *key)
{
	(void) nv;
	(void) path;
	(void) key;
}

void
update_vdevs_config_dev_sysfs_path(nvlist_t *config)
{
	(void) config;
}

int
zpool_disk_wait(const char *path)
{
	(void) path;
	return (ENOTSUP);
}
