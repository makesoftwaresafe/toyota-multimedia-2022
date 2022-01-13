#include <stdio.h>      /* for printf() and fprintf() */
#include <float.h>
#include <stdlib.h>     /* for atoi(), abort() */
#include <string.h>     /* for memset() */

#include "dlt.h"

/* Context declaration.. */
DLT_DECLARE_CONTEXT(context);

int test_01(void);

/**
   * Main function
    */
int main(int argc, char* argv[])
{
    argc = argc;
    argv = argv;

    /* Register APP */
    DLT_REGISTER_APP("DLHP", "DLT log storage Test");

    /* Register CONTEXTS... */
    dlt_register_context_hp(&context, "TEST", "HP log test", DLT_TRACE_BUF_SMALL);

    test_01();

    return 0;
}

int test_01()
{
    char buffer[1024 * 1];
    int num;

    for (num = 1; num <= 1024 * 1; num++)
    {
        buffer[num] = num;
    }

    /* Show all log messages and traces */
    dlt_set_application_ll_ts_limit(DLT_LOG_VERBOSE, DLT_TRACE_STATUS_ON);

    /* Dummy message: 16 byte header, 1k payload */
    dlt_user_trace_network_hp(&context,
            DLT_NW_TRACE_HP0, 16, buffer,
            1024 * 1, buffer);

   while(1)
   {
       sleep(5);
   }
    dlt_set_application_ll_ts_limit(DLT_LOG_DEFAULT, DLT_TRACE_STATUS_DEFAULT);
    sleep(2);


    return 0;
}
