/**
 * @licence app begin@
 * Copyright (C) 2016  Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * This file is part of GENIVI Project Dlt - Diagnostic Log and Trace monitor apps.
 *
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author Christoph Lipka <clipka@jp.adit-jv.com> ADIT 2016
 *
 * \file Adit-product.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <dlt.h>
#include <dlt_common.h>
#include "../dlt-monitor.h"

DLT_DECLARE_CONTEXT(product_context);

#define ISSUE "/etc/issue"
#define FILENAME "/tmp/version.txt"
#define LINE    1024
#define LEN     128

#define NAME "ADIT"

#define TAG_LEN

typedef struct
{
    char tag[LEN];
    char product[LEN];
    char distro[LEN];
    char hotfix[LEN];
    char arch[LEN];
    char kernel_version[LEN];
} AditProductData;

/**
 * @brief to initiate adit product info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int init(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        return -1;
    }

    DLT_REGISTER_CONTEXT(product_context,
                         collector->ctid,
                         "ADIT Product Data Collector");
    collector->state = COLLECTOR_INITIALISED;
    DLT_LOG(product_context,
            DLT_LOG_VERBOSE,
            DLT_STRING("Initialize"),
            DLT_STRING(collector->name));

    return 0;
}

/**
 * @brief to send adit product info to DLT
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int collect(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    FILE *fp = NULL;
    FILE *file_p = NULL;
    char line[LINE];
    const char del1[2] = ":";
    const char del2[2] = " ";
    const char del3[2] = "\n";
    int len;
    int retval;
    char *str;
    char *str1;
    char *str2;
    char *str3;
    char *token;
    char *token2;
    AditProductData data;

    if (collector == NULL)
    {
        return -1;
    }

    if (collector->state == COLLECTOR_NOT_RUNNING)
    {
        collector->init(collector);
    }
    collector->state = COLLECTOR_RUNNING;
    memset(&data, 0, sizeof(AditProductData));
    DLT_LOG(product_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));

    fp = fopen(ISSUE, "r");

    if (fp == NULL)
    {
        DLT_LOG(product_context,
                DLT_LOG_ERROR,
                DLT_STRING("Failed to open "),
                DLT_STRING(ISSUE));
        return -1;
    }

    /*
     * Relevant lines in /etc/issue are
     * ADIT: inte_r3_e28 mgc-imx6-arm - DISTRI: MGC_20140501 spring-2014-05 - Hotfix:
     * Architecture: mx6q - cortexa9-vfp-neon
     */

    /* handle 1st line */
    memset(&line, 0, LINE);
    if (fgets(line, LINE, fp) != NULL)
    {
        char *save_ptr = NULL;

        str = &line[strlen("ADIT: ")]; /* get rid of 'ADIT: ' */
        token = strtok_r(str, del1, &save_ptr);
        str1 = strdup(token);
        token = strtok_r(NULL, del1, &save_ptr);
        str2 = strdup(token);
        token = strtok_r(NULL, del3, &save_ptr);
        str3 = strdup(token);

        /* get tag and product */
        token2 = strtok_r(str1, del2, &save_ptr);
        strcpy(data.tag, token2);
        token2 = strtok_r(NULL, del2, &save_ptr);
        strcpy(data.product, token2);

        /* get distribution */
        len = strlen(str2) - strlen(" - Hotfix");
        strncpy(data.distro, str2, len);

        /* get hotfix */
        if (strlen(str3) > 3)
        {
            strcpy(data.hotfix, str3);
        }
        else
        {
            strcpy(data.hotfix,"");
        }
    }
    else
    {
        DLT_LOG(product_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot read from "),
                DLT_STRING(ISSUE));
        fclose(fp);
        return -1;
    }

    /* handle 2nd line */
    memset(&line, 0, LINE);
    if (fgets(line, LINE, fp) != NULL)
    {
        str = &line[strlen("Architecture: ")]; /* get rid of 'Architecture: ' */
        token = strtok(str, del3);
        strcpy(data.arch, token);
    }
    else
    {
        DLT_LOG(product_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot read Architecture from "),
                DLT_STRING(ISSUE));
        fclose(fp);
        return -1;
    }
    /* Getting kernel version */
    memset(&line, 0, LINE);
    snprintf(line, LINE, "uname -r > %s", FILENAME);

    retval = system(line); 	/* Directing kernel version into file */
    if (retval == -1)
    {
        DLT_LOG(product_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot read kernel version"));
        fclose(fp);
        return -1;
    }
    else
    {
        file_p = fopen(FILENAME, "r");
        if (!file_p)
        {
            DLT_LOG(product_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("Failed to Open,"),
                    DLT_STRING(strerror(errno)));
            return -1;
        }
        else
        {
            memset(&line, 0, LINE);
            if (fgets(line, LINE, file_p) != NULL)
            {
                token = strtok(line, del3);
                strcpy(data.kernel_version, token);
            }
            else
            {
                DLT_LOG(product_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("Unable to get kernel version from file, abnormal exit status."));
                fclose(file_p);
                return -1;
            }
        }
    }

    /* log to DLT */
    DLT_LOG(product_context,
            DLT_LOG_INFO,
            DLT_STRING(data.tag),
            DLT_STRING(data.product),
            DLT_STRING(data.distro),
            DLT_STRING(data.hotfix),
            DLT_STRING(data.arch);
            DLT_STRING(data.kernel_version));

    free(str1);
    free(str2);
    free(str3);
    fclose(fp);
    fclose(file_p);
    return 0;
}

/**
 * @brief to cleanup adit product info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        return -1;
    }

    DLT_LOG(product_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    collector->state = COLLECTOR_NOT_RUNNING;

    DLT_UNREGISTER_CONTEXT(product_context);

    return 0;
}
