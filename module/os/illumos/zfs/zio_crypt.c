/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source. A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2017, Datto, Inc. All rights reserved.
 * Copyright (c) 2025, OpenZFS on illumos.
 *
 * ZIO encryption/decryption routines for illumos.
 *
 * The canonical implementation lives in module/os/linux/zfs/zio_crypt.c.
 * It is largely portable (uses the ICP API), but accesses zfs_uio_t
 * members directly (uio_iov, uio_iovcnt, uio_segflg), which on
 * illumos are behind the GET_UIO_STRUCT() wrapper.
 *
 * We solve this by providing a local "crypto_uio_t" type that has the
 * same layout as a native illumos uio_t for ICP consumption, and
 * redefining zfs_uio_t to be that type for the duration of this file.
 * Then we #include the Linux source.
 */

/*
 * First, pull in all the headers that zio_crypt.c needs, which will
 * define the real zfs_uio_t.  Then we undefine and redefine it.
 */
#include <sys/zio_crypt.h>
#include <sys/dmu.h>
#include <sys/dmu_objset.h>
#include <sys/dnode.h>
#include <sys/fs/zfs.h>
#include <sys/zio.h>
#include <sys/zil.h>
#include <sys/sha2.h>
#include <sys/hkdf.h>
#include <sys/qat.h>

/*
 * Now redefine zfs_uio_t to be the native uio_t for this file.
 * The Linux zio_crypt.c uses zfs_uio_t with direct member access
 * (uio_iov, uio_iovcnt, uio_segflg).  The native illumos uio_t
 * has these members, so the code will compile.
 */
#undef zfs_uio_t
#define	zfs_uio_t	uio_t

/* Suppress Linux module parameter macros */
#define	module_param(name, type, perm)
#define	MODULE_PARM_DESC(name, desc)

/*
 * Prevent the included file from re-including headers we already have.
 * The include guards will handle most, but we need to prevent the
 * file from seeing our zfs_uio_t wrapper typedef again.
 */

/*
 * Now include the actual implementation.  The relative path goes from
 * module/os/illumos/zfs/ up to module/ then into os/linux/zfs/.
 */
#include "../../../os/linux/zfs/zio_crypt.c"
