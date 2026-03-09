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
 * CDDL HEADER END
 */

#ifndef _LIBSPL_ILLUMOS_SYS_PARAM_H
#define	_LIBSPL_ILLUMOS_SYS_PARAM_H

#include_next <sys/param.h>

/*
 * illumos already defines MAXNAMELEN, PAGESIZE, MAXUID, etc.
 * Provide any missing definitions that OpenZFS expects.
 */

#ifndef MAXPROJID
#define	MAXPROJID	MAXUID
#endif

#ifndef UID_NOACCESS
#define	UID_NOACCESS	60002
#endif

/*
 * ptob and roundup are kernel macros on illumos.
 * Provide them for userland use.
 */
#ifndef ptob
#define	ptob(x)		((x) * sysconf(_SC_PAGESIZE))
#endif

#ifndef roundup
#define	roundup(x, y)	((((x) + ((y) - 1)) / (y)) * (y))
#endif

extern size_t spl_pagesize(void);

#ifndef HAVE_EXECVPE
extern int execvpe(const char *name, char * const argv[], char * const envp[]);
#endif

#endif /* _LIBSPL_ILLUMOS_SYS_PARAM_H */
