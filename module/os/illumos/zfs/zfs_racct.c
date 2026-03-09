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
 * Resource accounting stubs for illumos.
 * illumos does not have a racct framework; I/O stats are tracked
 * via spa_iostats only.
 */

#include <sys/zfs_context.h>
#include <sys/zfs_racct.h>

void
zfs_racct_read(spa_t *spa, uint64_t size, uint64_t iops, dmu_flags_t flags)
{
	spa_iostats_read_add(spa, size, iops, flags);
}

void
zfs_racct_write(spa_t *spa, uint64_t size, uint64_t iops, dmu_flags_t flags)
{
	spa_iostats_write_add(spa, size, iops, flags);
}
