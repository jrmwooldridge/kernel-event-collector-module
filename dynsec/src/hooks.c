// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 VMware, Inc. All rights reserved.
#include <linux/binfmts.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include "dynsec.h"
#include "factory.h"
#include "stall_tbl.h"
#include "stall_reqs.h"
#include "lsm_mask.h"

int dynsec_bprm_set_creds(struct linux_binprm *bprm)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    int response = 0;
    int rc;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->bprm_set_creds(bprm);
        if (ret) {
            goto out;
        }
    }
#endif
    if (!bprm || !bprm->file) {
        goto out;
    }

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_EXEC, DYNSEC_HOOK_TYPE_EXEC,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }
    if (fill_in_bprm_set_creds(dynsec_event_to_exec(event), bprm,
                               GFP_KERNEL)) {
        if (report_flags & DYNSEC_REPORT_STALL) {
            rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

            if (!rc) {
                ret = response;
            }
        } else {
            u32 size = enqueue_nonstall_event(stall_tbl, event);

            if (!size) {
                free_dynsec_event(event);
            }
        }
    } else {
        free_dynsec_event(event);
    }

out:

    return ret;
}

int dynsec_inode_unlink(struct inode *dir, struct dentry *dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    int response = 0;
    int rc;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    umode_t mode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_unlink(dir, dentry);
        if (ret) {
            goto out;
        }
    }
#endif

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    // Only care about certain types of files
    if (!dentry->d_inode) {
        goto out;
    }
    mode = dentry->d_inode->i_mode;
    if (!(S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))) {
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_UNLINK, DYNSEC_HOOK_TYPE_UNLINK,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }

    if (fill_in_inode_unlink(dynsec_event_to_unlink(event), dir, dentry,
                               GFP_KERNEL)) {
        if (report_flags & DYNSEC_REPORT_STALL) {
            rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

            if (!rc) {
                ret = response;
            }
        } else {
            u32 size = enqueue_nonstall_event(stall_tbl, event);

            if (!size) {
                free_dynsec_event(event);
            }
        }
    } else {
        free_dynsec_event(event);
    }

out:

    return ret;
}

int dynsec_inode_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    int response = 0;
    int rc;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    umode_t mode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_rmdir(dir, dentry);
        if (ret) {
            goto out;
        }
    }
#endif

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    // Only care about certain types of files
    if (!dentry->d_inode) {
        goto out;
    }
    mode = dentry->d_inode->i_mode;
    if (!(S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))) {
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_RMDIR, DYNSEC_HOOK_TYPE_RMDIR,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }

    if (fill_in_inode_unlink(dynsec_event_to_unlink(event), dir, dentry,
                               GFP_KERNEL)) {
        if (report_flags & DYNSEC_REPORT_STALL) {
            rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

            if (!rc) {
                ret = response;
            }
        } else {
            u32 size = enqueue_nonstall_event(stall_tbl, event);

            if (!size) {
                free_dynsec_event(event);
            }
        }
    } else {
        free_dynsec_event(event);
    }

out:

    return ret;
}

int dynsec_inode_rename(struct inode *old_dir, struct dentry *old_dentry,
                        struct inode *new_dir, struct dentry *new_dentry)
{
    struct dynsec_event *event = NULL;
    int ret = 0;
    int response = 0;
    int rc;
    uint16_t report_flags = DYNSEC_REPORT_AUDIT;
    umode_t mode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        ret = g_original_ops_ptr->inode_rename(old_dir, old_dentry,
                                               new_dir, new_dentry);
        if (ret) {
            goto out;
        }
    }
#endif

    if (!stall_tbl_enabled(stall_tbl)) {
        goto out;
    }
    if (task_in_connected_tgid(current)) {
        report_flags |= DYNSEC_REPORT_SELF;
    } else {
        report_flags |= DYNSEC_REPORT_STALL;
    }

    if (!old_dentry->d_inode) {
        goto out;
    }
    mode = old_dentry->d_inode->i_mode;
    if (!(S_ISLNK(mode) || S_ISREG(mode) || S_ISDIR(mode))) {
        goto out;
    }

    event = alloc_dynsec_event(DYNSEC_EVENT_TYPE_RENAME, DYNSEC_HOOK_TYPE_RENAME,
                               report_flags, GFP_KERNEL);
    if (!event) {
        goto out;
    }

    if (fill_in_inode_rename(dynsec_event_to_rename(event),
                             old_dir, old_dentry,
                             new_dir, new_dentry,
                             GFP_KERNEL)) {
        if (report_flags & DYNSEC_REPORT_STALL) {
            rc = dynsec_wait_event_timeout(event, &response, 1000, GFP_KERNEL);

            if (!rc) {
                ret = response;
            }
        } else {
            u32 size = enqueue_nonstall_event(stall_tbl, event);

            if (!size) {
                free_dynsec_event(event);
            }
        }
    } else {
        free_dynsec_event(event);
    }

out:

    return ret;
}

int dynsec_inode_setattr(struct dentry *dentry, struct iattr *attr)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->inode_setattr(dentry, attr);
    }
#endif

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_inode_mkdir(struct inode *dir, struct dentry *dentry,
                              umode_t mode)
#else
int dynsec_inode_mkdir(struct inode *dir, struct dentry *dentry,
                              int mode)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->inode_mkdir(dir, dentry, mode);
    }
#endif

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_inode_create(struct inode *dir, struct dentry *dentry,
                        umode_t mode)
#else
int dynsec_inode_create(struct inode *dir, struct dentry *dentry,
                        int mode)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->inode_create(dir, dentry, mode);
    }
#endif

    return 0;
}

int dynsec_inode_link(struct dentry *old_dentry, struct inode *dir,
                      struct dentry *new_dentry)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->inode_link(old_dentry, dir, new_dentry);
    }
#endif

    return 0;
}

int dynsec_inode_symlink(struct inode *dir, struct dentry *dentry,
                const char *old_name)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->inode_symlink(dir, dentry, old_name);
    }
#endif

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
int dynsec_file_open(struct file *file)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
int dynsec_file_open(struct file *file, const struct cred *cred)
#else
int dynsec_dentry_open(struct file *file, const struct cred *cred)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->dentry_open(file, cred);
    }
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->file_open(file, cred);
    }
#endif

    return 0;
}

// Cannot Stall - Enable only for open events
void dynsec_file_free(struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        g_original_ops_ptr->file_free_security(file);
    }
#endif
}

int dynsec_ptrace_traceme(struct task_struct *parent)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->ptrace_traceme(parent);
    }
#endif

    return 0;
}

int dynsec_ptrace_access_check(struct task_struct *child, unsigned int mode)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->ptrace_access_check(child, mode);
    }
#endif

    return 0;
}

// Cannot Stall
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#if RHEL_MAJOR == 8 && RHEL_MINOR == 0
int dynsec_task_kill(struct task_struct *p, struct siginfo *info,
                     int sig, const struct cred *cred)
#else
int dynsec_task_kill(struct task_struct *p, struct kernel_siginfo *info,
                     int sig, const struct cred *cred)
#endif
#else
int dynsec_task_kill(struct task_struct *p, struct siginfo *info,
                     int sig, u32 secid)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
    if (g_original_ops_ptr) {
        return g_original_ops_ptr->task_kill(p, info, sig, secid);
    }
#endif

    return 0;
}

// #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
// int dynsec_mmap_file(struct file *file, unsigned long reqprot, unsigned long prot,
//                      unsigned long flags)
// #else
// int dynsec_file_mmap(struct file *file, unsigned long reqprot, unsigned long prot,
//                      unsigned long flags, unsigned long addr, unsigned long addr_only)
// #endif
// {
// #if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
//     if (g_original_ops_ptr) {
//         return g_original_ops_ptr->file_mmap(file, reqprot, prot, flags, addr, addr_only);
//     }
// #elif LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
//     if (g_original_ops_ptr) {
//         return g_original_ops_ptr->mmap_file(file, reqprot, prot, flags);
//     }
// #endif

//     return 0;
// }

// int dynsec_task_fix_setuid(struct cred *new, const struct cred *old, int flags)
// {
// #if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
//     if (g_original_ops_ptr) {
//         return g_original_ops_ptr->task_fix_setuid(new, old, flags);
//     }
// #endif

//     return 0;
// }
