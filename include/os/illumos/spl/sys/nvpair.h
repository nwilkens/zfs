// SPDX-License-Identifier: CDDL-1.0
/*
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

#ifndef _SPL_SYS_NVPAIR_H
#define	_SPL_SYS_NVPAIR_H

/*
 * On illumos, nvlist/nvpair are native kernel types.  OpenZFS has its
 * own portable include/sys/nvpair.h.  We must ensure the native
 * definitions are used.
 *
 * Since both the native and OpenZFS headers use the same include
 * guard (_SYS_NVPAIR_H), we include the native header first and
 * set the guard so OpenZFS's version is skipped.
 *
 * In the kernel build, nvpair.h comes from the illumos-gate source
 * tree.  In userland, it comes from /usr/include.  The build system
 * must put the illumos-gate kernel headers BEFORE OpenZFS's include/
 * in the -I search path, OR this header must include by absolute path.
 */

/*
 * For now, just chain to the next header.  When building inside
 * illumos-gate, the Makefile ensures the kernel headers come before
 * OpenZFS's include/ directory.  In our test environment, we use
 * -isystem to prioritize the kernel headers.
 */
/*
 * Ensure va_list is available before any header that uses it.
 */
#include <sys/varargs.h>

#include_next <sys/nvpair.h>

#endif	/* _SPL_SYS_NVPAIR_H */
