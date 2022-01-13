/*****************************************************************************
-  \file : iap2_usb_gadget_load_modules.c
-  \version : $Id: iap2_usb_gadget_load_modules.c, v Exp $
-  \release : $Name:$
-  Contains the source code implementation for usb role switch
-  \component :
-  \author : Manavalan Veeramani/RBEI/ manavalan.veeramani@in.bosch.com
-  \copyright (c) 2010 - 2013 Advanced Driver Information Technology.
-  This code is developed by Advanced Driver Information Technology.
-  Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
-  All rights reserved.
*****************************************************************************/

/* **********************  includes  ********************** */

#include <libkmod.h>
#include "iap2_usb_gadget_load_modules.h"

#include "iap2_dlt_log.h"

LOCAL S32 iap2InsertModule(struct kmod_module *module, char* options)
{
    S32 rc;

    rc = kmod_module_insert_module(module, KMOD_PROBE_APPLY_BLACKLIST, options);
    if (0 == rc)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_DEBUG, "iap2LoadModule()  loaded module %s\n", kmod_module_get_name(module));
    }
    else if (rc == KMOD_PROBE_APPLY_BLACKLIST)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()  module %s black listed\n", kmod_module_get_name(module));
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()  failed to insert module %s | err = %d \n", kmod_module_get_name(module), rc);
    }

    return rc;
}

LOCAL S32 iap2CreateCtxandModule(struct kmod_ctx **ctx, struct kmod_module **module, const char* modname)
{
    S32 rc = -1;

    if (modname != NULL)
    {
        *ctx = kmod_new(NULL, NULL);
        if (*ctx == NULL)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()  kmod_new() failed\n");
            rc = -1;
        }
        else
        {
            rc = 0;
        }
    }
    if (rc == 0)
    {
        rc = kmod_module_new_from_name(*ctx, modname, module);
        if (rc)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()  kmod_module_new_from_name() failed\n");
            rc = -1;
        }
    }

    return rc;
}

/* load kernel module */
S32 iap2LoadModule(const iap2LoadModuleParameters* iap2ModuleLoadParam, const char* modname, S32 length)
{
    S32 rc = 0;
    struct kmod_ctx *ctx = NULL;
    struct kmod_module *module = NULL;

    rc = iap2CreateCtxandModule(&ctx, &module, modname);
    if (rc == 0)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_DEBUG, "iap2LoadModule()  modname='%s' obj=%p\n", modname, module);

        if ( (strncmp(modname, CONFIGFS_MODULE_NAME, length) == 0)      ||
             (strncmp(modname, LIBCOMPOSITE_MODULE_NAME, length) == 0)  ||
             (strncmp(modname, USB_F_FS_MODULE_NAME, length) == 0)      ||
             (strncmp(modname, U_ETHER_MODULE_NAME, length) == 0)       ||
             (strncmp(modname, USB_F_NCM_MODULE_NAME, length) == 0) )
        {
            if (iap2ModuleLoadParam != NULL)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()   No parameters supported for module %s\n", kmod_module_get_name(module));
                rc = -1;
            }

            if (rc == 0)
            {
                rc = iap2InsertModule(module, NULL);
            }
        }
        else if (strncmp(modname, GADGET_FFS_MODULE_NAME, length) == 0)
        {
            char options[MAX_STRING_LEN];

            if (iap2ModuleLoadParam != NULL)
            {
                memset(&options[0], 0, MAX_STRING_LEN);
                snprintf(&options[0], MAX_STRING_LEN-1, "idVendor=%s idProduct=%s iManufacturer=%s iProduct=%s iSerialNumber=%s bcdDevice=%s qmult=%s",
                        iap2ModuleLoadParam->gadget_fs.vendorId, iap2ModuleLoadParam->gadget_fs.productId, iap2ModuleLoadParam->gadget_fs.manufacturer,
                        iap2ModuleLoadParam->gadget_fs.name, iap2ModuleLoadParam->gadget_fs.serial, iap2ModuleLoadParam->gadget_fs.bcdDevice,
                        iap2ModuleLoadParam->gadget_fs.qmult);

                rc = iap2InsertModule(module, &options[0]);
            }
            else
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()  No parameters given for module %s\n", kmod_module_get_name(module));
                rc = -1;
            }
        }
        else
        {
            /* error. unknown module name */
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2LoadModule()  unknown module name\n");
            rc = -1;
        }
    }
    if (module != NULL)
    {
        kmod_module_unref(module);
    }
    if (ctx != NULL)
    {
        kmod_unref(ctx);
    }

    return rc;
}

/* unload kernel module */
S32 iap2UnloadModule(const char* modname)
{
    int rc = 0;
    struct kmod_ctx *ctx = NULL;
    struct kmod_module *module = NULL;

    rc = iap2CreateCtxandModule(&ctx, &module, modname);
    if(rc == 0)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_DEBUG, "iap2UnloadModule()  modname='%s' obj=%p\n", modname, module);

        rc = kmod_module_remove_module(module, KMOD_PROBE_APPLY_BLACKLIST);
        if (0 == rc)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_DEBUG, "iap2UnloadModule()  unloaded module %s\n", kmod_module_get_name(module));
        }
        else if (rc == KMOD_PROBE_APPLY_BLACKLIST)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2UnloadModule()  module %s black listed\n", kmod_module_get_name(module));
        }
        else
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "iap2UnloadModule()  failed to remove module %s | err = %d \n", kmod_module_get_name(module), rc);
        }
    }
    if(module != NULL)
    {
        kmod_module_unref(module);
    }
    if(ctx != NULL)
    {
        kmod_unref(ctx);
    }

    return rc;
}
