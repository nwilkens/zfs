// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

#ifndef _SPL_SYS_UIO_H
#define	_SPL_SYS_UIO_H

/*
 * illumos has native uio_t, iovec_t, uio_rw_t, etc.
 */
#include_next <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OpenZFS uses zfs_uio_t and zfs_uio_rw_t wrappers around native types.
 * On illumos the native uio_t is used directly — we wrap it minimally
 * so the platform-neutral code compiles.
 */
typedef	uio_rw_t	zfs_uio_rw_t;
typedef	uio_seg_t	zfs_uio_seg_t;

/*
 * uio_extflg: extended flags for Direct I/O
 */
#ifndef UIO_DIRECT
#define	UIO_DIRECT	0x0001
#endif

/*
 * Direct I/O page tracking.  On illumos, pages are represented by
 * page_t (struct page from <vm/page.h>).  We use struct page here
 * as a forward reference since uio.h is included before VM headers.
 */
typedef struct {
	struct page	**pages;
	int		npages;
} zfs_uio_dio_t;

typedef struct zfs_uio {
	uio_t		*uio;
	offset_t	uio_soffset;
	uint16_t	uio_extflg;
	zfs_uio_dio_t	uio_dio;
} zfs_uio_t;

#define	GET_UIO_STRUCT(u)	(u)->uio
#define	zfs_uio_segflg(u)	GET_UIO_STRUCT(u)->uio_segflg
#define	zfs_uio_offset(u)	GET_UIO_STRUCT(u)->uio_offset
#define	zfs_uio_resid(u)	GET_UIO_STRUCT(u)->uio_resid
#define	zfs_uio_iovcnt(u)	GET_UIO_STRUCT(u)->uio_iovcnt
#define	zfs_uio_iovlen(u, idx)	GET_UIO_STRUCT(u)->uio_iov[(idx)].iov_len
#define	zfs_uio_iovbase(u, idx)	GET_UIO_STRUCT(u)->uio_iov[(idx)].iov_base
#define	zfs_uio_rw(u)		GET_UIO_STRUCT(u)->uio_rw
#define	zfs_uio_soffset(u)	(u)->uio_soffset
#define	zfs_uio_fault_disable(u, set)
#define	zfs_uio_prefaultpages(size, u)	(0)

static inline void
zfs_uio_setoffset(zfs_uio_t *uio, offset_t off)
{
	zfs_uio_offset(uio) = off;
}

static inline void
zfs_uio_setsoffset(zfs_uio_t *uio, offset_t off)
{
	zfs_uio_soffset(uio) = off;
}

static inline void
zfs_uio_advance(zfs_uio_t *uio, ssize_t size)
{
	zfs_uio_resid(uio) -= size;
	zfs_uio_offset(uio) += size;
}

static inline void
zfs_uio_init(zfs_uio_t *uio, uio_t *uio_s)
{
	memset(uio, 0, sizeof (zfs_uio_t));
	if (uio_s != NULL) {
		GET_UIO_STRUCT(uio) = uio_s;
		zfs_uio_soffset(uio) = uio_s->uio_offset;
	}
}

int zfs_uio_fault_move(void *p, size_t n, zfs_uio_rw_t dir, zfs_uio_t *uio);

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_UIO_H */
