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

#ifndef _SPL_SYS_SIMD_X86_H
#define	_SPL_SYS_SIMD_X86_H

/*
 * x86 SIMD feature detection and FPU context management for illumos.
 *
 * illumos uses is_x86_feature(x86_featureset, X86FSET_*) for CPU feature
 * detection, and kernel_fpu_begin()/kernel_fpu_end() (or the kfpu KPI)
 * for FPU save/restore in the kernel.
 */

#include <sys/types.h>
#include <sys/x86_archext.h>

#ifdef _KERNEL
#include <sys/kfpu.h>
#endif

#define	kfpu_init()		(0)
#define	kfpu_fini()		do {} while (0)
#define	kfpu_allowed()		1
#define	kfpu_initialize(tsk)	do {} while (0)

/*
 * kfpu_begin / kfpu_end - save and restore FPU state around SIMD use.
 * illumos provides kernel_fpu_begin()/kernel_fpu_end() in the kernel.
 * Userland does not need FPU save/restore (the ABI guarantees it).
 */
#ifdef _KERNEL
#define	kfpu_begin()	kernel_fpu_begin(NULL, KFPU_USE_LWP)
#define	kfpu_end()	kernel_fpu_end(NULL, KFPU_USE_LWP)
#else
#define	kfpu_begin()	do {} while (0)
#define	kfpu_end()	do {} while (0)
#endif

/*
 * SIMD feature detection functions.
 *
 * illumos uses the x86_featureset[] array with is_x86_feature() for all
 * CPU capability detection.
 */

static inline boolean_t
zfs_sse_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SSE));
}

static inline boolean_t
zfs_sse2_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SSE2));
}

static inline boolean_t
zfs_sse3_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SSE3));
}

static inline boolean_t
zfs_ssse3_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SSSE3));
}

static inline boolean_t
zfs_sse4_1_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SSE4_1));
}

static inline boolean_t
zfs_sse4_2_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SSE4_2));
}

static inline boolean_t
zfs_avx_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX));
}

static inline boolean_t
zfs_avx2_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX2));
}

static inline boolean_t
zfs_shani_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_SHA));
}

static inline boolean_t
zfs_avx512f_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F));
}

static inline boolean_t
zfs_avx512cd_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512CD));
}

static inline boolean_t
zfs_avx512er_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512ER));
}

static inline boolean_t
zfs_avx512pf_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512PF));
}

static inline boolean_t
zfs_avx512bw_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512BW));
}

static inline boolean_t
zfs_avx512dq_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512DQ));
}

static inline boolean_t
zfs_avx512vl_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512VL));
}

static inline boolean_t
zfs_avx512ifma_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512FMA));
}

static inline boolean_t
zfs_avx512vbmi_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AVX512F) &&
	    is_x86_feature(x86_featureset, X86FSET_AVX512VBMI));
}

static inline boolean_t
zfs_aes_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_AES));
}

static inline boolean_t
zfs_pclmulqdq_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_PCLMULQDQ));
}

static inline boolean_t
zfs_movbe_available(void)
{
	/*
	 * illumos does not track MOVBE in x86_featureset.
	 * Return B_FALSE conservatively; the GCM code has
	 * a non-MOVBE fallback path.
	 */
	return (B_FALSE);
}

static inline boolean_t
zfs_vaes_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_VAES));
}

static inline boolean_t
zfs_vpclmulqdq_available(void)
{
	return (is_x86_feature(x86_featureset, X86FSET_VPCLMULQDQ));
}

static inline boolean_t
zfs_sha512ext_available(void)
{
	return (B_FALSE);
}

#endif	/* _SPL_SYS_SIMD_X86_H */
