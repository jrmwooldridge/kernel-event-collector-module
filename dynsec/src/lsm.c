// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 VMware, Inc. All rights reserved.

// Adapted from kernel-event-collector-module

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)  //{
#include <linux/lsm_hooks.h>  // security_hook_heads
#endif  //}
#include <linux/rculist.h>  // hlist_add_tail_rcu
#include "symbols.h"
#include "lsm_mask.h"
#include "dynsec.h"
#include "hooks.h"

// checkpatch-ignore: AVOID_EXTERNS
#define DEBUGGING_SANITY 0
#if DEBUGGING_SANITY  //{ WARNING from checkpatch
#define PR_p "%px"
#else  //}{ checkpatch no WARNING
#define PR_p "%p"
#endif  //}


// Event Type To LSM Hook Names
#define DYNSEC_LSM_inode_rename         DYNSEC_HOOK_TYPE_RENAME
#define DYNSEC_LSM_inode_unlink         DYNSEC_HOOK_TYPE_UNLINK
#define DYNSEC_LSM_inode_rmdir          DYNSEC_HOOK_TYPE_RMDIR
#define DYNSEC_LSM_inode_mdkir          DYNSEC_HOOK_TYPE_MKDIR
#define DYNSEC_LSM_inode_create         DYNSEC_HOOK_TYPE_CREATE
#define DYNSEC_LSM_inode_setattr        DYNSEC_HOOK_TYPE_SETATTR
#define DYNSEC_LSM_inode_link           DYNSEC_HOOK_TYPE_LINK
#define DYNSEC_LSM_inode_symlink        DYNSEC_HOOK_TYPE_SYMLINK

// may need another hook
#define DYNSEC_LSM_bprm_set_creds       DYNSEC_HOOK_TYPE_EXEC

#define DYNSEC_LSM_task_kill            DYNSEC_HOOK_TYPE_SIGNAL
// depends on kver
#define DYNSEC_LSM_dentry_open          DYNSEC_HOOK_TYPE_OPEN
#define DYNSEC_LSM_file_open            DYNSEC_HOOK_TYPE_OPEN

// Ptrace hook type maps to two hooks
#define DYNSEC_LSM_ptrace_traceme       DYNSEC_HOOK_TYPE_PTRACE
#define DYNSEC_LSM_ptrace_access_check  DYNSEC_HOOK_TYPE_PTRACE


static bool g_lsmRegistered;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{ not RHEL8
struct        security_operations  *g_original_ops_ptr;   // Any LSM which we are layered on top of
static struct security_operations   g_combined_ops;       // Original LSM plus our hooks combined
#endif //}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)  //{
static unsigned int cblsm_hooks_count;
static struct security_hook_list cblsm_hooks[64];  // [0..39] not needed?
#endif  //}

struct lsm_symbols {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    struct security_operations **security_ops;
#else
    struct security_hook_heads *security_hook_heads;
#endif
};

static struct lsm_symbols lsm_syms;
static struct lsm_symbols *p_lsm;

bool dynsec_init_lsmhooks(uint64_t enableHooks)
{
    p_lsm = NULL;
    g_lsmRegistered = false;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    g_original_ops_ptr = NULL;
#endif

    // Add check when implementing LSM hooks
    BUILD_BUG_ON(DYNSEC_LSM_bprm_set_creds != DYNSEC_HOOK_TYPE_EXEC);
    BUILD_BUG_ON(DYNSEC_LSM_inode_rename   != DYNSEC_HOOK_TYPE_RENAME);
    BUILD_BUG_ON(DYNSEC_LSM_inode_unlink   != DYNSEC_HOOK_TYPE_UNLINK);
    BUILD_BUG_ON(DYNSEC_LSM_inode_rmdir    != DYNSEC_HOOK_TYPE_RMDIR);

    memset(&lsm_syms, 0, sizeof(lsm_syms));

#define find_lsm_sym(sym_name, ops) \
    find_symbol_indirect(#sym_name, (unsigned long *)&ops.sym_name);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    find_lsm_sym(security_ops, lsm_syms);
    if (!lsm_syms.security_ops)
    {
        goto out_fail;
    }
#else
    find_lsm_sym(security_hook_heads, lsm_syms);
    if (!lsm_syms.security_hook_heads)
    {
        goto out_fail;
    }
#endif
    p_lsm = &lsm_syms;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{
    //
    // Save off the old LSM pointers
    //
    g_original_ops_ptr = *p_lsm->security_ops;
    if (g_original_ops_ptr != NULL)
    {
        g_combined_ops = *g_original_ops_ptr;
    }
    pr_info("Other LSM named %s", g_original_ops_ptr->name);

    #define CB_LSM_SETUP_HOOK(NAME) do { \
        if (enableHooks & DYNSEC_LSM_##NAME) \
            g_combined_ops.NAME = dynsec_##NAME; \
    } while (0)

#else  // }{ LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
    cblsm_hooks_count = 0;
    memset(cblsm_hooks, 0, sizeof(cblsm_hooks));

    #define CB_LSM_SETUP_HOOK(NAME) do { \
        if (p_lsm->security_hook_heads && enableHooks & DYNSEC_LSM_##NAME) { \
            pr_info("Hooking %u@" PR_p " %s\n", cblsm_hooks_count, &p_lsm->security_hook_heads->NAME, #NAME); \
            cblsm_hooks[cblsm_hooks_count].head = &p_lsm->security_hook_heads->NAME; \
            cblsm_hooks[cblsm_hooks_count].hook.NAME = dynsec_##NAME; \
            cblsm_hooks[cblsm_hooks_count].lsm = "dynsec"; \
            cblsm_hooks_count++; \
        } \
    } while (0)
#endif  // }

    //
    // Now add our hooks
    //
    CB_LSM_SETUP_HOOK(bprm_set_creds); // process banning  (exec)
    CB_LSM_SETUP_HOOK(inode_unlink);   // security_inode_unlink
    CB_LSM_SETUP_HOOK(inode_rmdir);   // security_inode_rmdir
    CB_LSM_SETUP_HOOK(inode_rename);   // security_inode_rename

#undef CB_LSM_SETUP_HOOK



#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{
    *(p_lsm->security_ops) = &g_combined_ops;
#else  //}{
    {
        unsigned int j;

        for (j = 0; j < cblsm_hooks_count; ++j) {
            cblsm_hooks[j].lsm = "dynsec";
            hlist_add_tail_rcu(&cblsm_hooks[j].list, cblsm_hooks[j].head);
        }
    }
#endif  //}

    g_lsmRegistered = true;
    return true;

out_fail:
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{
    pr_info("LSM: Failed to find security_ops\n");
#else  //}{
    pr_info("LSM: Failed to find security_hook_heads\n");
#endif  //}
    return false;
}

// KERNEL_VERSION(4,0,0) and above say this is none of our business
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{
bool ec_do_lsm_hooks_changed(uint64_t enableHooks)
{
    bool changed = false;

    // TODO: Implement this if we need check for changed LSM hooks

    return changed;
}
#endif  //}

void dynsec_lsm_shutdown(void)
{
    if (g_lsmRegistered
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{
    &&      p_lsm->security_ops
#endif  //}
    )
    {
        pr_info("Unregistering ec_LSM...");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)  //{
        *(p_lsm->security_ops) = g_original_ops_ptr;
#else  // }{ >= KERNEL_VERSION(4,0,0)
        security_delete_hooks(cblsm_hooks, cblsm_hooks_count);
#endif  //}
    } else
    {
        pr_info("ec_LSM not registered so not unregistering");
    }
}