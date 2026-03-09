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

/*
 * cmn_err_once — print a message only once (rate-limited).
 * illumos doesn't have this; just forward to cmn_err.
 */
void
cmn_err_once(int ce, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vcmn_err(ce, fmt, ap);
	va_end(ap);
}

/*
 * getcomm — return the name of the current process.
 */
const char *
getcomm(void)
{
	return (curproc->p_user.u_comm);
}

/*
 * thread_create_named — create a named kernel thread.
 * illumos thread_create doesn't take a name; ignore it.
 */
kthread_t *
thread_create_named(const char *name, caddr_t stk, size_t stksize,
    void (*proc)(void *), void *arg, size_t len, proc_t *pp, int state,
    pri_t pri)
{
	(void) name;
	return (thread_create(stk, stksize, proc, arg, len, pp, state, pri));
}

/*
 * vfs_ref / vrecycle — OpenZFS VFS helpers not in illumos.
 */
void
vfs_ref(vfs_t *vfsp)
{
	VFS_HOLD(vfsp);
}

void
vrecycle(vnode_t *vp)
{
	(void) vp;
}
