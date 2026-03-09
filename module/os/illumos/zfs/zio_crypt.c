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
 * OS-specific crypto wrappers for illumos.
 * The ICP (Illumos Crypto Port) provides all the crypto primitives.
 * The shared zio_crypt.c in module/zfs/ handles the main logic.
 * This file provides any illumos-specific glue, which currently
 * is minimal since ICP is native to illumos.
 */

#include <sys/zfs_context.h>
#include <sys/zio_crypt.h>
#include <sys/dmu.h>
#include <sys/fs/zfs.h>
#include <sys/zio.h>
