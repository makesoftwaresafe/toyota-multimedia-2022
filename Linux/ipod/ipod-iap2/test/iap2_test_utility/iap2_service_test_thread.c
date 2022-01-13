

/* **********************  includes  ********************** */
#include "iap2_test_init.h"
#include "iap2_test_utility.h"
#include <iap2_commands.h>
#include <iap2_file_transfer.h>
#include <iap2_external_accessory_protocol_session.h>
#include <sys/poll.h>
#include <mqueue.h>
#include <iap2_service_init.h>
#include "iap2_dlt_log.h"

/* **********************  functions  ********************** */

void iap2ServiceComThread(void* exinf)
{
    S32 rc = IAP2_CTL_ERROR;
    mqd_t mq_fd = -1;
    mqd_t mqAppFd = -1;

    iAP2PollFDs_t pollFDs[10];
    S32 desc_ready = 0;
    S32 j = 0;
    S32 cntfds = 0;
    int nfds = 0;       /* highest-numbered file descriptor in any of the three sets, plus 1 */
    fd_set read_fds;    /* will be watched to see if characters become available for reading */
    fd_set write_fds;

    iAP2Device_t* iap2device = (iAP2Device_t*)exinf;
    BOOL b_endComThread = FALSE;

    rc = iap2CreateMq(&mq_fd, TEST_MQ_NAME, O_RDONLY);
    if(rc == IAP2_OK){
        rc = iap2CreateMq(&mqAppFd, TEST_MQ_NAME_APP_TSK, O_WRONLY);
        if(rc != IAP2_OK){
            printf(" iap2ComThread():  create mq %s failed %d \n", TEST_MQ_NAME_APP_TSK, rc);
        }
    } else{
        printf(" iap2ComThread():  create mq %s failed %d \n", TEST_MQ_NAME, rc);
    }

    if(rc == IAP2_OK)
    {
        rc = iap2AddFDToPollFDs(&pollFDs[0], cntfds, mq_fd, POLLIN);
        if(rc >= IAP2_OK)
        {
            cntfds = rc;
            rc = IAP2_OK;
        }
        else
        {
            printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
        }
    }

    if(rc == IAP2_OK)
    {
        /* main loop */
        while (FALSE == b_endComThread)
        {
            /* FD_ZERO() clears out the fd_set, so it doesn't contain any file descriptors */
            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);

            rc = iap2AddFDsToFDset(&pollFDs[0], cntfds, &nfds, &read_fds, &write_fds);
            if(rc != IAP2_OK)
            {
                printf(" iap2AddFDsToFDset = %d \n", rc);
            }

            desc_ready = select(nfds+1, &read_fds, &write_fds, NULL, NULL);
            if(desc_ready > 0)
            {
                for(j = 0; (j < cntfds) && (desc_ready > 0) && (rc >= IAP2_OK); j++)
                {
                    if( (j < cntfds) && (FD_ISSET(pollFDs[j].fd, &read_fds)) )
                    {
//                        printf("    fd[%d] %d (event: %d) is set \n",
//                                j, pollFDs[j].fd, pollFDs[j].event);

                        if(mq_fd == pollFDs[j].fd)
                        {
                            rc = (*iap2HdlComThreadPollMqEvent)(iap2device, pollFDs[j].fd, mqAppFd, &b_endComThread);
                            if(rc < IAP2_OK){
                                printf(" %u ms  iap2HdlComThreadPollMqEvent = %d \n", iap2CurrTimeMs(), rc);
                            }
                        }
                        else
                        {
                            rc = iAP2HandleEvent(iap2device, pollFDs[j].fd, pollFDs[j].event);
                            if(rc < IAP2_OK){
                                printf(" %u ms  iAP2HandleEvent(1) = %d \n", iap2CurrTimeMs(), rc);
                            }
                        }
                        desc_ready--;
                    }
                    if( (j < cntfds) && (FD_ISSET(pollFDs[j].fd, &write_fds)) )
                    {
//                        printf("    fd[%d] %d (event: %d) is set \n",
//                                j, pollFDs[j].fd, pollFDs[j].event);

                        rc = iAP2HandleEvent(iap2device, pollFDs[j].fd, pollFDs[j].event);
                        if(rc < IAP2_OK){
                            printf(" %u ms  iAP2HandleEvent(2) = %d \n", iap2CurrTimeMs(), rc);
                        }
                        desc_ready--;
                    }
                }

                if(IAP2_DEV_NOT_CONNECTED == rc){
                    b_endComThread = TRUE;
                    /* set testStateError to TRUE */
                    iap2SetTestStateError(TRUE);

                    rc = iap2AddFDToPollFDs(&pollFDs[0], cntfds, mq_fd, POLLIN);
                    if(rc >= IAP2_OK){
                        cntfds = rc;
                        rc = IAP2_OK;
                    } else{
                        printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
                    }
                }
            }
            else
            {
                /* desc_ready = 0 : select() timed out
                 * desc_ready < 0 : indicates an error (check errno)
                 */
                printf("  myPollFDs failed = %d \n", desc_ready);

                if(desc_ready < 0)
                {
                    b_endComThread = TRUE;
                }
            }
        }/* while-select */
    }

    if(mqAppFd > 0){
        rc = mq_close(mqAppFd);
        mqAppFd = -1;
    }
    if(mq_fd > 0){
        rc = mq_close(mq_fd);
        mq_fd = -1;
    }

    printf(" %u ms  exit iap2ComThread \n", iap2CurrTimeMs());
    pthread_exit((void*)exinf);
}
