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
 * SPL miscellaneous functions for illumos.
 *
 * On illumos, most SPL "misc" functions are provided natively:
 *   - zone_get_hostid()  - native in <sys/zone.h>
 *   - ddi_copyin()       - native in <sys/ddi.h>
 *   - ddi_copyout()      - native in <sys/ddi.h>
 *   - cmn_err()          - native in <sys/cmn_err.h>
 *
 * This file provides only functions that are NOT in the base illumos
 * kernel but are referenced by common OpenZFS code:
 *   - current_is_reclaim_thread() — always returns 0
 */

#include <sys/zfs_context.h>

/*
 * Check if the current thread is a memory reclaim thread.
 * On Linux, this checks if curproc is kswapd; on FreeBSD, pageproc.
 * On illumos, the page scanner is internal and there is no simple
 * user-accessible check.  Always return 0.
 */
int
current_is_reclaim_thread(void)
{
	return (0);
}
