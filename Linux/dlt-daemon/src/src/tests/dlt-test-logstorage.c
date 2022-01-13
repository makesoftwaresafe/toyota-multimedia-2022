#include <stdio.h>      /* for printf() and fprintf() */
#include <float.h>
#include <stdlib.h>     /* for atoi(), abort() */
#include <string.h>     /* for memset() */

#include "dlt.h"

#define LOG_DELAY       200 * 1000
#define NUM_LOG_MSGS    10

int test_01(void);


/* Context declaration.. */
DLT_DECLARE_CONTEXT(context);

/**
   * Main function of tool.
    */
int main(int argc, char* argv[])
{
         argc = argc;
         argv = argv;

    /* Register APP */
    DLT_REGISTER_APP("DLST","DLT log storage Test");

    /* Register CONTEXTS... */
    DLT_REGISTER_CONTEXT(context,"TEST","Information context");

    printf("========================DLT log storage Test============================\n");

    test_01();

    printf("=====================================================================\n");
    return 0;
}

/* Test setting of log level in application on plugin of external device */
int test_01()
{
    int i;

    /* Tests starting */
    printf("test_01 starting\n");
    printf("test_01: Sending log messages with level : \n");
    printf("         FATAL\n");
    printf("         ERROR\n");
    printf("         WARN\n");

    printf("test_01: Check DLT viewer\n");
    printf("test_01: Log messages with FATAL, ERROR should be seen\n");
    printf("test_01: Connect USB to TARGET\n");

    for(i=1;i<=NUM_LOG_MSGS;i++)
    {
        printf("Send log message  %d\n",i);

        DLT_LOG(context,DLT_LOG_FATAL,DLT_STRING("DLT Log Storage Test"),DLT_INT(i));
        DLT_LOG(context,DLT_LOG_ERROR,DLT_STRING("DLT Log Storage Test"),DLT_INT(i));
        DLT_LOG(context,DLT_LOG_WARN,DLT_STRING("DLT Log Storage Test"),DLT_INT(i));

        usleep(LOG_DELAY);
    }
    printf("test_01: Remove  USB from TARGET\n");
    printf("test_01: Open log file stored in USB using DLT viewer\n");

    return 0;
}
