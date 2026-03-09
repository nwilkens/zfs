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

#ifndef _SPL_SYS_RANDOM_H
#define	_SPL_SYS_RANDOM_H

/*
 * illumos has random_get_bytes() and random_get_pseudo_bytes() natively
 * in <sys/random.h>.
 */
#include_next <sys/random.h>

/*
 * random_in_range — return a pseudo-random number in [0, range).
 * Linux uses prandom_u32_max, FreeBSD uses prng32_bounded.
 * On illumos, use random_get_pseudo_bytes to generate the value.
 */
static inline uint32_t
random_in_range(uint32_t range)
{
	uint32_t r;

	if (range <= 1)
		return (0);

	random_get_pseudo_bytes((uint8_t *)&r, sizeof (r));
	return (r % range);
}

#endif	/* _SPL_SYS_RANDOM_H */
