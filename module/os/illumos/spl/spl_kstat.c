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
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

/*
 * SPL kstat wrappers for illumos.
 *
 * illumos has a complete native kstat framework.  OpenZFS adds
 * "raw ops" (headers/data/addr callbacks) for procfs-like kstats.
 * The raw_ops are stored in the kstat_t structure; on illumos we
 * add them as opaque private data since the native kstat_t does
 * not have these fields.
 *
 * For now, these are stubs that store the callbacks.  The actual
 * rendering is handled by the procfs_list kstat adapter.
 */

#include <sys/zfs_context.h>
#include <sys/kstat.h>
