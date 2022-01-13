/*
 * @licence app begin@
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * This file is part of GENIVI Project DLT - Diagnostic Log and Trace.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*!
 * \author
 * Christoph Lipka <clipka@jp.adit-jv.com>
 *
 * \copyright Copyright © 2016 Advanced Driver Information Technology. \n
 * License MPL-2.0: Mozilla Public License version 2.0 http://mozilla.org/MPL/2.0/.
 *
 * \file dlt_daemon_filter.c
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "dlt_common.h"
#include "dlt-daemon.h"
#include "dlt_protocol.h"
#include "dlt-daemon_cfg.h"
#include "dlt_daemon_event_handler.h"
#include "dlt_daemon_connection.h"
#include "dlt_config_file_parser.h"
#include "dlt_daemon_filter.h"
#include "dlt_daemon_filter_backend.h"

#define MOST_CLOSED_FILTER_NAME "Customer"

/* General filter configuration options */
#define GENERAL_BASE_NAME "General"
typedef struct
{
    char *name;
    int (*handler)(DltMessageFilter *mf, char *val);
    int is_opt;
} general_opts;

/* Filter section options */
#define FILTER_BASE_NAME "Filter"
typedef struct
{
    char *name;
    int (*handler)(DltMessageFilter *mf, DltFilterConfiguration *config, char *value);
} filter_opts;

/* Injection section options */
#define INJECTION_BASE_NAME "Injection"
typedef struct
{
    char *name;
    int (*handler)(DltMessageFilter *mf, DltInjectionConfig *config, char *value);
} injection_opts;

/**
 * @brief Set Control Message bit mask to 1
 *
 * @param flags DltServiceIdFlags
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int enable_all(DltServiceIdFlag *flags)
{
    if (flags == NULL)
    {
        dlt_vlog(LOG_ERR, "%s: invalid arguments\n", __FUNCTION__);
        return -1;
    }

    memset(flags, 0xff, sizeof(DltServiceIdFlag));

    return 0;
}

/**
 * @brief Init Control Message bit mask
 *
 * @param flags DltServiceIdFlags
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int init_flags(DltServiceIdFlag *flags)
{
    if (flags == NULL)
    {
        dlt_vlog(LOG_ERR, "%s: invalid arguments\n", __FUNCTION__);
        return -1;
    }

    memset(flags, 0, sizeof(DltServiceIdFlag));

    return 0;
}

/**
 * @brief Set bit in Control Message bit mask
 *
 * @param flags DltServiceIdFlags
 * @param id    set bit for given id
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int set_bit(DltServiceIdFlag *flags, int id)
{
    int is_upper;
    int byte_pos;
    int bit_pos;
    int tmp;

    if (flags == NULL)
    {
        dlt_vlog(LOG_ERR, "%s: invalid arguments\n", __FUNCTION__);
        return -1;
    }

    if ((id <= DLT_SERVICE_ID) || (id >= DLT_USER_SERVICE_ID_LAST_ENTRY) ||
       (id >= DLT_SERVICE_ID_LAST_ENTRY && id <= DLT_USER_SERVICE_ID))
    {
        dlt_vlog(LOG_WARNING, "Given ID = %d is invalid\n", id);
        return -1;
    }

    is_upper = id & DLT_USER_SERVICE_ID;
    tmp = id & 0xFF;

    byte_pos = tmp >> 3; //  tmp / 8;
    bit_pos = tmp & 7; // tmp % 8;

    if (is_upper)
    {
        SET_BIT(flags->upper[byte_pos], bit_pos);
    }
    else
    {
        SET_BIT(flags->lower[byte_pos], bit_pos);
    }

    return 0;
}

/**
 * @brief Get bit in Control Message bit mask
 *
 * @param flags DltServiceIdFlags
 * @param id    set bit for given id
 * @return 0,1 on success, -1 otherwise
 */
DLT_STATIC int bit(DltServiceIdFlag *flags, int id)
{
    int is_upper;
    int byte_pos;
    int bit_pos;
    int tmp;

    if (flags == NULL)
    {
        dlt_vlog(LOG_ERR, "%s: invalid arguments\n", __FUNCTION__);
        return -1;
    }

    if ((id <= DLT_SERVICE_ID) || (id >= DLT_USER_SERVICE_ID_LAST_ENTRY) ||
       (id >= DLT_SERVICE_ID_LAST_ENTRY && id <= DLT_USER_SERVICE_ID))
    {
        dlt_vlog(LOG_WARNING, "Given ID = %d is invalid\n", id);
        return -1;
    }

    is_upper = id & DLT_USER_SERVICE_ID;
    tmp = id & 0xFF;

    byte_pos = tmp >> 3; //  tmp / 8;
    bit_pos = tmp & 7; // tmp % 8;

    if (is_upper)
    {
        return BIT(flags->upper[byte_pos], bit_pos);
    }
    else
    {
        return BIT(flags->lower[byte_pos], bit_pos);
    }
}

/**
 * @brief Check if the filter configuration name is unique
 *
 * In case the name is not unique, print a warning to the user
 *
 * @param mf        MessageFilter pointer
 * @param config    DltFilterConfiguration pointer
 * @param value     Value given in configuration file
 *
 * @return DLT_RETURN_OK on success, DLT error code otherwise
 */
DLT_STATIC int dlt_daemon_filter_name(DltMessageFilter *mf,
                                   DltFilterConfiguration *config,
                                   char *value)
{
    DltFilterConfiguration *conf;

    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        dlt_vlog(LOG_ERR,
                 "Cannot check section name. Invalid parameter in %s\n",
                 __func__);
        return DLT_RETURN_WRONG_PARAMETER;
    }

    conf = mf->configs;

    while (conf)
    {
        if (conf->name != NULL)
        {
            if (strncmp(conf->name, value, strlen(value)) == 0)
            {
                dlt_vlog(LOG_WARNING, "Section name '%s' already in use\n", value);
                return DLT_RETURN_ERROR;
            }
        }
        conf = conf->next;
    }

    config->name = strdup(value);
    if (config->name == NULL)
    {
        dlt_log(LOG_ERR, "Cannot duplicate string\n");
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

/**
 * @brief Convert string with filter level information to a number
 *
 * Convert a filter level string from configuration file to a valid filter
 * level.
 *
 * @param mf        MessageFilter pointer
 * @param config    DltFilterConfiguration pointer
 * @param value     Value given in configuration file
 *
 * @return DLT_RETURN_OK on success, DLT error code otherwise
 */
DLT_STATIC int dlt_daemon_filter_level(DltMessageFilter *mf,
                                   DltFilterConfiguration *config,
                                   char *value)
{
    char *max_ptr = NULL;
    unsigned long int max;

    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        dlt_vlog(LOG_ERR,
                 "Cannot check section name. Invalid parameter in %s\n",
                 __func__);
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* level should be the upper value of the range */
    max = strtoul(value, &max_ptr, 10);

    if (max_ptr == value)
    {
        dlt_vlog(LOG_WARNING, "Level %s is not a number\n", value);
        return DLT_RETURN_ERROR;
    }

    if (max > DLT_FILTER_LEVEL_MAX)
    {
        dlt_vlog(LOG_WARNING, "Level %d is invalid\n", max);
        return DLT_RETURN_ERROR;
    }

    /* set level (level_min will be set afterwards) */
    config->level_max = (unsigned int)max;

    return DLT_RETURN_OK;
}

/**
 * @brief Set the control message mask for a filter configuration
 *
 * Use the string given in the configuration file for that filter configuration
 * to create a mask of allowed control messages.
 *
 * @param mf        MessageFilter pointer
 * @param config    DltFilterConfiguration pointer
 * @param value     Value given in configuration file
 *
 * @return DLT_RETURN_OK on success, DLT error code otherwise
 */
DLT_STATIC int dlt_daemon_filter_control_mask(DltMessageFilter *mf,
                                          DltFilterConfiguration *config,
                                          char *value)
{
    char *token = NULL;
    char *save_ptr;

    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        dlt_vlog(LOG_ERR,
                 "Cannot check section name. Invalid parameter in %s\n",
                 __func__);
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* initialize mask */
    init_flags(&config->ctrl_mask);

    /* check wildcard */
    if (value[0] == '*')
    {
        enable_all(&config->ctrl_mask);
        return DLT_RETURN_OK;
    }

    /* check for no client specifier */
    if (strncasecmp(value, DLT_FILTER_CLIENT_NONE, strlen(value)) == 0)
    {
        return DLT_RETURN_OK;
    }

    /* list of allowed control messages given */
    token = strtok_r(value, ",", &save_ptr);

    while (token != NULL)
    {
        int base = 16;
        long id = strtol(token, NULL, base);
        if (id <= DLT_SERVICE_ID || id >= DLT_USER_SERVICE_ID_LAST_ENTRY ||
            (id >= DLT_SERVICE_ID_LAST_ENTRY && id <= DLT_USER_SERVICE_ID ))
        {
            dlt_vlog(LOG_WARNING,
                     "Ignore invalid service ID: %s\n",
                     token);
        }
        else
        {
            set_bit(&config->ctrl_mask, id);
        }
        token = strtok_r(NULL, ",", &save_ptr);
    }

    return 0;
}

/**
 * @brief Set the client (connection) mask for a filter configuration
 *
 * Get here a list of strings from the configuration file and create a mask out
 * of it.
 * E.g Serial, TCP will become a mask for connection defined in
 * dlt_daemon_connection_types.h:
 * (DLT_CON_MASK_CLIENT_MSG_SERIAL | DLT_CON_MASK_CLIENT_MSG_TCP)
 *
 * @param mf        MessageFilter pointer
 * @param config    DltFilterConfiguration pointer
 * @param value     Value given in configuration file
 *
 * @return DLT_RETURN_OK on success, DLT error code otherwise
 */
DLT_STATIC int dlt_daemon_filter_client_mask(DltMessageFilter *mf,
                                         DltFilterConfiguration *config,
                                         char *value)
{
    char *token = NULL;
    char *rest = NULL;

    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        dlt_vlog(LOG_ERR,
                 "Cannot check section name. Invalid parameter in %s\n",
                 __func__);
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* initialize mask */
    config->client_mask = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;

    /* check wildcard */
    if (value[0] == '*')
    {
        config->client_mask = DLT_CON_MASK_ALL;
        return DLT_RETURN_OK;
    }

    /* check for no client specifier */
    if (strncasecmp(value, DLT_FILTER_CLIENT_NONE, strlen(value)) == 0)
    {
        /* return default mask */
        return DLT_RETURN_OK;
    }

    /* list of allowed clients given */
    token = strtok_r(value, ",", &rest);
    while (token != NULL)
    {
        if (strncasecmp(token, "Serial", strlen(token)) == 0)
        {
            config->client_mask |= DLT_CON_MASK_CLIENT_MSG_SERIAL;
        }
        else if (strncasecmp(token, "TCP", strlen(token)) == 0)
        {
            config->client_mask |= DLT_CON_MASK_CLIENT_CONNECT;
            config->client_mask |= DLT_CON_MASK_CLIENT_MSG_TCP;
        }
        else if (strncasecmp(token, "Logstorage", strlen(token)) == 0)
        {
            config->client_mask |= DLT_CON_MASK_CLIENT_MSG_OFFLINE_LOGSTORAGE;
        }
        else if (strncasecmp(token, "Trace", strlen(token)) == 0)
        {
            config->client_mask |= DLT_CON_MASK_CLIENT_MSG_OFFLINE_TRACE;
        }
        else
        {
            dlt_vlog(LOG_INFO, "Ignoring unknown client type: %s\n", token);
        }

        token = strtok_r(NULL, ",", &rest);
    }

    return 0;
}

/**
 * @brief Set injections per filter
 *
 * @param mf        MessageFilter pointer
 * @param config    DltFilterConfiguration pointer
 * @param value     Value given in configuration file
 *
 * @return DLT_RETURN_OK on success, DLT error code otherwise
 */
DLT_STATIC int dlt_daemon_filter_injections(DltMessageFilter *mf,
                                        DltFilterConfiguration *config,
                                        char *value)
{
    int i;
    char *token = NULL;
    char *save_ptr;

    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        dlt_vlog(LOG_ERR,
                 "Cannot check section name. Invalid parameter in %s\n",
                 __func__);
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /*value is a komma separated list of injections or '*' or NONE */
    if (strncmp(value, "*", strlen("*")) == 0)
    {
        config->num_injections = -1;
    }
    else if(strncasecmp(value, DLT_FILTER_CLIENT_NONE, strlen(value)) == 0)
    {
        config->num_injections = 0;
    }
    else /* at least one specific injection is given */
    {
        config->num_injections = 1;
        /* count numbers of commata to get number of */
        for(i = 0; value[i]; i++)
        {
            if (value[i] == ',')
            {
                config->num_injections += 1;
            }
        }

        config->injections = calloc(config->num_injections, sizeof(char *));
        if (config->injections == NULL)
        {
            dlt_log(LOG_CRIT,
                    "Memory allocation for injection configuration failed!\n");
            return -1;
        }
        i = 0;

        token = strtok_r(value, ",", &save_ptr);
        while (token != NULL)
        {
            config->injections[i] = strdup(token);
            i++;
            token = strtok_r(NULL, ",", &save_ptr);
        }
    }

    return 0;
}

DLT_STATIC filter_opts filter[] =
{
    {.name = "Name", .handler = dlt_daemon_filter_name},
    {.name = "Level", .handler = dlt_daemon_filter_level},
    {.name = "Clients", .handler = dlt_daemon_filter_client_mask},
    {.name = "ControlMessages", .handler = dlt_daemon_filter_control_mask},
    {.name = "Injections", .handler = dlt_daemon_filter_injections},
    {NULL, NULL}
};

/**
 * @brief insert filter configuration
 *
 * @param configs      Filter Configuration
 * @param next_configs Next Filter Configuration
 * @param conf         Filter Configuration which has to insert
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_add_filter_configuration(DltFilterConfiguration **configs,
                                 DltFilterConfiguration **next_configs,
                                 DltFilterConfiguration **conf)
{
    if ((configs == NULL) || (next_configs == NULL))
    {
        return DLT_RETURN_ERROR;
    }

    /* if the same level is given for two definitions, update to latest filter */
    if ((*configs)->level_max == (*conf)->level_max)
    {
        dlt_vlog(LOG_WARNING,
                "The same level is given for %s and %s. Update to latest filter(%s).\n",
                (*configs)->name,
                (*conf)->name,
                (*conf)->name);
        (*conf)->level_min = (*configs)->level_min;
        (*conf)->next = (*configs)->next;
        *configs = *conf;
        return DLT_RETURN_OK;
    }

    /* insert conf into configs */
    if (((*next_configs)->level_min <= (*conf)->level_max) &&
        ((*next_configs)->level_max > (*conf)->level_max))
    {
        /* set level_min */
        (*conf)->level_min = (*next_configs)->level_min;
        (*next_configs)->level_min = (*conf)->level_max + 1;
        (*conf)->next = *next_configs;
        *next_configs = *conf;
        return DLT_RETURN_OK;
    }

    return DLT_RETURN_ERROR;
}

/**
 * @brief Setup a filter configuration
 *
 * Use the information of a filter configuration section in the configuration
 * file to setup a filter configuration
 *
 * @param mf        Message filter
 * @param config    Config file handle
 * @param sec_name  Name of the section to setup this filter configuration
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_setup_filter_section(DltMessageFilter *mf,
                                           DltConfigFile *config,
                                           char *sec_name)
{
    int i = 0;
    char value[DLT_CONFIG_FILE_ENTRY_MAX_LEN + 1] = {'\0'};
    DltFilterConfiguration tmp = { 0 };
    DltFilterConfiguration *conf = NULL;
    DltFilterConfiguration *configs = NULL;

    if ((mf == NULL) || (config == NULL) || (sec_name == NULL))
    {
        return -1;
    }

    conf = calloc(1, sizeof(DltFilterConfiguration));

    if (!conf)
    {
        dlt_vlog(LOG_ERR, "Configs could not be allocated\n");
        return DLT_RETURN_ERROR;
    }

    while (filter[i].name)
    {
        if (dlt_config_file_get_value(config,
                                      sec_name,
                                      filter[i].name,
                                      value) != DLT_RETURN_OK)
        {
            free(conf);
            return DLT_RETURN_ERROR;
        }

        if (filter[i].handler)
        {
            if (filter[i].handler(mf, &tmp, value) != DLT_RETURN_OK)
            {
                dlt_vlog(LOG_ERR,
                         "Failed to set injection parameter: %s\n",
                         filter[i].name);
                free(conf);
                return DLT_RETURN_ERROR;
            }
        }
        else
        {
            dlt_vlog(LOG_CRIT,
                     "No handler for option '%s' found\n",
                     filter[i].name);
            free(conf);
            return DLT_RETURN_ERROR;
        }

        i++;
    }

    /* set filter */
    if (tmp.name != NULL)
    {
        conf->name = strdup(tmp.name);
    }
    conf->level_min = 0;
    if (mf->configs == NULL)
    {
        conf->level_min = 0;
    }
    else
    {
        conf->level_min = mf->configs->level_max + 1;
    }
    conf->level_max = tmp.level_max;
    conf->client_mask = tmp.client_mask;
    conf->ctrl_mask = tmp.ctrl_mask;
    conf->num_injections = tmp.num_injections;
    if (tmp.num_injections > 0)
    {
        conf->injections = calloc(tmp.num_injections,
                           sizeof(char *));

        if (!conf->injections)
        {
            dlt_vlog(LOG_ERR, "Injections could not be allocated\n");
            return DLT_RETURN_ERROR;
        }
    }

    for(i = 0; i < tmp.num_injections; i++)
    {
        conf->injections[i] = strdup(tmp.injections[i]);
    }

    /* set level_min and set conf in the right place */
    configs = mf->configs;

    if (configs == NULL)
    {
        configs = conf;
        mf->configs = configs;
        mf->head = mf->configs;
    }
    else
    {
        if (configs->level_max < conf->level_max)
        {
            /* insert at the end */
            configs->next = conf;
            mf->configs = mf->configs->next;
        }
        else
        {
            /* go back to the head and look for the place to insert */
            configs = mf->head;
            if (dlt_daemon_add_filter_configuration(&configs, &configs, &conf) == 0)
            {
                /* insert at the head */
                mf->configs = configs;
                mf->head = mf->configs;
                if (mf->configs->next != NULL)
                {
                    mf->configs = mf->configs->next;
                }
            }
            else
            {
                while (configs->next)
                {
                    if (dlt_daemon_add_filter_configuration(&configs, &configs->next, &conf) == 0)
                    {
                        /* insert between mf->configs */
                        if (mf->configs->next != NULL)
                        {
                            mf->configs = mf->configs->next;
                        }
                        break;
                    }
                    configs = configs->next;
                }
            }
        }
    }

    free(tmp.name);
    for (i = 0; i < tmp.num_injections; i++)
    {
        free (tmp.injections[i]);
    }
    free(tmp.injections);

    return 0;
}

/**
 * @brief Set service identifier for an injection configuration
 *
 * Use the string from the configuration and configure the service identifier:
 *
 *  - num_injections == 0:  No injection is allowed
 *  - num_injections == -1: All injections are allowed
 *  - num_injections > 0:   Number of injections given
 *
 * @param ids[out]   Pointer to serivce identifier
 * @param num[out]   Number of service identifier
 * @param value      String taken from configuration file
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_set_injection_service_ids(int **ids,
                                                int *num,
                                                char *value)
{
    if ((ids == NULL) || (num ==  NULL) || (value == NULL))
    {
        return -1;
    }

    /* value is a komma separated list of injections or '*' or NONE */
    if (strncmp(value, "*", strlen("*")) == 0)
    {
        *num = -1;
    }
    else if(strncasecmp(value, DLT_FILTER_CLIENT_NONE, strlen(value)) == 0)
    {
        *num = 0;
    }
    else /* at least one specific service id is given */
    {
        int i;
        *num = 1;
        /* count numbers of commata to get number of */
        for(i = 0; value[i]; i++)
        {
            if (value[i] == ',')
            {
                *num += 1;
            }
        }

        if (*ids != NULL)
        {
            free(*ids);
        }

        *ids = calloc(*num, sizeof(int));
        if (*ids == NULL)
        {
            dlt_log(LOG_CRIT, "Failed to allocate memory for service IDs\n");
            return -1;
        }

        char *token = NULL;
        char *save_ptr;
        i = 0;

        token = strtok_r(value, ",", &save_ptr);
        while (token != NULL)
        {
            (*ids)[i] = strtol(token, NULL, 10);
            i++;
            token = strtok_r(NULL, ",", &save_ptr);
        }
    }

    return 0;
}

/**
 * @brief Find an injection configuration by given name
 *
 * This function searches for an injection configuration with the given name. It
 * is assumed that each injection has a unique name.
 *
 * @param injections    Array of known injections
 * @param name          Name of the injection configuration
 * @return Pointer to injection configuration on success, NULL otherwise
 */
DLT_STATIC DltInjectionConfig *dlt_daemon_filter_find_injection_by_name(
    DltInjectionConfig *injections,
    char *name)
{
    int i;

    if ((injections == NULL) || (name == NULL))
    {
        return NULL;
    }

    for (i = 0; i < DLT_FILTER_INJECTION_CONFIG_MAX; i++)
    {
        if (injections[i].name == NULL)
        {
            return NULL;
        }

        if (strncmp(injections[i].name, name, strlen(injections[i].name)) == 0)
        {
            /* injection found */
            return &injections[i];
        }
    }
    return NULL;
}

/**
 * Find an injection configuration by given name from char **
 *
 * @param num_injections number of injections
 * @param injections     char array of known injections
 * @param name           name of the injection configuration
 * @return 0 if found, -1 if not found
 */
DLT_STATIC int dlt_daemon_filter_find_injection(int num_injections, char **injections, char *name)
{
    int i = 0;

    if ((injections == NULL) || (name == NULL))
    {
        return -1;
    }

    for (i = 0; i < num_injections; i++)
    {
        if (injections[i] == NULL)
        {
            return -1;
        }

        if (strncmp(injections[i], name, strlen(injections[i])) == 0)
        {
            /* injection found */
            return 0;
        }
    }

    return -1;
}

DLT_STATIC int dlt_daemon_injection_name(DltMessageFilter *mf,
                                         DltInjectionConfig *config,
                                         char *value)
{
    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if (dlt_daemon_filter_find_injection_by_name(mf->injections,
                                                 value) != NULL)
    {
        dlt_vlog(LOG_ERR,
                 "Injection configuration name '%s'already in use\n",
                 value);
        return DLT_RETURN_ERROR;
    }

    config->name = strdup(value);
    if (config->name == NULL)
    {
        dlt_log(LOG_CRIT, "Cannot duplicate string\n");
        return -DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_injection_apid(DltMessageFilter *mf,
                                         DltInjectionConfig *config,
                                         char *value)
{
    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    config->apid = strdup(value);

    if (config->apid == NULL)
    {
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_injection_ctid(DltMessageFilter *mf,
                                         DltInjectionConfig *config,
                                         char *value)
{
    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    config->ctid = strdup(value);

    if (config->ctid == NULL)
    {
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_injection_ecu_id(DltMessageFilter *mf,
                                           DltInjectionConfig *config,
                                           char *value)
{
    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    config->ecuid = strdup(value);

    if (config->ecuid == NULL)
    {
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_injection_service_id(DltMessageFilter *mf,
                                               DltInjectionConfig *config,
                                               char *value)
{
    if ((mf == NULL) || (config == NULL) || (value == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if (dlt_daemon_set_injection_service_ids(&config->service_ids,
                                             &config->num_sevice_ids,
                                             value) == -1)
    {
        dlt_log(LOG_ERR, "Cannot set injection service ID\n");
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC injection_opts injection[] =
{
    {.name = "Name", .handler = dlt_daemon_injection_name},
    {.name = "LogAppName", .handler = dlt_daemon_injection_apid},
    {.name = "ContextName", .handler = dlt_daemon_injection_ctid},
    {.name = "NodeID", .handler = dlt_daemon_injection_ecu_id},
    {.name = "ServiceID", .handler = dlt_daemon_injection_service_id},
    {NULL, NULL}
};

/**
 * @brief Setup an injection message configuration
 *
 * Get all information for an injection message configuration from the message
 * filter configuration file and setup an injection message configuration.
 *
 * @param mf        Message Filter
 * @param config    Configuration file handle
 * @param sec_name   Section name
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_setup_injection_config(DltMessageFilter *mf,
                                             DltConfigFile *config,
                                             char *sec_name)
{
    int i = 0;
    DltInjectionConfig *tmp = NULL;
    char value[DLT_CONFIG_FILE_ENTRY_MAX_LEN + 1] = {'\0'};

    if ((mf == NULL) || (config == NULL) || (sec_name == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* find next free injection message configuration slot */
    if (mf->num_injections == DLT_FILTER_INJECTION_CONFIG_MAX)
    {
        dlt_log(LOG_ERR, "Maximum number of supported injections reached\n");
        return DLT_RETURN_ERROR;
    }

    /* set current injection configuration */
    tmp = &mf->injections[mf->num_injections];

    i = 0;
    while (injection[i].name)
    {
        if (dlt_config_file_get_value(config,
                                      sec_name,
                                      injection[i].name,
                                      value) != DLT_RETURN_OK)
        {
            dlt_vlog(LOG_ERR, "Failed to read parameter: %s\n", injection[i].name);
        }

        if (injection[i].handler)
        {
            if (injection[i].handler(mf, tmp, value) != DLT_RETURN_OK)
            {
                dlt_vlog(LOG_ERR,
                         "Failed to set injection parameter: %s\n",
                         injection[i].name);
                return DLT_RETURN_ERROR;
            }
        }
        else
        {
            dlt_vlog(LOG_CRIT,
                     "No handler for option '%s' found\n",
                     injection[i].name);
            return DLT_RETURN_ERROR;
        }

        i++;
    }

    mf->num_injections++; /* next injection */

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_get_name(DltMessageFilter *mf, char *val)
{
    if ((mf == NULL) || (val == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    mf->name = strdup(val);
    if (mf->name == NULL)
    {
        dlt_log(LOG_CRIT, "Cannot allocate memory for configuration name\n");
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_get_default_level(DltMessageFilter *mf, char *val)
{
    char *endptr;
    unsigned int tmp;

    if ((mf == NULL) || (val == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    tmp = strtoul(val, &endptr, 10);

    if (endptr == val)
    {
        dlt_log(LOG_WARNING, "Default Level is not a number\n");
        return DLT_RETURN_ERROR;
    }

    if (tmp > DLT_FILTER_LEVEL_MAX)
    {
        dlt_log(LOG_WARNING, "Default Level is invalid\n");
        return DLT_RETURN_ERROR;
    }

    /* set default level */
    mf->default_level = tmp;

    return DLT_RETURN_OK;
}

DLT_STATIC int dlt_daemon_get_backend(DltMessageFilter *mf, char *val)
{
    if ((mf == NULL) || (val == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    mf->backend = strdup(val);
    if (mf->backend == NULL)
    {
        dlt_log(LOG_ERR, "Cannot duplicate string for backend name\n");
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

DLT_STATIC general_opts general[] =
{
    {.name = "Name", .handler = dlt_daemon_get_name, .is_opt = 1},
    {.name = "DefaultLevel", .handler = dlt_daemon_get_default_level, .is_opt = 0},
    {.name = "Backend", .handler = dlt_daemon_get_backend, .is_opt = 1},
    {NULL, NULL, 0}
};

DLT_STATIC int dlt_daemon_setup_filter_properties(DltMessageFilter *mf,
                                              DltConfigFile *config,
                                              char *sec_name)
{
    char value[DLT_CONFIG_FILE_ENTRY_MAX_LEN + 1] = {'\0'};
    int i = 0;
    int ret = DLT_RETURN_OK;

    if ((mf == NULL) || (config == NULL) || (sec_name == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    while (general[i].name)
    {
        ret = dlt_config_file_get_value(config,
                                        sec_name,
                                        general[i].name,
                                        value);

        if (ret != DLT_RETURN_OK && general[i].is_opt)
        {
            dlt_vlog(LOG_INFO,
                    "Optional parameter '%s' not specified\n",
                    general[i].name);
            i++;
            ret = DLT_RETURN_OK; /* its OK to have no optional parameter */
            continue;
        }
        else if (ret != DLT_RETURN_OK)
        {
            dlt_vlog(LOG_ERR,
                    "Missing configuration for '%s'\n",
                    general[i].name);
            break;
        }

        if (general[i].handler)
        {
            ret = general[i].handler(mf, value);
        }
        else
        {
            dlt_vlog(LOG_CRIT,
                     "No handler for option '%s' found\n",
                     general[i].name);
            ret = DLT_RETURN_ERROR;
            break;
        }

        if (ret != DLT_RETURN_OK)
        {
            dlt_vlog(LOG_ERR,
                     "Configuration for '%s' is invalid: %s\n",
                     general[i].name,
                     value);
        }

        i++;
    }

    return ret;
}

/**
 * add or update the most closed filter
 *
 * @param config    Filter configuration
 * @param name      name of the filter
 * @param min       the lower value of the level range
 * @return DltFilterConfiguration
 */
DLT_STATIC DltFilterConfiguration* dlt_daemon_add_closed_filter(char *name,
                                      unsigned int min)
{
    DltFilterConfiguration *conf = NULL;

    if (name == NULL)
    {
        return NULL;
    }

    conf = calloc(1, sizeof(DltFilterConfiguration));

    if (conf == NULL)
    {
        dlt_vlog(LOG_ERR, "Configs could not be allocated\n");
        return conf;
    }

    conf->name = strdup(name);
    conf->level_min = min;
    conf->level_max = DLT_FILTER_LEVEL_MAX;
    init_flags(&conf->ctrl_mask);
    conf->client_mask = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;
    conf->num_injections = 0;

    return conf;
}

/**
 * @brief Compare flags
 *
 * @param flag1
 * @param flag2
 *
 * @return 0 if equal, otherwise -1
 */
DLT_STATIC int dlt_filter_control_compare_flags(const DltServiceIdFlag *flag1,
                                            const DltServiceIdFlag *flag2)
{
    if ((flag1 == NULL) || (flag2 == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if ((memcmp(flag1->upper, flag2->upper, sizeof(flag1->upper)) != 0) ||
        (memcmp(flag1->lower, flag2->lower, sizeof(flag1->lower)) != 0))
    {
        return DLT_RETURN_ERROR;
    }

    return DLT_RETURN_OK;
}

/**
 * check each client mask between filters and update the more opened filter
 *
 * @param conf      the more restricted filter
 * @param next_conf the more opened filter
 * return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_check_each_client_mask(DltFilterConfiguration *conf,
                                             DltFilterConfiguration *next_conf,
                                             unsigned int client)
{
    if ((conf == NULL) || (next_conf == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if ((conf->client_mask & client) &&
        !(next_conf->client_mask & client))
    {
        dlt_vlog(LOG_WARNING,
                 "client_mask: Open %s as much as %s\n",
                 next_conf->name,
                 conf->name);
        next_conf->client_mask |= client;
        return DLT_RETURN_OK;
    }

    return DLT_RETURN_TRUE;
}

/**
 * check client mask between filters and update the more opened filter
 *
 * @param conf      the more restricted filter
 * @param next_conf the more opened filter
 * return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_check_client_mask(DltFilterConfiguration *conf,
                                        DltFilterConfiguration *next_conf)
{
    if ((conf == NULL) || (next_conf == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* if conf has default or next_conf has all, do nothing */
    if ((conf->client_mask == DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK) ||
        (next_conf->client_mask == DLT_CON_MASK_ALL))
    {
        return DLT_RETURN_OK;
    }

    /* if conf has all but next_conf doesn't, open all clients */
    if (dlt_daemon_check_each_client_mask(conf, next_conf, DLT_CON_MASK_ALL) == DLT_RETURN_OK)
    {
        return DLT_RETURN_OK;
    }

    /* check each client mask */
    dlt_daemon_check_each_client_mask(conf, next_conf, DLT_CON_MASK_CLIENT_CONNECT);
    dlt_daemon_check_each_client_mask(conf, next_conf, DLT_CON_MASK_CLIENT_MSG_SERIAL);
    dlt_daemon_check_each_client_mask(conf, next_conf, DLT_CON_MASK_CLIENT_MSG_OFFLINE_TRACE);
    dlt_daemon_check_each_client_mask(conf, next_conf, DLT_CON_MASK_CLIENT_MSG_OFFLINE_LOGSTORAGE);

    return DLT_RETURN_OK;
}

/**
 * check ctrl mask between filters and update the more opened filter
 *
 * @param conf      the more restricted filter
 * @param next_conf the more opened filter
 * return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_check_ctrl_mask(DltFilterConfiguration *conf,
                                        DltFilterConfiguration *next_conf)
{
    DltServiceIdFlag none;
    DltServiceIdFlag all;
    int i = 0;
    memset(&none, 0, sizeof(DltServiceIdFlag));
    memset(&all, 0xFF, sizeof(DltServiceIdFlag));

    if ((conf == NULL) || (next_conf == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* if conf has none or next_conf has all, do nothing */
    if ((dlt_filter_control_compare_flags(&conf->ctrl_mask, &none) == 0) ||
        (dlt_filter_control_compare_flags(&next_conf->ctrl_mask, &all) == 0))
    {
        return DLT_RETURN_OK;
    }

    /* if conf has all but next_conf doesn't, open all ctrl masks */
    if ((dlt_filter_control_compare_flags(&conf->ctrl_mask, &all) == 0) &&
        (dlt_filter_control_compare_flags(&next_conf->ctrl_mask, &all) != 0))
    {
        dlt_vlog(LOG_WARNING,
                 "ctrl_mask: Open %s as much as %s\n",
                 next_conf->name,
                 conf->name);
        enable_all(&next_conf->ctrl_mask);
        return DLT_RETURN_OK;
    }

    /* check each ctrl masks */
    for (i = DLT_SERVICE_ID_SET_LOG_LEVEL; i < DLT_SERVICE_ID_LAST_ENTRY; i++)
    {
        if ((bit(&conf->ctrl_mask, i) == 1) && (bit(&next_conf->ctrl_mask, i) != 1))
        {
            dlt_vlog(LOG_WARNING,
                     "ctrl_mask: Open %s as much as %s\n",
                     next_conf->name,
                     conf->name);
            set_bit(&next_conf->ctrl_mask, i);
        }
    }

    for (i = DLT_SERVICE_ID_UNREGISTER_CONTEXT; i < DLT_USER_SERVICE_ID_LAST_ENTRY; i++)
    {
        if ((bit(&conf->ctrl_mask, i) == 1) && (bit(&next_conf->ctrl_mask, i) != 1))
        {
            dlt_vlog(LOG_WARNING,
                     "ctrl_mask: Open %s as much as %s\n",
                     next_conf->name,
                     conf->name);
            set_bit(&next_conf->ctrl_mask, i);
        }
    }

    return DLT_RETURN_OK;
}


/**
 * check injections between filters and update the more opened filter
 *
 * @param conf      the more restricted filter
 * @param next_conf the more opened filter
 * return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_check_injections(DltFilterConfiguration *conf,
                                       DltFilterConfiguration *next_conf)
{
    int i = 0;

    if ((conf == NULL) || (next_conf == NULL))
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    /* if conf has nothing or next_conf has all, do nothing */
    if ((conf->num_injections == 0) || (next_conf->num_injections < 0))
    {
        return DLT_RETURN_OK;
    }

    /* if conf has all but next_conf doesn't, add all injections */
    if ((conf->num_injections == -1) && (next_conf->num_injections != -1))
    {
        next_conf->num_injections = -1;

        dlt_vlog(LOG_WARNING,
                 "injections: Open %s as much as %s\n",
                 next_conf->name,
                 conf->name);
        next_conf->injections = NULL;
        return DLT_RETURN_OK;
    }

    /* check each injection */
    if (conf->num_injections > 0)
    {
        for (i = 0; i < conf->num_injections; i++)
        {
            /* if the injection is not found in next_conf, add */
            if (dlt_daemon_filter_find_injection(next_conf->num_injections,
                                                 next_conf->injections,
                                                 conf->injections[i]) != 0)
            {
                next_conf->num_injections++;

                dlt_vlog(LOG_WARNING,
                         "injections: Open %s as much as %s\n",
                         next_conf->name,
                         conf->name);

                if (next_conf->injections == NULL)
                {
                    next_conf->injections = calloc(next_conf->num_injections, sizeof(char *));
                    if (next_conf->injections == NULL)
                    {
                        dlt_log(LOG_CRIT,
                                "Memory allocation for injection configuration failed!\n");
                        return -1;
                    }
                    next_conf->injections[0] = strdup(conf->injections[i]);
                }
                else
                {
                    next_conf->injections[next_conf->num_injections - 1] = strdup(conf->injections[i]);
                }
            }
        }
    }

    return DLT_RETURN_OK;
}

/**
 * check if the filter levels are all covered and in correct restriction order
 *
 * @param mf    Message filter
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_check_filter_level(DltMessageFilter *mf)
{
    DltFilterConfiguration *conf;

    if (mf == NULL)
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if (mf->configs == NULL)
    {
        /* if no mf->configs is defined, add a most closed filter automatically */
        dlt_vlog(LOG_WARNING,
                 "Filter not defined. Add a most closed filter named '%s'.\n",
                 MOST_CLOSED_FILTER_NAME);
        mf->configs = dlt_daemon_add_closed_filter(MOST_CLOSED_FILTER_NAME, 0);

        if (mf->configs == NULL)
        {
            dlt_log(LOG_ERR, "Cannot prepare filter\n");
            return DLT_RETURN_ERROR;
        }
    }
    else
    {
        conf = mf->configs;

        while (conf)
        {
            if (conf->next == NULL)
            {
                /* check if it has DLT_FILTER_LEVEL_MAX as upper value of the level range */
                if (conf->level_max < DLT_FILTER_LEVEL_MAX)
                {
                    dlt_vlog(LOG_WARNING,
                             "Make %s level defined until %d\n",
                             conf->name,
                             DLT_FILTER_LEVEL_MAX);
                    conf->level_max = DLT_FILTER_LEVEL_MAX;
                }
            }
            else
            {
                /* check if the filter is in correct order
                 * lower level has more closed filter, higher level has more opened filter */
                /* client_mask */
                dlt_daemon_check_client_mask(conf, conf->next);

                /* control message */
                dlt_daemon_check_ctrl_mask(conf, conf->next);

                /* injections */
                dlt_daemon_check_injections(conf, conf->next);
            }

            conf = conf->next;
        }
    }

    return DLT_RETURN_OK;
}

/**
 * set default level
 *
 * @param mf    Message filter
 * @return 0 on success, -1 otherwise
 */
DLT_STATIC int dlt_daemon_set_default_level(DltMessageFilter *mf)
{
    DltFilterConfiguration *conf;

    if (mf == NULL)
    {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    conf = mf->configs;

    while (conf)
    {
        if (conf->level_max >= mf->default_level)
        {
            mf->current = conf;
            return DLT_RETURN_OK;
        }
        conf = conf->next;
    }

    /* if this happens, the default level was not acceptable.
     * this shouldn't happen. */
    dlt_vlog(LOG_ERR,
             "Default level %d is not acceptable\n",
             mf->default_level);

    return DLT_RETURN_ERROR;
}

int dlt_daemon_prepare_message_filter(DltDaemonLocal *daemon_local,
                                      int verbose)
{
    PRINT_FUNCTION_VERBOSE(verbose);

    if (daemon_local == NULL)
    {
        return -1;
    }

    DltMessageFilter *mf;
    char *filter_config = daemon_local->flags.msgFilterConfFile;
    DltConfigFile *config = NULL;
    int sec = 0;
    int num_sec = 0;
    int ret = 0;

    if (daemon_local == NULL)
    {
        return -1;
    }

    mf = &daemon_local->pFilter;

    mf->current = NULL;

    config = dlt_config_file_init(filter_config);
    if (config == NULL)
    {
        dlt_log(LOG_CRIT, "Failed to open filter configuration file\n");
        return -1;
    }

    dlt_config_file_get_num_sections(config, &num_sec);

    while (sec < num_sec)
    {
        char sec_name[DLT_CONFIG_FILE_ENTRY_MAX_LEN + 1] = {'\0'};

        if (dlt_config_file_get_section_name(config, sec, sec_name) == -1)
        {
            dlt_log(LOG_CRIT, "Failed to read section name\n");
            ret = -1;
            break;
        }

        if (strstr(sec_name, GENERAL_BASE_NAME) != NULL)
        {
            if (dlt_daemon_setup_filter_properties(mf, config, sec_name) == -1)
            {
                if (mf->name != NULL)
                {
                    free(mf->name);
                    mf->name = NULL;
                }

                if (mf->backend != NULL)
                {
                    free(mf->backend);
                    mf->backend = NULL;
                }

                dlt_vlog(LOG_CRIT, "Filter configuration [%s] is invalid\n", sec_name);
                ret = -1;
                break;
            }
        }
        else if (strstr(sec_name, FILTER_BASE_NAME) != NULL)
        {
            if (dlt_daemon_setup_filter_section(mf, config, sec_name) == -1)
            {
                dlt_vlog(LOG_CRIT, "Filter configuration [%s] is invalid\n", sec_name);
                ret = -1;
                break;
            }
        }
        else if (strstr(sec_name, INJECTION_BASE_NAME) != NULL)
        {
            if (dlt_daemon_setup_injection_config(mf, config, sec_name) == -1)
            {
                dlt_vlog(LOG_CRIT, "Filter configuration [%s] is invalid\n", sec_name);
                ret = -1;
                break;
            }
        }
        else /* unknown section */
        {
            dlt_vlog(LOG_WARNING, "Unknown section: %s", sec_name);
        }
        sec++;
    }

    /* set head pointer */
    mf->configs = mf->head;

    /* check if levels are all covered and in correct restriction order */
    if (dlt_daemon_check_filter_level(mf) == -1)
    {
        dlt_log(LOG_CRIT, "Filter level is not covered completely\n");
        ret = -1;
    }

    /* set default level */
    if (dlt_daemon_set_default_level(mf) == -1)
    {
        dlt_log(LOG_CRIT, "Could not set default level\n");
        ret = -1;
    }

    /* initialize backend if available */
    if ((ret == 0) && (mf->backend != NULL))
    {
        if (dlt_daemon_filter_backend_init(daemon_local,
                                           mf->default_level,
                                           verbose) != 0)
        {
            dlt_log(LOG_CRIT, "Filter backend initialization failed\n");
            ret = -1;
        }
    }

    dlt_config_file_release(config);

    return ret;
}

void dlt_daemon_cleanup_message_filter(DltDaemonLocal *daemon_local,
                                       int verbose)
{
    PRINT_FUNCTION_VERBOSE(verbose);

    int i = 0;
    DltMessageFilter *mf = NULL;
    DltFilterConfiguration *conf;

    if (daemon_local == NULL)
    {
        return;
    }

    mf = &daemon_local->pFilter;

    /* free filter configurations */
    while (mf->configs)
    {
        free(mf->configs->name);
        mf->configs->name = NULL;
        int j;
        for (j = 0; j < mf->configs->num_injections; j++)
        {
            free(mf->configs->injections[j]);
            mf->configs->injections[j] = NULL;
        }
        free(mf->configs->injections);
        mf->configs->injections = NULL;

        conf = mf->configs->next;
        free(mf->configs);
        mf->configs = conf;
    }

    /* free injection configurations */
    for (i = 0; i < DLT_FILTER_INJECTION_CONFIG_MAX; i++)
    {
        /* return when injection was not initialized */
        if (mf->injections[i].name != NULL)
        {
            free(mf->injections[i].name);
        }

        if (mf->injections[i].apid != NULL)
        {
            free(mf->injections[i].apid);
        }

        if (mf->injections[i].ctid != NULL)
        {
            free(mf->injections[i].ctid);
        }

        if (mf->injections[i].ecuid != NULL)
        {
            free(mf->injections[i].ecuid);
        }

        if (mf->injections[i].service_ids != NULL)
        {
            free(mf->injections[i].service_ids);
        }

        memset(&mf->injections[i], 0, sizeof(mf->injections[i]));
    }

    if (mf->backend != NULL)
    {
        dlt_daemon_filter_backend_deinit(daemon_local, verbose);
        free(mf->backend);
        mf->backend = NULL;
    }

    free(mf->name);
    mf->name = NULL;
    mf->current = NULL;
}

int dlt_daemon_filter_is_connection_allowed(DltMessageFilter *filter,
                                            DltConnectionType type)
{
    if (filter == NULL)
    {
        return -1;
    }

    DltFilterConfiguration *curr = filter->current;

    return curr->client_mask & DLT_CONNECTION_TO_MASK(type);
}

int dlt_daemon_filter_is_control_allowed(DltMessageFilter *filter,
                                         int message_id)
{
    if (filter == NULL)
    {
        return -1;
    }

    return bit(&(filter->current->ctrl_mask), message_id);
}

int dlt_daemon_filter_is_injection_allowed(DltMessageFilter *filter,
                                           char *apid,
                                           char *ctid,
                                           char *ecuid,
                                           int service_id)
{
    DltFilterConfiguration *curr = NULL;
    int i = 0;
    int j = 0;

    if ((filter == NULL) || (apid == NULL) || (ctid == NULL) || (ecuid == NULL))
    {
        return -1;
    }

    curr = filter->current;

    /* check first, if all or no injection is allowed */
    if (curr->num_injections == 0) /* no injection allowed */
    {
        return 0;
    }
    else if (curr->num_injections == -1) /* all allowed */
    {
        return 1;
    }

    /* Only a certain list of injection messages is allowed. This list is a
     * whitelist. That means, as soon as machting entry is found, true will
     * be returned. */
    for (i = 0; i < curr->num_injections; i++)
    {
        DltInjectionConfig *icfg = dlt_daemon_filter_find_injection_by_name(
            filter->injections,
            curr->injections[i]);
        if (icfg == NULL)
        {
            dlt_log(LOG_ERR, "Injection configuration entry not found!\n");
            return -1;
        }

        /* check application identifier, context identifier, node identifier
         * and injection id (service id). Every entry must be valid to allow
         * that injection message */
        if (strncmp(icfg->apid, apid, strlen(icfg->apid)) != 0)
        {
            break;
        }

        if (strncmp(icfg->ctid, ctid, strlen(icfg->ctid)) != 0)
        {
            break;
        }

        if (strncmp(icfg->ecuid, ecuid, strlen(icfg->ecuid)) != 0)
        {
            break;
        }

        for (j = 0; j < icfg->num_sevice_ids; j++)
        {
            /* if one of the stored ids is the same, the injection message
             * is valid */
            if (icfg->service_ids[j] == service_id)
            {
                return 1;
            }
        }
    }

    return 0;
}

int dlt_daemon_filter_change_filter_level(DltDaemonLocal *daemon_local,
                                          unsigned int level,
                                          int verbose)
{
    DltMessageFilter *mf = &daemon_local->pFilter;
    DltFilterConfiguration *conf;

    PRINT_FUNCTION_VERBOSE(verbose);

    if (daemon_local == NULL)
    {
        return -1;
    }

    if (level > DLT_FILTER_LEVEL_MAX)
    {
        dlt_vlog(LOG_ERR,
                 "Invalid arguments %s: %p, %u\n",
                 __func__,
                 daemon_local,
                 level);
        return -1;
    }

    conf = mf->configs;

    while (conf)
    {
        if (conf->level_max >= level)
        {
            mf->current = conf;
            break;
        }
        conf = conf->next;
    }

    /* if conf is NULL, the current level happens to be not updated */
    if (!conf)
    {
        dlt_vlog(LOG_ERR, "Level %d is not acceptable\n", level);
        return -1;
    }

    DltConnection *temp = dlt_connection_get_next(daemon_local->pEvent.connections,
                                                DLT_CON_MASK_CLIENT_CONNECT);

    if (temp == NULL)
    {
        /* this should not happen, because all connections are created on daemon
         * startup */
        dlt_log(LOG_CRIT, "Connection not found!\n");
        return -1;
    }

    /* Will activate the connection if allowed,
     * or deactivate it if its not allowed anymore
     */
    return dlt_connection_check_activate(&daemon_local->pEvent,
                                         temp,
                                         &daemon_local->pFilter,
                                         ACTIVATE);
}

int dlt_daemon_filter_process_filter_control_messages(
    DltDaemon *daemon,
    DltDaemonLocal *daemon_local,
    DltReceiver *receiver,
    int verbose)
{
    PRINT_FUNCTION_VERBOSE(verbose);

    (void) daemon; /* not needed here, satisfy compiler */
    (void) receiver; /* not needed here, satisfy compiler */

    if (daemon_local == NULL)
    {
        dlt_vlog(LOG_ERR, "Invalid function parameters in %s\n", __func__);
        return -1;
    }

    if (daemon_local->pFilter.backend != NULL)
    {
        /* call backend dispatch function with daemon_local as first param */
        return dlt_daemon_filter_backend_dispatch(daemon_local, &verbose);
    }

    return 0;
}
