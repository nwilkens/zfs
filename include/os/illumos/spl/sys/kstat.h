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
 * Copyright (c) 2024, OpenZFS Contributors.  All rights reserved.
 */

#ifndef _SPL_SYS_KSTAT_H
#define	_SPL_SYS_KSTAT_H

/*
 * illumos provides the complete kstat framework natively:
 *   kstat_t, kstat_named_t, kstat_io_t, kstat_intr_t, kstat_timer_t,
 *   kid_t, KSTAT_TYPE_*, KSTAT_DATA_*, KSTAT_FLAG_*, KSTAT_STRLEN,
 *   kstat_create, kstat_install, kstat_delete, kstat_named_init,
 *   kstat_runq_enter, kstat_runq_exit, kstat_waitq_enter, kstat_waitq_exit,
 *   KSTAT_NAMED_STR_PTR, KSTAT_NAMED_STR_BUFLEN, KSTAT_READ, KSTAT_WRITE.
 *
 * Chain to the real header.
 */
#include_next <sys/kstat.h>

#endif	/* _SPL_SYS_KSTAT_H */
