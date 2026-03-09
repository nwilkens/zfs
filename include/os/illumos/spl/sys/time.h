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

#ifndef _SPL_SYS_TIME_H
#define	_SPL_SYS_TIME_H

/*
 * illumos has hrtime_t, gethrtime(), timestruc_t, gethrestime() natively.
 * Just pull in the system header.
 */
#include_next <sys/time.h>

/*
 * getlrtime — "get lax resolution time".  On Linux this uses a cheaper
 * clock; on illumos gethrtime() is already fast, so just alias it.
 */
#ifndef getlrtime
#define	getlrtime()	gethrtime()
#endif

/*
 * SEC_TO_TICK / NSEC_TO_TICK — convert seconds/nanoseconds to clock ticks.
 */
#ifndef SEC_TO_TICK
#define	SEC_TO_TICK(sec)	((sec) * hz)
#endif
#ifndef NSEC_TO_TICK
#define	NSEC_TO_TICK(nsec)	((nsec) / (NANOSEC / hz))
#endif

#endif	/* _SPL_SYS_TIME_H */
