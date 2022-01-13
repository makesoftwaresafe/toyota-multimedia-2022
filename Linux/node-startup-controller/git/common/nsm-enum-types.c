/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <glib-object.h>

#include <common/nsm-enum-types.h>



GType
nsm_shutdown_type_get_type (void)
{
  GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GFlagsValue values[] =
      {
        { NSM_SHUTDOWN_TYPE_NOT,    "NSM_SHUTDOWN_TYPE_NOT",    N_ ("No shutdown"),        },
        { NSM_SHUTDOWN_TYPE_NORMAL, "NSM_SHUTDOWN_TYPE_NORMAL", N_ ("Normal shutdown"),    },
        { NSM_SHUTDOWN_TYPE_FAST,   "NSM_SHUTDOWN_TYPE_FAST",   N_ ("Fast shutdown"),      },
        { NSM_SHUTDOWN_TYPE_RUNUP,  "NSM_SHUTDOWN_TYPE_RUNUP",  N_ ("Shutdown cancelled"), },
        { 0 ,                       NULL,                       NULL,                      },
      };

      type = g_flags_register_static ("NSMShutdownType", values); 
    }
  return type;
}



GType
nsm_error_status_get_type (void)
{
  GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GEnumValue values[] =
      {
        { NSM_ERROR_STATUS_NOT_SET,          "NSM_ERROR_STATUS_NOT_SET",          N_ ("Error not set"),           },
        { NSM_ERROR_STATUS_OK,               "NSM_ERROR_STATUS_OK",               N_ ("No error"),                },
        { NSM_ERROR_STATUS_ERROR,            "NSM_ERROR_STATUS_ERROR",            N_ ("Non-specific error"),      },
        { NSM_ERROR_STATUS_DBUS,             "NSM_ERROR_STATUS_DBUS",             N_ ("Dbus comunication error"), },
        { NSM_ERROR_STATUS_INTERNAL,         "NSM_ERROR_STATUS_INTERNAL",         N_ ("Internal error"),          },
        { NSM_ERROR_STATUS_PARAMETER,        "NSM_ERROR_STATUS_PARAMETER",        N_ ("Parameter wrong"),         },
        { NSM_ERROR_STATUS_WRONG_SESSION,    "NSM_ERROR_STATUS_WRONG_SESSION",    N_ ("Unknown session"),         },
        { NSM_ERROR_STATUS_RESPONSE_PENDING, "NSM_ERROR_STATUS_RESPONSE_PENDING", N_ ("Reponse pending"),         },
        { NSM_ERROR_STATUS_LAST,             "NSM_ERROR_STATUS_LAST",             N_ ("Last error"),              },
        { 0 ,                                NULL,                                NULL,                           },
      };

      type = g_enum_register_static ("NSMErrorStatus", values);
    }
  return type;
}



GType
nsm_node_state_get_type (void)
{
  GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GEnumValue values[] =
      {
        { NSM_NODE_STATE_NOT_SET,           "NSM_NODE_STATE_NOT_SET",           N_ ("Node sate not set"),             },
        { NSM_NODE_STATE_START_UP,          "NSM_NODE_STATE_START_UP",          N_ ("Start basic system"),            },
        { NSM_NODE_STATE_BASE_RUNNING,      "NSM_NODE_STATE_BASE_RUNNING",      N_ ("Basic components started"),      },
        { NSM_NODE_STATE_LUC_RUNNING,       "NSM_NODE_STATE_LUC_RUNNING",       N_ ("LUC components started"),        },
        { NSM_NODE_STATE_FULLY_RUNNING,     "NSM_NODE_STATE_FULLY_RUNNING",     N_ ("Foreground components started"), },
        { NSM_NODE_STATE_FULLY_OPERATIONAL, "NSM_NODE_STATE_FULLY_OPERATIONAL", N_ ("All components started"),        },
        { NSM_NODE_STATE_SHUTTING_DOWN,     "NSM_NODE_STATE_SHUTTING_DOWN",     N_ ("Shutdown the system"),           },
        { NSM_NODE_STATE_FAST_SHUTDOWN,     "NSM_NODE_STATE_FAST_SHUTDOWN",     N_ ("Fast shutdown active"),          },
        { NSM_NODE_STATE_DEGRADED_POWER,    "NSM_NODE_STATE_DEGRADED_POWER",    N_ ("Power state node degraded"),     },
        { NSM_NODE_STATE_SHUTDOWN,          "NSM_NODE_STATE_SHUTDOWN",          N_ ("Node shut down"),                },
        { NSM_NODE_STATE_LAST,              "NSM_NODE_STATE_LAST",              N_ ("Last node state"),               },
        { 0 ,                               NULL,                               NULL,                                 },
      };

      type = g_enum_register_static ("NSMNodeState", values);
    }
  return type;
}
