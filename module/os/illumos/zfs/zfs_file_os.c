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
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * Kernel file operations for illumos.
 * Uses vn_open/vn_rdwr/VOP interfaces.
 */

#include <sys/zfs_context.h>
#include <sys/zfs_file.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/sunddi.h>
#include <sys/dmu.h>

int
zfs_file_open(const char *path, int flags, int mode, zfs_file_t **fpp)
{
	vnode_t *vp;
	int error;

	error = vn_open(path, UIO_SYSSPACE, flags, mode, &vp, CRCREAT, 0);
	if (error != 0)
		return (SET_ERROR(error));

	VN_RELE(vp);

	/*
	 * vn_open gives us a vnode; we need to wrap it in a file_t.
	 * Use falloc() / setf() pattern or just use vn_openat + getf.
	 * For kernel-internal file I/O, we use the file_t obtained
	 * from vn_open directly.
	 */
	error = vn_open(path, UIO_SYSSPACE, flags, mode, &vp, CRCREAT, 0);
	if (error != 0)
		return (SET_ERROR(error));

	*fpp = (zfs_file_t *)vp;
	return (0);
}

void
zfs_file_close(zfs_file_t *fp)
{
	vnode_t *vp = (vnode_t *)fp;

	(void) VOP_CLOSE(vp, FREAD | FWRITE, 1, 0, kcred, NULL);
	VN_RELE(vp);
}

int
zfs_file_write(zfs_file_t *fp, const void *buf, size_t count, ssize_t *resid)
{
	vnode_t *vp = (vnode_t *)fp;
	ssize_t written;
	int error;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)buf, count, 0,
	    UIO_SYSSPACE, FAPPEND, RLIM64_INFINITY, kcred, &written);
	if (error != 0)
		return (SET_ERROR(error));

	if (resid != NULL)
		*resid = count - written;
	else if (written != count)
		return (SET_ERROR(EIO));

	return (0);
}

int
zfs_file_pwrite(zfs_file_t *fp, const void *buf, size_t count, loff_t off,
    uint8_t ashift, ssize_t *resid)
{
	vnode_t *vp = (vnode_t *)fp;
	ssize_t r;
	int error;

	(void) ashift;

	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)buf, count, off,
	    UIO_SYSSPACE, 0, RLIM64_INFINITY, kcred, &r);
	if (error != 0)
		return (SET_ERROR(error));

	if (resid != NULL)
		*resid = r;
	else if (r != 0)
		return (SET_ERROR(EIO));

	return (0);
}

int
zfs_file_read(zfs_file_t *fp, void *buf, size_t count, ssize_t *resid)
{
	vnode_t *vp = (vnode_t *)fp;
	ssize_t r;
	int error;

	error = vn_rdwr(UIO_READ, vp, buf, count, 0,
	    UIO_SYSSPACE, 0, RLIM64_INFINITY, kcred, &r);
	if (error != 0)
		return (SET_ERROR(error));

	if (resid != NULL)
		*resid = r;

	return (0);
}

int
zfs_file_pread(zfs_file_t *fp, void *buf, size_t count, loff_t off,
    ssize_t *resid)
{
	vnode_t *vp = (vnode_t *)fp;
	ssize_t r;
	int error;

	error = vn_rdwr(UIO_READ, vp, buf, count, off,
	    UIO_SYSSPACE, 0, RLIM64_INFINITY, kcred, &r);
	if (error != 0)
		return (SET_ERROR(error));

	if (resid != NULL)
		*resid = r;

	return (0);
}

int
zfs_file_seek(zfs_file_t *fp, loff_t *offp, int whence)
{
	vnode_t *vp = (vnode_t *)fp;
	struct vattr va;
	int error;

	if (whence == SEEK_END) {
		va.va_mask = AT_SIZE;
		error = VOP_GETATTR(vp, &va, 0, kcred, NULL);
		if (error != 0)
			return (SET_ERROR(error));
		*offp = va.va_size + *offp;
	} else if (whence == SEEK_CUR) {
		/* Caller must track position themselves */
	}
	/* SEEK_SET: *offp is already the absolute position */

	return (0);
}

int
zfs_file_getattr(zfs_file_t *fp, zfs_file_attr_t *zfattr)
{
	vnode_t *vp = (vnode_t *)fp;
	struct vattr va;
	int error;

	va.va_mask = AT_SIZE | AT_MODE;
	error = VOP_GETATTR(vp, &va, 0, kcred, NULL);
	if (error != 0)
		return (SET_ERROR(error));

	zfattr->zfa_size = va.va_size;
	zfattr->zfa_mode = va.va_mode;

	return (0);
}

int
zfs_file_fsync(zfs_file_t *fp, int flags)
{
	vnode_t *vp = (vnode_t *)fp;
	int error;

	(void) flags;

	error = VOP_FSYNC(vp, FSYNC, kcred, NULL);

	return (SET_ERROR(error));
}

int
zfs_file_deallocate(zfs_file_t *fp, loff_t offset, loff_t len)
{
	(void) fp;
	(void) offset;
	(void) len;

	/* illumos does not have a generic fallocate/fspacectl interface */
	return (SET_ERROR(ENOTSUP));
}

zfs_file_t *
zfs_file_get(int fd)
{
	file_t *fp;

	fp = getf(fd);
	if (fp == NULL)
		return (NULL);

	return ((zfs_file_t *)fp->f_vnode);
}

void
zfs_file_put(zfs_file_t *fp)
{
	(void) fp;
	/* The caller is responsible for releasef() */
}

loff_t
zfs_file_off(zfs_file_t *fp)
{
	(void) fp;
	/* On illumos, vnode-based file I/O doesn't track an offset */
	return (0);
}

void *
zfs_file_private(zfs_file_t *fp)
{
	(void) fp;
	return (NULL);
}

int
zfs_file_unlink(const char *fnamep)
{
	return (vn_remove(fnamep, UIO_SYSSPACE, RMFILE));
}
