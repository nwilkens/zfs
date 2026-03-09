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
 * SPL ACL functions for illumos.
 *
 * illumos has native NFSv4/POSIX ACL support with ace_t, acl_t,
 * ACE_* constants, acl_check(), etc.  On FreeBSD, this file provides
 * translation between ZFS ace_t and FreeBSD struct acl.  On illumos,
 * no such translation is needed because ZFS uses the native ace_t
 * format directly.
 *
 * This file is intentionally empty.
 */
