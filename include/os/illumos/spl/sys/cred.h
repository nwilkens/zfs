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

#ifndef _SPL_SYS_CRED_H
#define	_SPL_SYS_CRED_H

/*
 * illumos provides cred_t, CRED(), kcred, crgetuid, crgetruid, crgetgid,
 * crgetgroups, crgetngroups, crgetzoneid, crhold, crfree natively.
 *
 * Chain to the real header.
 */
#include_next <sys/cred.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * KUID_TO_SUID / KGID_TO_SGID — on illumos uid_t and gid_t are already
 * the "solaris" UID/GID, so these are identity mappings.
 */
#ifndef KUID_TO_SUID
#define	KUID_TO_SUID(x)		(x)
#endif

#ifndef KGID_TO_SGID
#define	KGID_TO_SGID(x)		(x)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* _SPL_SYS_CRED_H */
