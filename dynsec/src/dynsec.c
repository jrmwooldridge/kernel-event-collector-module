// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2021 VMware, Inc. All rights reserved.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "lsm_mask.h"
#include "version.h"
#include "symbols.h"

#include "stall_reqs.h"
#include "logging.h"
#include "path_utils.h"

#define DYNSEC_LSM_HOOKS (\
        DYNSEC_HOOK_TYPE_EXEC      |\
        DYNSEC_HOOK_TYPE_UNLINK    |\
        DYNSEC_HOOK_TYPE_RMDIR     |\
        DYNSEC_HOOK_TYPE_RENAME    |\
        DYNSEC_HOOK_TYPE_SETATTR   |\
        DYNSEC_HOOK_TYPE_CREATE    |\
        DYNSEC_HOOK_TYPE_MKDIR     |\
        DYNSEC_HOOK_TYPE_OPEN      |\
        DYNSEC_HOOK_TYPE_CLOSE)

uint64_t lsm_hooks_mask = DYNSEC_LSM_HOOKS;
uint64_t lsm_hooks_enabled = DYNSEC_LSM_HOOKS &
    ~(DYNSEC_HOOK_TYPE_OPEN |
      DYNSEC_HOOK_TYPE_CLOSE);

static char lsm_hooks_str[64];
static char lsm_hooks_disable[64];
// Hooks to only allow for kmod instance. Superset.
module_param_string(lsm_hooks, lsm_hooks_str,
                    sizeof(lsm_hooks_str), 0644);
// Hooks to allow later on but disable on load of kmod.
// Subset of lsm_hooks_mask
module_param_string(lsm_hooks_disable, lsm_hooks_disable,
                    sizeof(lsm_hooks_disable), 0644);

static void setup_lsm_hooks(void)
{
    int strto_ret;

    // Set hooks kmod instance may allow.
    if (lsm_hooks_str[0])
    {
        uint64_t local_lsm_hooks = 0;

        lsm_hooks_str[sizeof(lsm_hooks_str) - 1] = 0;
        strto_ret = kstrtoull(lsm_hooks_str, 16, &local_lsm_hooks);
        if (!strto_ret)
        {
            lsm_hooks_mask = local_lsm_hooks;
            lsm_hooks_enabled = lsm_hooks_mask;
        }
    }

    // Hook to disable on load but may allow later.
    if (lsm_hooks_disable[0])
    {
        uint64_t local_lsm_hooks = 0;

        lsm_hooks_disable[sizeof(lsm_hooks_disable) - 1] = 0;
        strto_ret = kstrtoull(lsm_hooks_disable, 16, &local_lsm_hooks);
        if (!strto_ret)
        {
            lsm_hooks_enabled &= ~(local_lsm_hooks);
        }
    }
}

static int __init dynsec_init(void)
{
    DS_LOG(DS_INFO, "Initializing Dynamic Security Module Brand(%s)",
           CB_APP_MODULE_NAME);

    setup_lsm_hooks();

    if (!dynsec_sym_init()) {
        return -EINVAL;
    }

    if (!dynsec_path_utils_init()) {
        return -EINVAL;
    }

    if (!dynsec_init_lsmhooks(lsm_hooks_mask)) {
        return -EINVAL;
    }

    if (!dynsec_chrdev_init()) {
        dynsec_lsm_shutdown();
        return -EINVAL;
    }

    return 0;
}

static void __exit dynsec_exit(void)
{
    DS_LOG(DS_INFO, "Exiting Dynamic Security Module Brand(%s)",
           CB_APP_MODULE_NAME);

    dynsec_chrdev_shutdown();

    dynsec_lsm_shutdown();
}

module_init(dynsec_init);
module_exit(dynsec_exit);

MODULE_LICENSE("GPL");
