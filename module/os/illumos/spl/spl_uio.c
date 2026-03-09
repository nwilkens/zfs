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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

/*
 * SPL uio wrappers for illumos.
 *
 * illumos has native uiomove().  OpenZFS wraps it with zfs_uio_t to
 * support direct I/O and other extensions.  These wrappers bridge
 * the OpenZFS zfs_uio_t interface to the native illumos uio_t.
 */

#include <sys/zfs_context.h>
#include <sys/uio_impl.h>

/*
 * Maximum number of iovec entries to clone on the stack.
 * Anything larger will be kmem_alloc'd.
 */
#define	IOV_MAX_STACK	8

int
zfs_uiomove(void *cp, size_t n, zfs_uio_rw_t dir, zfs_uio_t *uio)
{
	ASSERT3U(zfs_uio_rw(uio), ==, dir);
	return (uiomove(cp, n, dir, GET_UIO_STRUCT(uio)));
}

/*
 * Same as zfs_uiomove() but does not modify the uio structure.
 * Returns in *cbytes how many bytes were copied.
 */
int
zfs_uiocopy(void *p, size_t n, zfs_uio_rw_t rw, zfs_uio_t *uio,
    size_t *cbytes)
{
	uio_t uio_clone;
	iovec_t iov_clone[IOV_MAX_STACK];
	int iovcnt;
	int error;

	ASSERT3U(zfs_uio_rw(uio), ==, rw);

	iovcnt = zfs_uio_iovcnt(uio);

	/*
	 * Clone the uio so we do not modify the original.
	 */
	uio_clone = *(GET_UIO_STRUCT(uio));
	if (iovcnt <= IOV_MAX_STACK) {
		bcopy(GET_UIO_STRUCT(uio)->uio_iov, iov_clone,
		    iovcnt * sizeof (iovec_t));
		uio_clone.uio_iov = iov_clone;
	} else {
		iovec_t *iov = kmem_alloc(iovcnt * sizeof (iovec_t), KM_SLEEP);
		bcopy(GET_UIO_STRUCT(uio)->uio_iov, iov,
		    iovcnt * sizeof (iovec_t));
		uio_clone.uio_iov = iov;
	}

	error = uiomove(p, n, rw, &uio_clone);
	*cbytes = zfs_uio_resid(uio) - uio_clone.uio_resid;

	if (iovcnt > IOV_MAX_STACK)
		kmem_free(uio_clone.uio_iov, iovcnt * sizeof (iovec_t));

	return (error);
}

/*
 * Drop the next n chars out of *uio.
 */
void
zfs_uioskip(zfs_uio_t *uio, size_t n)
{
	zfs_uio_seg_t segflg;

	if (n > zfs_uio_resid(uio))
		return;

	segflg = zfs_uio_segflg(uio);
	zfs_uio_segflg(uio) = UIO_SYSSPACE;
	zfs_uiomove(NULL, n, zfs_uio_rw(uio), uio);
	zfs_uio_segflg(uio) = segflg;
}

int
zfs_uio_fault_move(void *p, size_t n, zfs_uio_rw_t dir, zfs_uio_t *uio)
{
	ASSERT3U(zfs_uio_rw(uio), ==, dir);
	return (uiomove(p, n, dir, GET_UIO_STRUCT(uio)));
}

/*
 * Check if the uio is page-aligned in memory.
 */
boolean_t
zfs_uio_page_aligned(zfs_uio_t *uio)
{
	const iovec_t *iov = GET_UIO_STRUCT(uio)->uio_iov;
	int i;

	for (i = zfs_uio_iovcnt(uio); i > 0; iov++, i--) {
		uintptr_t addr = (uintptr_t)iov->iov_base;
		size_t size = iov->iov_len;
		if ((addr & (PAGESIZE - 1)) || (size & (PAGESIZE - 1)))
			return (B_FALSE);
	}

	return (B_TRUE);
}

/*
 * Direct I/O page management stubs.
 *
 * Direct I/O (DIO) is a Linux-specific feature that pins user pages
 * directly.  illumos ZFS does not support DIO, so these are stubs.
 */
void
zfs_uio_free_dio_pages(zfs_uio_t *uio, zfs_uio_rw_t rw)
{
	(void) uio;
	(void) rw;
}

int
zfs_uio_get_dio_pages_alloc(zfs_uio_t *uio, zfs_uio_rw_t rw)
{
	(void) uio;
	(void) rw;
	return (SET_ERROR(ENOTSUP));
}
