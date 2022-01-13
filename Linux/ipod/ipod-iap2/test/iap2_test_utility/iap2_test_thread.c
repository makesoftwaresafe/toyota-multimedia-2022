

/* **********************  includes  ********************** */
#include "iap2_test_init.h"
#include "iap2_test_utility.h"
#include <iap2_commands.h>
#include <iap2_file_transfer.h>
#include <iap2_external_accessory_protocol_session.h>
#include <sys/poll.h>
#include <mqueue.h>
#include "iap2_dlt_log.h"

/* **********************  functions  ********************** */

void iap2ComThread(void* exinf)
{
    S32 rc = IAP2_CTL_ERROR;
    mqd_t mq_fd = -1;
    mqd_t mqAppFd = -1;

    iAP2GetPollFDs_t getPollFDs;
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
        /* get file descriptors */
        rc = iAP2GetPollFDs(iap2device, &getPollFDs);
        printf(" %u ms  iAP2GetPollFDs = %d \n", iap2CurrTimeMs(), rc);

        for(j=0; j<getPollFDs.numberFDs; j++)
        {
            rc = iap2AddFDToPollFDs(&pollFDs[0], j, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);
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
        if(cntfds == getPollFDs.numberFDs)
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
        else
        {
            printf(" %u ms  cntfds: %d != %d numberFDs\n", iap2CurrTimeMs(), cntfds, getPollFDs.numberFDs);
            rc = IAP2_CTL_ERROR;
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

                    /* remove libusb file descriptors, but wait to handle mq messages */
                    rc = iAP2GetPollFDs(iap2device, &getPollFDs);

                    memset(&pollFDs[0], 0, sizeof(pollFDs));
                    cntfds = 0;
                    for(j=0; j<getPollFDs.numberFDs; j++)
                    {
                        rc = iap2AddFDToPollFDs(&pollFDs[0], j, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);
                        if(rc >= IAP2_OK){
                            cntfds = rc;
                            rc = IAP2_OK;
                        } else{
                            printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
                        }
                    }
                    if(cntfds == getPollFDs.numberFDs){
                        rc = iap2AddFDToPollFDs(&pollFDs[0], cntfds, mq_fd, POLLIN);
                        if(rc >= IAP2_OK){
                            cntfds = rc;
                            rc = IAP2_OK;
                        } else{
                            printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
                        }
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

S32 iap2AddFDToPollFDs(iAP2PollFDs_t* getPollFDs, S32 numFDs,
                       int fdToAdd, S16 eventToAdd)
{
    S32 rc = IAP2_OK;
    S32 i = 0;

    if(getPollFDs == NULL)
    {
        rc = IAP2_CTL_ERROR;
    }
    else
    {
        i = numFDs;

        getPollFDs[i].fd = fdToAdd;
        getPollFDs[i].event = eventToAdd;

        i++;
        rc = i;
    }

    return rc;
}

S32 iap2AddFDsToFDset(iAP2PollFDs_t* getPollFDs, S32 numFDs, int* maxfd,
                      fd_set* to_readfds, fd_set* to_writefds)
{
    S32 rc = 0;
    S32 i = 0;

    if((getPollFDs == NULL) || (to_readfds == NULL)
        || (to_writefds == NULL) || (maxfd == NULL))
    {
        rc = -1;
    }
    else
    {
        /* adds the file descriptors to the fd_set */
        for(i = 0; i < numFDs; i++)
        {
            /* find highest-numbered file descriptor */
            if(getPollFDs[i].fd > *maxfd)
            {
                *maxfd = getPollFDs[i].fd;
            }

            if(getPollFDs[i].event == POLLIN)
            {
                /* FD_SET() adds the file descriptor to the fd_set */
                FD_SET(getPollFDs[i].fd, to_readfds);
//                printf("  fd %d is set to read_fds, event %d \n",
//                        getPollFDs[i].fd, getPollFDs[i].event);
            }
            else if(getPollFDs[i].event == POLLOUT)
            {
                /* FD_SET() adds the file descriptor to the fd_set */
                FD_SET(getPollFDs[i].fd, to_writefds);
//                printf("  fd %d is set to write_fds, event %d \n",
//                        getPollFDs[i].fd, getPollFDs[i].event);
            }
            else
            {
                printf("  fd %d is used for unknown event %d \n",
                        getPollFDs[i].fd, getPollFDs[i].event);
            }
        }
        rc = 0;
    }

    return rc;
}
