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

#ifndef _SPL_SYS_MOD_H
#define	_SPL_SYS_MOD_H

/*
 * Module parameter interface for illumos.
 *
 * On illumos, ZFS module parameters are simply global variables declared
 * in their respective source files.  The ZFS_MODULE_PARAM macro expands
 * to nothing because the variable is already declared elsewhere.
 *
 * ZFS_MODULE_PARAM_CALL is used for parameters that require a custom
 * getter/setter function.  On illumos these also expand to nothing since
 * parameter access is handled through the ZFS ioctl/property interface.
 *
 * module_init/module_exit map to the illumos _init/_fini/_info model.
 * However since OpenZFS on illumos is built as a kernel module using the
 * standard modldrv/modlinkage pattern, module_init and module_exit are
 * called explicitly from _init and _fini respectively.
 */

/*
 * Permission flags for ZFS module parameters.
 * On illumos these are informational only; actual access control is
 * handled by the ZFS property/ioctl interface.
 */
#define	ZMOD_RW	0644
#define	ZMOD_RD	0444

/*
 * ZFS_MODULE_PARAM - declare a tunable module parameter.
 * On illumos the variable is already declared as a global; this macro
 * is a no-op.
 */
#define	ZFS_MODULE_PARAM(scope_prefix, name_prefix, name, type, perm, desc)

/*
 * ZFS_MODULE_PARAM_ARGS - argument list for a param_set_* function.
 * On illumos these functions take no special sysctl arguments.
 */
#define	ZFS_MODULE_PARAM_ARGS	void

/*
 * ZFS_MODULE_PARAM_CALL - declare a parameter with a custom handler.
 * On illumos this expands to nothing; the handler is invoked through
 * the ZFS property interface instead.
 */
#define	ZFS_MODULE_PARAM_CALL( \
    scope_prefix, name_prefix, name, func, _, perm, desc)

/*
 * ZFS_MODULE_VIRTUAL_PARAM_CALL - virtual parameter with custom handler.
 */
#define	ZFS_MODULE_VIRTUAL_PARAM_CALL ZFS_MODULE_PARAM_CALL

/*
 * module_init / module_exit
 *
 * OpenZFS common code uses these to register initialization and cleanup
 * functions.  On illumos, _init() calls module_init functions and _fini()
 * calls module_exit functions.  These macros register the functions in a
 * linker set so they can be invoked automatically.
 *
 * In practice, OpenZFS for illumos arranges for these to be called from
 * the top-level _init/_fini in zfs.c.  The macros themselves just provide
 * the function prototype with the expected signature.
 */
/*
 * Linux kernel module parameter macros — no-op on illumos.
 */
#define	module_param(a, b, c)
#define	module_param_call(a, b, c, d, e)
#define	module_param_named(a, b, c, d)
#define	MODULE_PARM_DESC(a, b)

#define	module_init(fn)		\
	void zfs_mod_init_##fn(void) { fn(); }

#define	module_init_early(fn)	\
	void zfs_mod_init_early_##fn(void) { fn(); }

#define	module_exit(fn)		\
	void zfs_mod_exit_##fn(void) { fn(); }

#endif	/* _SPL_SYS_MOD_H */
