// SPDX-License-Identifier: CDDL-1.0
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
 * Copyright (c) 2025 by Oxide Computer Company
 */

#ifndef _SYS_TRACE_ZFS_H
#define	_SYS_TRACE_ZFS_H

/*
 * illumos has native DTrace support; the trace point infrastructure used
 * by Linux (trace_zfs.h / DEFINE_EVENT macros) is not needed here.
 *
 * DTrace SDT probes in ZFS are declared inline at their call sites using
 * the DTRACE_PROBE* macros from <sys/sdt.h>.  No stub definitions are
 * required in this header.
 */

#endif	/* _SYS_TRACE_ZFS_H */
