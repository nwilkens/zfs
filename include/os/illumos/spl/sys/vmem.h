// SPDX-License-Identifier: CDDL-1.0
/*
 * Copyright 2025 OpenZFS Contributors. All rights reserved.
 */

#ifndef _SPL_SYS_VMEM_H
#define	_SPL_SYS_VMEM_H

/*
 * illumos has a real vmem arena allocator:
 *   vmem_alloc(vmem_t *vmp, size_t size, int vmflag)
 *   vmem_free(vmem_t *vmp, void *vaddr, size_t size)
 *
 * OpenZFS neutral code uses a simplified 2-arg interface:
 *   vmem_alloc(size_t size, int kmflags)
 *   vmem_free(void *ptr, size_t size)
 *
 * Chain to the real header first so vmem_t and the real functions
 * are available to other kernel headers, then redirect the names
 * that OpenZFS neutral code uses.
 */
#include_next <sys/vmem.h>
#include <sys/kmem.h>

/*
 * Save the real vmem functions under different names before
 * redefining.  Code that needs the real vmem arena allocator
 * (e.g., module/os/illumos/) should use the real_ prefixed versions
 * or call the functions by their original names before this header.
 */
#define	real_vmem_alloc	vmem_alloc
#define	real_vmem_free	vmem_free

/*
 * Redirect 2-arg OpenZFS vmem calls to kmem (following FreeBSD model).
 */
#undef vmem_alloc
#undef vmem_free
#undef vmem_zalloc

#define	vmem_alloc(size, flags)		kmem_alloc((size), (flags))
#define	vmem_free(ptr, size)		kmem_free((ptr), (size))
#define	vmem_zalloc(size, flags)	kmem_zalloc((size), (flags))

#endif	/* _SPL_SYS_VMEM_H */
