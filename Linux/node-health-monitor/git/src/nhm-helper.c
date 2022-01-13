/* NHM - NodeHealthMonitor
 *
 * Copyright (C) 2013 Continental Automotive Systems, Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Author: Jean-Pierre Bogler <Jean-Pierre.Bogler@continental-corporation.com>
 */

/**
 * SECTION: nhm-helper
 * @title: NodeHealthMonitor (NHM) helper functions
 * @short_description: Functions commonly used by NHM
 *
 * The section implements common tasks of in NHM in separate functions.
 */


/******************************************************************************
*
* Header includes
*
******************************************************************************/

/* Own header files */
#include "nhm-helper.h"

/* System header files                   */
#include <dlt/dlt.h>       /* DLT trace  */
#include <glib-2.0/glib.h> /* Use GTypes */


/******************************************************************************
*
* Exported global variables and constants
*
******************************************************************************/

/* Context for Log'n'Trace */
DLT_DECLARE_CONTEXT(nhm_helper_trace_ctx);


/******************************************************************************
*
* Interfaces. Exported functions.
*
******************************************************************************/

/**
 * nhm_helper_str_in_strv:
 * @str:    String that is searched
 * @strv:   String array in which to search string
 * @return: %TRUE:  The string array contains the string.
 *          %FALSE: String is not in array.
 *
 * The function checks if a passed string is within the passed string array.
 */
gboolean
nhm_helper_str_in_strv(const gchar *str,
                       gchar       *strv[])
{
  gboolean retval = FALSE;
  guint    idx    = 0;

  if(strv != NULL)
  {
    for(idx = 0; (idx < g_strv_length(strv)) && (retval == FALSE); idx++)
    {
      retval = (g_strcmp0(str, strv[idx]) == 0);
    }
  }
  else
  {
    retval = FALSE;
  }

  return retval;
}
