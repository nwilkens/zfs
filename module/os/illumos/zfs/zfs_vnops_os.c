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
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2017 by Delphix. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 * Copyright 2020 Joyent, Inc.
 * Copyright (c) 2025, OpenZFS on illumos.
 */

/* Portions Copyright 2007 Jeremy Teo */
/* Portions Copyright 2010 Robert Milkowski */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <sys/vfs_opreg.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/kmem.h>
#include <sys/taskq.h>
#include <sys/uio.h>
#include <sys/atomic.h>
#include <sys/mman.h>
#include <sys/pathname.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/zfs_dir.h>
#include <sys/zfs_acl.h>
#include <sys/zfs_ioctl.h>
#include <sys/fs/zfs.h>
#include <sys/dmu.h>
#include <sys/dmu_objset.h>
#include <sys/spa.h>
#include <sys/txg.h>
#include <sys/dbuf.h>
#include <sys/zap.h>
#include <sys/sa.h>
#include <sys/dirent.h>
#include <sys/policy.h>
#include <sys/sunddi.h>
#include <sys/filio.h>
#include <sys/sid.h>
#include <sys/zfs_ctldir.h>
#include <sys/zfs_fuid.h>
#include <sys/zfs_sa.h>
#include <sys/zfs_rlock.h>
#include <sys/zfs_vnops.h>
#include <sys/zfs_znode.h>
#include <sys/zfs_vfsops.h>
#include <sys/zfs_project.h>
#include <sys/zil.h>
#include <sys/sa_impl.h>
#include "fs/fs_subr.h"

/*
 * illumos kernel global: non-zero when kernel page mapping (segkpm) is
 * available.  Used by zfs_map_page()/zfs_unmap_page() to choose between
 * hat_kpm_mapin() and ppmapin().
 */
extern int kpm_enable;

/*
 * Programming rules.
 *
 * Each vnode op performs some logical unit of work.  To do this, the ZPL must
 * properly lock its in-core state, create a DMU transaction, do the work,
 * record this work in the intent log (ZIL), commit the DMU transaction,
 * and wait for the intent log to commit if it is a synchronous operation.
 * Moreover, the vnode ops must work in both normal and log replay context.
 * The ordering of events is important to avoid deadlocks and references
 * to freed memory.  The example below illustrates the following Big Rules:
 *
 *  (1) A check must be made in each zfs thread for a mounted file system.
 *	This is done avoiding races using zfs_enter(zfsvfs).
 *	A zfs_exit(zfsvfs) is needed before all returns.  Any znodes
 *	must be checked with zfs_verify_zp(zp).  Both of these macros
 *	can return EIO from the calling function.
 *
 *  (2) VN_RELE() should always be the last thing except for zil_commit()
 *	(if necessary) and zfs_exit(). This is for 3 reasons:
 *	First, if it's the last reference, the vnode/znode
 *	can be freed, so the zp may point to freed memory.  Second, the last
 *	reference will call zfs_zinactive(), which may induce a lot of work --
 *	pushing cached pages (which acquires range locks) and syncing out
 *	cached atime changes.  Third, zfs_zinactive() may require a new tx,
 *	which could deadlock the system if you were already holding one.
 *	If you must call VN_RELE() within a tx then use VN_RELE_ASYNC().
 *
 *  (3) All range locks must be grabbed before calling dmu_tx_assign(),
 *	as they can span dmu_tx_assign() calls.
 *
 *  (4) If ZPL locks are held, pass DMU_TX_NOWAIT as the second argument to
 *      dmu_tx_assign().  This is critical because we don't want to block
 *      while holding locks.
 *
 *	If no ZPL locks are held (aside from zfs_enter()), use DMU_TX_WAIT.
 *	This reduces lock contention and CPU usage when we must wait (note
 *	that if throughput is constrained by the storage, nearly every
 *	transaction must wait).
 *
 *  (5) If the operation succeeded, generate the intent log entry for it
 *	before dropping locks.  This ensures that the ordering of events
 *	in the intent log matches the order in which they actually occurred.
 *
 *  (6) At the end of each vnode op, the DMU tx must always commit,
 *	regardless of whether there were any errors.
 *
 *  (7) After dropping all locks, invoke zil_commit(zilog, foid)
 *	to ensure that synchronous semantics are provided when necessary.
 */

/* ARGSUSED */
static int
zfs_open(vnode_t **vpp, int flag, cred_t *cr, caller_context_t *ct)
{
	znode_t	*zp = VTOZ(*vpp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	if ((flag & FWRITE) && (zp->z_pflags & ZFS_APPENDONLY) &&
	    ((flag & FAPPEND) == 0)) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EPERM));
	}

	if (!zfs_has_ctldir(zp) && zp->z_zfsvfs->z_vscan &&
	    ZTOV(zp)->v_type == VREG &&
	    !(zp->z_pflags & ZFS_AV_QUARANTINED) && zp->z_size > 0) {
		if (fs_vscan(*vpp, cr, 0) != 0) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EACCES));
		}
	}

	/*
	 * Keep a count of the synchronous opens in the znode.  On first
	 * synchronous open we must convert all previous async transactions
	 * into sync to keep correct ordering.
	 */
	if (flag & (FSYNC | FDSYNC)) {
		if (atomic_inc_32_nv(&zp->z_sync_cnt) == 1)
			zil_async_to_sync(zfsvfs->z_log, zp->z_id);
	}

	zfs_exit(zfsvfs, FTAG);
	return (0);
}

/* ARGSUSED */
static int
zfs_close(vnode_t *vp, int flag, int count, offset_t offset, cred_t *cr,
    caller_context_t *ct)
{
	znode_t	*zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	/*
	 * Clean up any locks held by this process on the vp.
	 */
	cleanlocks(vp, ddi_get_pid(), 0);
	cleanshares(vp, ddi_get_pid());

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	/* Decrement the synchronous opens in the znode */
	if ((flag & (FSYNC | FDSYNC)) && (count == 1))
		atomic_dec_32(&zp->z_sync_cnt);

	if (!zfs_has_ctldir(zp) && zp->z_zfsvfs->z_vscan &&
	    ZTOV(zp)->v_type == VREG &&
	    !(zp->z_pflags & ZFS_AV_QUARANTINED) && zp->z_size > 0)
		VERIFY(fs_vscan(vp, cr, 1) == 0);

	zfs_exit(zfsvfs, FTAG);
	return (0);
}

/*
 * Lseek support for finding holes (cmd == _FIO_SEEK_HOLE) and
 * data (cmd == _FIO_SEEK_DATA). "off" is an in/out parameter.
 */
static int
zfs_holey_illumos(vnode_t *vp, int cmd, offset_t *off)
{
	znode_t	*zp = VTOZ(vp);
	uint64_t noff = (uint64_t)*off; /* new offset */
	uint64_t file_sz;
	int error;
	boolean_t hole;

	file_sz = zp->z_size;
	if (noff >= file_sz) {
		return (SET_ERROR(ENXIO));
	}

	if (cmd == _FIO_SEEK_HOLE)
		hole = B_TRUE;
	else
		hole = B_FALSE;

	error = dmu_offset_next(zp->z_zfsvfs->z_os, zp->z_id, hole, &noff);

	if (error == ESRCH)
		return (SET_ERROR(ENXIO));

	/*
	 * We could find a hole that begins after the logical end-of-file,
	 * because dmu_offset_next() only works on whole blocks.  If the
	 * EOF falls mid-block, then indicate that the "virtual hole"
	 * at the end of the file begins at the logical EOF, rather than
	 * at the end of the last block.
	 */
	if (noff > file_sz) {
		ASSERT(hole);
		noff = file_sz;
	}

	if (noff < *off)
		return (error);
	*off = noff;
	return (error);
}

/* ARGSUSED */
static int
zfs_ioctl(vnode_t *vp, int com, intptr_t data, int flag, cred_t *cred,
    int *rvalp, caller_context_t *ct)
{
	offset_t off;
	int error;
	zfsvfs_t *zfsvfs;
	znode_t *zp;

	switch (com) {
	case _FIOFFS:
	{
		return (zfs_sync(vp->v_vfsp, 0, cred));
	}
	case _FIOGDIO:
	case _FIOSDIO:
	{
		return (0);
	}

	case _FIO_SEEK_DATA:
	case _FIO_SEEK_HOLE:
	{
		if (ddi_copyin((void *)data, &off, sizeof (off), flag))
			return (SET_ERROR(EFAULT));

		zp = VTOZ(vp);
		zfsvfs = zp->z_zfsvfs;
		if ((error = zfs_enter(zfsvfs, FTAG)) != 0)
			return (error);

		/* offset parameter is in/out */
		error = zfs_holey_illumos(vp, com, &off);
		zfs_exit(zfsvfs, FTAG);
		if (error)
			return (error);
		if (ddi_copyout(&off, (void *)data, sizeof (off), flag))
			return (SET_ERROR(EFAULT));
		return (0);
	}
	}
	return (SET_ERROR(ENOTTY));
}

/*
 * Utility functions to map and unmap a single physical page.  These
 * are used to manage the mappable copies of ZFS file data, and therefore
 * do not update ref/mod bits.
 */
caddr_t
zfs_map_page(page_t *pp, enum seg_rw rw)
{
	if (kpm_enable)
		return (hat_kpm_mapin(pp, 0));
	ASSERT(rw == S_READ || rw == S_WRITE);
	return (ppmapin(pp, PROT_READ | ((rw == S_WRITE) ? PROT_WRITE : 0),
	    (caddr_t)-1));
}

void
zfs_unmap_page(page_t *pp, caddr_t addr)
{
	if (kpm_enable) {
		hat_kpm_mapout(pp, 0, addr);
	} else {
		ppmapout(addr);
	}
}

/*
 * When a file is memory mapped, we must keep the IO data synchronized
 * between the DMU cache and the memory mapped pages.  What this means:
 *
 * On Write:	If we find a memory mapped page, we write to *both*
 *		the page and the dmu buffer.
 */
void
update_pages(znode_t *zp, int64_t start, int len, objset_t *os)
{
	vnode_t *vp = ZTOV(zp);
	int64_t	off;

	off = start & PAGEOFFSET;
	for (start &= PAGEMASK; len > 0; start += PAGESIZE) {
		page_t *pp;
		uint64_t nbytes = MIN(PAGESIZE - off, len);

		if ((pp = page_lookup(vp, start, SE_SHARED)) != NULL) {
			caddr_t va;

			va = zfs_map_page(pp, S_WRITE);
			(void) dmu_read(os, zp->z_id, start + off, nbytes,
			    va + off, DMU_READ_PREFETCH);
			zfs_unmap_page(pp, va);
			page_unlock(pp);
		}
		len -= nbytes;
		off = 0;
	}
}

/*
 * When a file is memory mapped, we must keep the IO data synchronized
 * between the DMU cache and the memory mapped pages.  What this means:
 *
 * On Read:	We "read" preferentially from memory mapped pages,
 *		else we default from the dmu buffer.
 *
 * NOTE: We will always "break up" the IO into PAGESIZE uiomoves when
 *	 the file is memory mapped.
 */
int
mappedread(znode_t *zp, int nbytes, zfs_uio_t *uio)
{
	vnode_t *vp = ZTOV(zp);
	int64_t	start, off;
	int len = nbytes;
	int error = 0;

	start = zfs_uio_offset(uio);
	off = start & PAGEOFFSET;
	for (start &= PAGEMASK; len > 0; start += PAGESIZE) {
		page_t *pp;
		uint64_t bytes = MIN(PAGESIZE - off, len);

		if ((pp = page_lookup(vp, start, SE_SHARED)) != NULL) {
			caddr_t va;

			va = zfs_map_page(pp, S_READ);
			error = zfs_uiomove(va + off, bytes, UIO_READ, uio);
			zfs_unmap_page(pp, va);
			page_unlock(pp);
		} else {
			error = dmu_read_uio_dbuf(sa_get_db(zp->z_sa_hdl),
			    uio, bytes, DMU_READ_PREFETCH);
		}
		len -= bytes;
		off = 0;
		if (error)
			break;
	}
	return (error);
}

/*
 * zfs_access - illumos VOP_ACCESS implementation.
 *
 * Check file access permissions.
 *
 *	IN:	vp	- vnode of file to check.
 *		mode	- access mode requested (VREAD, VWRITE, VEXEC).
 *		flag	- additional flags.
 *		cr	- credentials of caller.
 *		ct	- caller context.
 *
 *	RETURN:	0 on success, error code on failure.
 */
/* ARGSUSED */
static int
zfs_access_vnop(vnode_t *vp, int mode, int flag, cred_t *cr,
    caller_context_t *ct)
{
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	if (flag & V_ACE_MASK)
		error = zfs_zaccess(zp, mode, flag, B_FALSE, cr, NULL);
	else
		error = zfs_zaccess_rwx(zp, mode, flag, cr, NULL);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * zfs_lookup - illumos VOP_LOOKUP implementation.
 *
 * Lookup a name in a directory.
 *
 *	IN:	dvp	- vnode of directory to search.
 *		nm	- name of entry to lookup.
 *		pnp	- full pathname to lookup [UNUSED].
 *		flags	- LOOKUP_XATTR set if looking for xattrs, FIGNORECASE.
 *		rdir	- root directory vnode [UNUSED].
 *		cr	- credentials of caller.
 *		ct	- caller context.
 *		direntflags - directory lookup flags.
 *		realpnp - returned pathname.
 *
 *	OUT:	vpp	- vnode of located entry, NULL if not found.
 *
 *	RETURN:	0 on success, error code on failure.
 */
/* ARGSUSED */
static int
zfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, struct pathname *pnp,
    int flags, vnode_t *rdir, cred_t *cr, caller_context_t *ct,
    int *direntflags, pathname_t *realpnp)
{
	znode_t *zdp = VTOZ(dvp);
	zfsvfs_t *zfsvfs = zdp->z_zfsvfs;
	int error = 0;

	/*
	 * Fast path lookup, does not enter the ZFS layer.
	 */
	if (!(flags & (LOOKUP_XATTR | FIGNORECASE))) {
		if (dvp->v_type != VDIR)
			return (SET_ERROR(ENOTDIR));

		if ((error = zfs_enter_verify_zp(zfsvfs, zdp, FTAG)) != 0)
			return (error);

		*vpp = NULL;

		if (zfsvfs->z_show_ctldir) {
			if (strcmp(nm, ZFS_CTLDIR_NAME) == 0) {
				*vpp = zfsvfs->z_ctldir;
				if (*vpp != NULL) {
					VN_HOLD(*vpp);
				}
				zfs_exit(zfsvfs, FTAG);
				return (*vpp == NULL ? ENOENT : 0);
			}
		}

		/*
		 * Handle ".." specially -- see zfs_dir.c comments.
		 */
		if (strcmp(nm, "..") == 0) {
			error = zfs_lookup_lock(zdp, nm, vpp, 0);
		} else {
			error = zfs_dirlook(zdp, nm, vpp, flags, NULL, NULL);
		}

		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	if ((error = zfs_enter_verify_zp(zfsvfs, zdp, FTAG)) != 0)
		return (error);

	*vpp = NULL;

	if (flags & LOOKUP_XATTR) {
		/*
		 * If the xattr property is off, refuse the lookup request.
		 */
		if (!(zfsvfs->z_vfs->vfs_flag & VFS_XATTR)) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EINVAL));
		}

		/*
		 * We don't allow recursive attributes..
		 * Maybe someday we will.
		 */
		if (zdp->z_pflags & ZFS_XATTR) {
			zfs_exit(zfsvfs, FTAG);
			return (SET_ERROR(EINVAL));
		}

		if ((error = zfs_get_xattrdir(zdp, vpp, cr, flags)) != 0) {
			zfs_exit(zfsvfs, FTAG);
			return (error);
		}

		/*
		 * Do we have permission to get into attribute directory?
		 */
		if ((error = zfs_zaccess(VTOZ(*vpp), ACE_EXECUTE, 0,
		    B_FALSE, cr, NULL)) != 0) {
			VN_RELE(*vpp);
			*vpp = NULL;
		}

		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	if (dvp->v_type != VDIR) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(ENOTDIR));
	}

	/*
	 * Check accessibility of directory.
	 */
	if ((error = zfs_zaccess(zdp, ACE_EXECUTE, 0, B_FALSE, cr, NULL)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	if (zfsvfs->z_utf8 && u8_validate(nm, strlen(nm),
	    NULL, U8_VALIDATE_ENTIRE, &error) < 0) {
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(EILSEQ));
	}

	error = zfs_dirlook(zdp, nm, vpp, flags, direntflags, realpnp);

	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/*
 * zfs_getattr - illumos VOP_GETATTR implementation.
 *
 *	IN:	vp	- vnode of file.
 *		vap	- va_mask identifies requested attributes.
 *			  If AT_XVATTR set, then optional attrs are requested.
 *		flags	- ATTR_NOACLCHECK (CIFS server context).
 *		cr	- credentials of caller.
 *		ct	- caller context.
 *
 *	OUT:	vap	- attribute values.
 *
 *	RETURN:	0 (always succeeds).
 */
/* ARGSUSED */
static int
zfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr,
    caller_context_t *ct)
{
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error = 0;
	uint64_t links;
	uint64_t mtime[2], ctime[2];
	xvattr_t *xvap = (xvattr_t *)vap;	/* vap may be an xvattr_t * */
	xoptattr_t *xoap = NULL;
	boolean_t skipaclchk = (flags & ATTR_NOACLCHECK) ? B_TRUE : B_FALSE;
	sa_bulk_attr_t bulk[2];
	int count = 0;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	zfs_fuid_map_ids(zp, cr, &vap->va_uid, &vap->va_gid);

	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_MTIME(zfsvfs), NULL, &mtime, 16);
	SA_ADD_BULK_ATTR(bulk, count, SA_ZPL_CTIME(zfsvfs), NULL, &ctime, 16);

	if ((error = sa_bulk_lookup(zp->z_sa_hdl, bulk, count)) != 0) {
		zfs_exit(zfsvfs, FTAG);
		return (error);
	}

	/*
	 * If ACL is trivial don't bother looking for ACE_READ_ATTRIBUTES.
	 * Also, if we are the owner don't bother, since owner should
	 * always be allowed to read basic attributes of file.
	 */
	if (!(zp->z_pflags & ZFS_ACL_TRIVIAL) &&
	    (vap->va_uid != crgetuid(cr))) {
		if ((error = zfs_zaccess(zp, ACE_READ_ATTRIBUTES, 0,
		    skipaclchk, cr, NULL)) != 0) {
			zfs_exit(zfsvfs, FTAG);
			return (error);
		}
	}

	/*
	 * Return all attributes.  It's cheaper to provide the answer
	 * than to determine whether we were asked the question.
	 */

	mutex_enter(&zp->z_lock);
	vap->va_type = vp->v_type;
	vap->va_mode = zp->z_mode & MODEMASK;
	vap->va_fsid = zp->z_zfsvfs->z_vfs->vfs_dev;
	vap->va_nodeid = zp->z_id;
	if ((vp->v_flag & VROOT) && zfs_show_ctldir(zp))
		links = zp->z_links + 1;
	else
		links = zp->z_links;
	vap->va_nlink = MIN(links, UINT32_MAX);	/* nlink_t limit */
	vap->va_size = zp->z_size;
	vap->va_rdev = vp->v_rdev;
	vap->va_seq = zp->z_seq;

	/*
	 * Add in any requested optional attributes and the create time.
	 * Also set the corresponding bits in the returned attribute bitmap.
	 */
	if ((xoap = xva_getxoptattr(xvap)) != NULL && zfsvfs->z_use_fuids) {
		if (XVA_ISSET_REQ(xvap, XAT_ARCHIVE)) {
			xoap->xoa_archive =
			    ((zp->z_pflags & ZFS_ARCHIVE) != 0);
			XVA_SET_RTN(xvap, XAT_ARCHIVE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_READONLY)) {
			xoap->xoa_readonly =
			    ((zp->z_pflags & ZFS_READONLY) != 0);
			XVA_SET_RTN(xvap, XAT_READONLY);
		}

		if (XVA_ISSET_REQ(xvap, XAT_SYSTEM)) {
			xoap->xoa_system =
			    ((zp->z_pflags & ZFS_SYSTEM) != 0);
			XVA_SET_RTN(xvap, XAT_SYSTEM);
		}

		if (XVA_ISSET_REQ(xvap, XAT_HIDDEN)) {
			xoap->xoa_hidden =
			    ((zp->z_pflags & ZFS_HIDDEN) != 0);
			XVA_SET_RTN(xvap, XAT_HIDDEN);
		}

		if (XVA_ISSET_REQ(xvap, XAT_NOUNLINK)) {
			xoap->xoa_nounlink =
			    ((zp->z_pflags & ZFS_NOUNLINK) != 0);
			XVA_SET_RTN(xvap, XAT_NOUNLINK);
		}

		if (XVA_ISSET_REQ(xvap, XAT_IMMUTABLE)) {
			xoap->xoa_immutable =
			    ((zp->z_pflags & ZFS_IMMUTABLE) != 0);
			XVA_SET_RTN(xvap, XAT_IMMUTABLE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_APPENDONLY)) {
			xoap->xoa_appendonly =
			    ((zp->z_pflags & ZFS_APPENDONLY) != 0);
			XVA_SET_RTN(xvap, XAT_APPENDONLY);
		}

		if (XVA_ISSET_REQ(xvap, XAT_NODUMP)) {
			xoap->xoa_nodump =
			    ((zp->z_pflags & ZFS_NODUMP) != 0);
			XVA_SET_RTN(xvap, XAT_NODUMP);
		}

		if (XVA_ISSET_REQ(xvap, XAT_OPAQUE)) {
			xoap->xoa_opaque =
			    ((zp->z_pflags & ZFS_OPAQUE) != 0);
			XVA_SET_RTN(xvap, XAT_OPAQUE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_QUARANTINED)) {
			xoap->xoa_av_quarantined =
			    ((zp->z_pflags & ZFS_AV_QUARANTINED) != 0);
			XVA_SET_RTN(xvap, XAT_AV_QUARANTINED);
		}

		if (XVA_ISSET_REQ(xvap, XAT_AV_MODIFIED)) {
			xoap->xoa_av_modified =
			    ((zp->z_pflags & ZFS_AV_MODIFIED) != 0);
			XVA_SET_RTN(xvap, XAT_AV_MODIFIED);
		}

		if (XVA_ISSET_REQ(xvap, XAT_CREATETIME)) {
			uint64_t times[2];

			(void) sa_lookup(zp->z_sa_hdl, SA_ZPL_CRTIME(zfsvfs),
			    times, sizeof (times));
			ZFS_TIME_DECODE(&xoap->xoa_createtime, times);
			XVA_SET_RTN(xvap, XAT_CREATETIME);
		}

		if (XVA_ISSET_REQ(xvap, XAT_REPARSE)) {
			xoap->xoa_reparse =
			    ((zp->z_pflags & ZFS_REPARSE) != 0);
			XVA_SET_RTN(xvap, XAT_REPARSE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_GEN)) {
			xoap->xoa_generation = zp->z_gen;
			XVA_SET_RTN(xvap, XAT_GEN);
		}

		if (XVA_ISSET_REQ(xvap, XAT_OFFLINE)) {
			xoap->xoa_offline =
			    ((zp->z_pflags & ZFS_OFFLINE) != 0);
			XVA_SET_RTN(xvap, XAT_OFFLINE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_SPARSE)) {
			xoap->xoa_sparse =
			    ((zp->z_pflags & ZFS_SPARSE) != 0);
			XVA_SET_RTN(xvap, XAT_SPARSE);
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJINHERIT)) {
			xoap->xoa_projinherit =
			    ((zp->z_pflags & ZFS_PROJINHERIT) != 0);
			XVA_SET_RTN(xvap, XAT_PROJINHERIT);
		}

		if (XVA_ISSET_REQ(xvap, XAT_PROJID)) {
			xoap->xoa_projid = zp->z_projid;
			XVA_SET_RTN(xvap, XAT_PROJID);
		}
	}

	ZFS_TIME_DECODE(&vap->va_atime, zp->z_atime);
	ZFS_TIME_DECODE(&vap->va_mtime, mtime);
	ZFS_TIME_DECODE(&vap->va_ctime, ctime);

	mutex_exit(&zp->z_lock);

	sa_object_size(zp->z_sa_hdl, &vap->va_blksize, &vap->va_nblocks);

	if (zp->z_blksz == 0) {
		/*
		 * Block size hasn't been set; suggest maximal I/O transfers.
		 */
		vap->va_blksize = zfsvfs->z_max_blksz;
	}

	zfs_exit(zfsvfs, FTAG);
	return (0);
}

/*
 * Stubs for complex vnode operations.
 *
 * These return ENOTSUP so that the build can proceed while proper
 * implementations are added incrementally.
 */

/* ARGSUSED */
int
zfs_setattr(znode_t *zp, vattr_t *vap, int flag, cred_t *cr,
    zidmap_t *mnt_ns)
{
	return (SET_ERROR(ENOTSUP));
}


/* ARGSUSED */
int
zfs_create(znode_t *dzp, const char *name, vattr_t *vap, int excl,
    int mode, znode_t **zpp, cred_t *cr, int flag, vsecattr_t *vsecp,
    zidmap_t *mnt_ns)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_remove(znode_t *dzp, const char *name, cred_t *cr, int flags)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_mkdir(znode_t *dzp, const char *dirname, vattr_t *vap,
    znode_t **zpp, cred_t *cr, int flags, vsecattr_t *vsecp,
    zidmap_t *mnt_ns)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_rmdir(znode_t *dzp, const char *name, znode_t *cwd,
    cred_t *cr, int flags)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
static int
zfs_readdir(vnode_t *vp, uio_t *uio, cred_t *cr, int *eofp,
    caller_context_t *ct, int flags)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_symlink(znode_t *dzp, const char *name, vattr_t *vap,
    const char *link, znode_t **zpp, cred_t *cr, int flags,
    zidmap_t *mnt_ns)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
static int
zfs_readlink(vnode_t *vp, uio_t *uio, cred_t *cr, caller_context_t *ct)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_link(znode_t *tdzp, znode_t *sp, const char *name,
    cred_t *cr, int flags)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_rename(znode_t *sdzp, const char *snm, znode_t *tdzp,
    const char *tnm, cred_t *cr, int flags, uint64_t rflags,
    vattr_t *wo_vap, zidmap_t *mnt_ns)
{
	return (SET_ERROR(ENOTSUP));
}

/* ARGSUSED */
int
zfs_space(znode_t *zp, int cmd, flock64_t *bfp, int flag,
    offset_t offset, cred_t *cr)
{
	return (SET_ERROR(ENOTSUP));
}

/*
 * The seek function is called to validate the final offset when an
 * lseek(2) is performed.
 */
/* ARGSUSED */
static int
zfs_seek(vnode_t *vp, offset_t ooff, offset_t *noffp,
    caller_context_t *ct)
{
	if (vp->v_type == VDIR)
		return (0);
	return ((*noffp < 0 || *noffp > MAXOFFSET_T) ? EINVAL : 0);
}

/* ARGSUSED */
static int
zfs_inactive(vnode_t *vp, cred_t *cr, caller_context_t *ct)
{
	znode_t	*zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	rw_enter(&zfsvfs->z_teardown_inactive_lock, RW_READER);
	if (zp->z_sa_hdl == NULL) {
		/*
		 * The fs has been unmounted, or we did a
		 * temporary hold on this vnode due to coming
		 * from a rezget.
		 */
		rw_exit(&zfsvfs->z_teardown_inactive_lock);
		vrecycle(vp);
		return (0);
	}

	if (zp->z_unlinked) {
		/*
		 * Fast path to recycle a vnode of a removed file.
		 */
		rw_exit(&zfsvfs->z_teardown_inactive_lock);
		vrecycle(vp);
		return (0);
	}

	if (zp->z_atime_dirty && zp->z_unlinked == 0) {
		dmu_tx_t *tx = dmu_tx_create(zfsvfs->z_os);

		dmu_tx_hold_sa(tx, zp->z_sa_hdl, B_FALSE);
		zfs_sa_upgrade_txholds(tx, zp);
		error = dmu_tx_assign(tx, DMU_TX_WAIT);
		if (error) {
			dmu_tx_abort(tx);
		} else {
			(void) sa_update(zp->z_sa_hdl, SA_ZPL_ATIME(zfsvfs),
			    (void *)&zp->z_atime, sizeof (zp->z_atime), tx);
			zp->z_atime_dirty = 0;
			dmu_tx_commit(tx);
		}
	}

	rw_exit(&zfsvfs->z_teardown_inactive_lock);
	return (0);
}

/* ARGSUSED */
static int
zfs_fid(vnode_t *vp, fid_t *fidp, caller_context_t *ct)
{
	znode_t		*zp = VTOZ(vp);
	zfsvfs_t	*zfsvfs = zp->z_zfsvfs;
	uint32_t	gen;
	uint64_t	gen64;
	uint64_t	object;
	zfid_short_t	*zfid;
	int		size, i, error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);

	if (fidp->fid_len < SHORT_FID_LEN) {
		fidp->fid_len = SHORT_FID_LEN;
		zfs_exit(zfsvfs, FTAG);
		return (SET_ERROR(ENOSPC));
	}

	object = zp->z_id;
	zfid = (zfid_short_t *)fidp;

	zfid->zf_len = SHORT_FID_LEN;

	for (i = 0; i < sizeof (zfid->zf_object); i++)
		zfid->zf_object[i] = (uint8_t)(object >> (8 * i));

	/* .zfs znodes always have a generation number of 0 */
	if (zp->z_id == zfsvfs->z_root && zfs_has_ctldir(zp)) {
		gen = 0;
	} else {
		(void) sa_lookup(zp->z_sa_hdl, SA_ZPL_GEN(zfsvfs), &gen64,
		    sizeof (gen64));
		gen = (uint32_t)gen64;
	}
	for (i = 0; i < sizeof (zfid->zf_gen); i++)
		zfid->zf_gen[i] = (uint8_t)(gen >> (8 * i));

	zfs_exit(zfsvfs, FTAG);
	return (0);
}

/* ARGSUSED */
static int
zfs_pathconf(vnode_t *vp, int cmd, ulong_t *valp, cred_t *cr,
    caller_context_t *ct)
{
	znode_t		*zp;
	zfsvfs_t	*zfsvfs;
	int error;

	switch (cmd) {
	case _PC_LINK_MAX:
		*valp = INT_MAX;
		return (0);

	case _PC_FILESIZEBITS:
		*valp = 64;
		return (0);

	case _PC_XATTR_EXISTS:
		zp = VTOZ(vp);
		zfsvfs = zp->z_zfsvfs;
		if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
			return (error);
		*valp = 0;
		error = zfs_dirent_lookup(VTOZ(vp), "", NULL, ZXATTR);
		if (error == 0)
			*valp = 1;
		else
			error = 0;
		zfs_exit(zfsvfs, FTAG);
		return (error);

	case _PC_SATTR_ENABLED:
	case _PC_SATTR_EXISTS:
		*valp = vfs_has_feature(vp->v_vfsp, VFSFT_SYSATTR_VIEWS) &&
		    (vp->v_type == VREG || vp->v_type == VDIR);
		return (0);

	case _PC_ACCESS_FILTERING:
		*valp = vfs_has_feature(vp->v_vfsp, VFSFT_ACCESS_FILTER) &&
		    vp->v_type == VDIR;
		return (0);

	case _PC_ACL_ENABLED:
		*valp = _ACL_ACE_ENABLED;
		return (0);

	case _PC_MIN_HOLE_SIZE:
		*valp = (ulong_t)SPA_MINBLOCKSIZE;
		return (0);

	case _PC_TIMESTAMP_RESOLUTION:
		/* nanosecond timestamp resolution */
		*valp = 1L;
		return (0);

	default:
		return (fs_pathconf(vp, cmd, valp, cr, ct));
	}
}

/*
 * zfs_write_simple - simple write helper that doesn't use the full
 * vnode machinery.  Used for ZIL replay and internal bookkeeping.
 */
int
zfs_write_simple(znode_t *zp, const void *data, size_t len,
    loff_t pos, size_t *resid)
{
	int error = 0;
	ssize_t n;
	dmu_tx_t *tx;
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;

	tx = dmu_tx_create(zfsvfs->z_os);
	dmu_tx_hold_write(tx, zp->z_id, pos, len);
	error = dmu_tx_assign(tx, DMU_TX_WAIT);
	if (error != 0) {
		dmu_tx_abort(tx);
		if (resid)
			*resid = len;
		return (error);
	}

	dmu_write(zfsvfs->z_os, zp->z_id, pos, len, data, tx, 0);
	dmu_tx_commit(tx);

	if (resid)
		*resid = 0;
	return (0);
}

/*
 * illumos vnode operations template for ZFS.
 *
 * Many of these forward through thin wrappers to the functions declared
 * in zfs_vnops.h / zfs_vnops_os.h that take znode_t* rather than vnode_t*.
 * The stubs for operations not yet implemented simply return ENOTSUP.
 */

/* ARGSUSED */
static int
zfs_setattr_vnop(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr,
    caller_context_t *ct)
{
	return (zfs_setattr(VTOZ(vp), vap, flags, cr, NULL));
}

/* ARGSUSED */
static int
zfs_read_vnop(vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr,
    caller_context_t *ct)
{
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);
	error = zfs_read(zp, (zfs_uio_t *)uio, ioflag, cr);
	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/* ARGSUSED */
static int
zfs_write_vnop(vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr,
    caller_context_t *ct)
{
	znode_t *zp = VTOZ(vp);
	zfsvfs_t *zfsvfs = zp->z_zfsvfs;
	int error;

	if ((error = zfs_enter_verify_zp(zfsvfs, zp, FTAG)) != 0)
		return (error);
	error = zfs_write(zp, (zfs_uio_t *)uio, ioflag, cr);
	zfs_exit(zfsvfs, FTAG);
	return (error);
}

/* ARGSUSED */
static int
zfs_create_vnop(vnode_t *dvp, char *name, vattr_t *vap, vcexcl_t excl,
    int mode, vnode_t **vpp, cred_t *cr, int flag, caller_context_t *ct,
    vsecattr_t *vsecp)
{
	znode_t *zp = NULL;
	int error;

	error = zfs_create(VTOZ(dvp), name, vap, excl, mode, &zp, cr,
	    flag, vsecp, NULL);
	if (error == 0)
		*vpp = ZTOV(zp);
	return (error);
}

/* ARGSUSED */
static int
zfs_remove_vnop(vnode_t *dvp, char *name, cred_t *cr,
    caller_context_t *ct, int flags)
{
	return (zfs_remove(VTOZ(dvp), name, cr, flags));
}

/* ARGSUSED */
static int
zfs_mkdir_vnop(vnode_t *dvp, char *dirname, vattr_t *vap, vnode_t **vpp,
    cred_t *cr, caller_context_t *ct, int flags, vsecattr_t *vsecp)
{
	znode_t *zp = NULL;
	int error;

	error = zfs_mkdir(VTOZ(dvp), dirname, vap, &zp, cr, flags,
	    vsecp, NULL);
	if (error == 0)
		*vpp = ZTOV(zp);
	return (error);
}

/* ARGSUSED */
static int
zfs_rmdir_vnop(vnode_t *dvp, char *name, vnode_t *cwd, cred_t *cr,
    caller_context_t *ct, int flags)
{
	return (zfs_rmdir(VTOZ(dvp), name, cwd ? VTOZ(cwd) : NULL, cr, flags));
}

/* ARGSUSED */
static int
zfs_rename_vnop(vnode_t *sdvp, char *snm, vnode_t *tdvp, char *tnm,
    cred_t *cr, caller_context_t *ct, int flags)
{
	return (zfs_rename(VTOZ(sdvp), snm, VTOZ(tdvp), tnm, cr, flags,
	    0, NULL, NULL));
}

/* ARGSUSED */
static int
zfs_symlink_vnop(vnode_t *dvp, char *name, vattr_t *vap, char *link,
    cred_t *cr, caller_context_t *ct, int flags)
{
	znode_t *zp = NULL;
	int error;

	error = zfs_symlink(VTOZ(dvp), name, vap, link, &zp, cr, flags, NULL);
	if (error == 0)
		VN_RELE(ZTOV(zp));
	return (error);
}

/* ARGSUSED */
static int
zfs_link_vnop(vnode_t *tdvp, vnode_t *svp, char *name, cred_t *cr,
    caller_context_t *ct, int flags)
{
	return (zfs_link(VTOZ(tdvp), VTOZ(svp), name, cr, flags));
}

/* ARGSUSED */
static int
zfs_fsync_vnop(vnode_t *vp, int syncflag, cred_t *cr, caller_context_t *ct)
{
	return (zfs_fsync(VTOZ(vp), syncflag, cr));
}

/* ARGSUSED */
static int
zfs_space_vnop(vnode_t *vp, int cmd, flock64_t *bfp, int flag,
    offset_t offset, cred_t *cr, caller_context_t *ct)
{
	return (zfs_space(VTOZ(vp), cmd, bfp, flag, offset, cr));
}

/* ARGSUSED */
static int
zfs_getsecattr_vnop(vnode_t *vp, vsecattr_t *vsecp, int flag, cred_t *cr,
    caller_context_t *ct)
{
	return (zfs_getsecattr(VTOZ(vp), vsecp, flag, cr));
}

/* ARGSUSED */
static int
zfs_setsecattr_vnop(vnode_t *vp, vsecattr_t *vsecp, int flag, cred_t *cr,
    caller_context_t *ct)
{
	return (zfs_setsecattr(VTOZ(vp), vsecp, flag, cr));
}

/*
 * illumos vnodeops table.
 *
 * The vnodeops structure is constructed at module load time via
 * vn_make_ops() and the template below.  Operations that are not yet
 * implemented fall through to the default (fs_nosys / fs_error) or to a
 * simple stub that returns ENOTSUP.
 */
vnodeops_t *zfs_vnodeops;

const fs_operation_def_t zfs_vnodeops_template[] = {
	VOPNAME_OPEN,		{ .vop_open = zfs_open },
	VOPNAME_CLOSE,		{ .vop_close = zfs_close },
	VOPNAME_READ,		{ .vop_read = zfs_read_vnop },
	VOPNAME_WRITE,		{ .vop_write = zfs_write_vnop },
	VOPNAME_IOCTL,		{ .vop_ioctl = zfs_ioctl },
	VOPNAME_GETATTR,	{ .vop_getattr = zfs_getattr },
	VOPNAME_SETATTR,	{ .vop_setattr = zfs_setattr_vnop },
	VOPNAME_ACCESS,		{ .vop_access = zfs_access_vnop },
	VOPNAME_LOOKUP,		{ .vop_lookup = zfs_lookup },
	VOPNAME_CREATE,		{ .vop_create = zfs_create_vnop },
	VOPNAME_REMOVE,		{ .vop_remove = zfs_remove_vnop },
	VOPNAME_LINK,		{ .vop_link = zfs_link_vnop },
	VOPNAME_RENAME,		{ .vop_rename = zfs_rename_vnop },
	VOPNAME_MKDIR,		{ .vop_mkdir = zfs_mkdir_vnop },
	VOPNAME_RMDIR,		{ .vop_rmdir = zfs_rmdir_vnop },
	VOPNAME_READDIR,	{ .vop_readdir = zfs_readdir },
	VOPNAME_SYMLINK,	{ .vop_symlink = zfs_symlink_vnop },
	VOPNAME_READLINK,	{ .vop_readlink = zfs_readlink },
	VOPNAME_FSYNC,		{ .vop_fsync = zfs_fsync_vnop },
	VOPNAME_INACTIVE,	{ .vop_inactive = zfs_inactive },
	VOPNAME_FID,		{ .vop_fid = zfs_fid },
	VOPNAME_SEEK,		{ .vop_seek = zfs_seek },
	VOPNAME_PATHCONF,	{ .vop_pathconf = zfs_pathconf },
	VOPNAME_GETSECATTR,	{ .vop_getsecattr = zfs_getsecattr_vnop },
	VOPNAME_SETSECATTR,	{ .vop_setsecattr = zfs_setsecattr_vnop },
	VOPNAME_SPACE,		{ .vop_space = zfs_space_vnop },
	VOPNAME_VNEVENT,	{ .vop_vnevent = fs_vnevent_support },
	NULL,			NULL
};
