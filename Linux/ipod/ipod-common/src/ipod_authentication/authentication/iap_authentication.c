/************************************************************************
 * \file authentication_ipc.c
 *
 * \version $Id: authentication_ipc.c, 
 *
 * \release $Name: 
 *
 * \brief This is the implementation of the authentication IPC component for ipod on Linux
 *
 * \component ipod control
 *
 * \author M.Shibata
 *
 * \copyright (c) 2003 - 2011 ADIT Corporation
 *
 ***********************************************************************/

#include <adit_typedef.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>

#include <ipodauth.h>
#include "iap2_dlt_log.h"

#include "authentication.h"
#include "authentication_lx.h"
#include "authentication_configuration.h"


/* Functions to convert big endian values from CP to host endianness */
#if __BYTE_ORDER == __LITTLE_ENDIAN

#define AUTH_TO_HOST_16(x) bswap_16(x)
#define AUTH_TO_HOST_32(x) bswap_32(x)

#define IPODAUTH_RESET_HOLD_TIME 1  /* Time to hold reset line [ms] */
#define IPODAUTH_RESET_WAIT_TIME 50 /* Time to wait after reset [ms] */

#elif __BYTE_ORDER == __BIG_ENDIAN

#define AUTH_TO_HOST_16(x) (x)
#define AUTH_TO_HOST_32(x) (x)

#else

#error - only big and little endian systems are supported.

#endif

#define IPODAUTH_SYNC_SEM_NAME "/iPodAuthSyncSemlock"
sem_t *g_iPodAuthSyncSem = SEM_FAILED;

LOCAL S32 g_auth_data_fd    = -1;
LOCAL S32 g_auth_gpio_reset_fd   = -1;
LOCAL S32 g_auth_gpio_ready_fd   = -1;
LOCAL IPODCoProVer_t g_auth_cp_version = CP_NOT_SET;
static U8 g_cert_data[IPOD_AUTH_CP_MAX_CERTLENGTH];
static U16 g_cert_data_len = 0x00;
static U8 g_serial_num_data[IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE];
static U16 g_serial_num_data_len = 0x00;

/* Forward declaration of internal functions */
LOCAL S32 AUTH_reset_cp(void);
LOCAL S32 AUTH_ready_cp(U16 *retry_cnt);
LOCAL S32 AUTH_write_cp(S32 write_addr, VP buf, U32 length);
LOCAL S32 AUTH_read_cp(S32 read_addr, VP buf, U32 length);
LOCAL S32 AUTH_transmit_cp_pages(S32 addr,
                                 U8 *buf,
                                 U32 length,
                                 U8 page_size,
                                 BOOL do_read);
LOCAL void AUTH_OSSleep(U32 sleep_ms);

LOCAL S32 AuthenticationSyncInit(void);


LOCAL S32 AuthenticationSyncInit(void)
{
    S32 rc =  IPOD_AUTH_OK;
    S32 sem_count = 1;
    S8  sem_name[] = IPODAUTH_SYNC_SEM_NAME;
    sem_t *authSyncSem = SEM_FAILED;

    /* If sem_open() is called multiple times within the same process using the
     * same name, sem_open() will return a pointer to the same semaphore,as long
     * as another process has not used sem_unlink() to unlink the semaphore.
     *
     * g_iPodAuthSyncSem will always point to the same semaphore.
     */

    /* create and open the named semaphore.
     *
     * semaphore is created with sem_count as 1, to make sure only one thread
     * gets it. As per specification it will be created in a virtual filesystem
     * "/dev/shm" with name "sem.IPODAUTH_SYNC_SEM_NAME"
     */
    mode_t tmp_umask = umask(0);
    authSyncSem = sem_open((const char *)sem_name, O_CREAT | O_EXCL,
        S_IRUSR | S_IWUSR |S_IRGRP | S_IWGRP |S_IROTH | S_IWOTH,
        sem_count);
    umask(tmp_umask);

    if(authSyncSem == SEM_FAILED)
    {
        rc = IPOD_AUTH_ERROR;

        if(EEXIST == errno)
        {
            /* One or the other task will fail in the first sem_open() call
             * due to errno EEXIST. They can get the semaphore address with
             * this second sem_open() call .
             */
            authSyncSem = sem_open((const char *)sem_name, 0);

            if(authSyncSem == SEM_FAILED)
            {
                /*very less likely to happen. If we hit here, we are dead*/
                rc = IPOD_AUTH_ERROR;
            }
            else
            {
                rc =  IPOD_AUTH_OK;
                g_iPodAuthSyncSem  =  authSyncSem;
            }
        }
    }
    else
    {
        g_iPodAuthSyncSem  =  authSyncSem;
    }

    if(IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc ;
}

S32 AuthenticationSetConfig(VP ConfigValues)
{
    S32 rc = IPOD_AUTH_OK;

    if(ConfigValues != NULL)
    {
        /* AuthenticationGetDevconfParameter protected in AuthenticationInit() by semaphore. */
        /* Use AuthenticationSyncInit() to create or open semaphore to protect AuthenticationSetConfig(). */
        rc = AuthenticationSyncInit();
        if(rc == IPOD_AUTH_OK)
        {
            /* Lock semaphore during AuthenticationSetDevconfParameter(). */
            if(sem_wait(g_iPodAuthSyncSem) == IPOD_AUTH_OK)
            {
                rc = AuthenticationSetDevconfParameter((AuthenticationConfig_t*)ConfigValues);
                /* Unlock semaphore. */
                if(g_iPodAuthSyncSem != NULL)
                {
                    sem_post(g_iPodAuthSyncSem);
                }
            }
            else
            {
                rc = IPOD_AUTH_ERROR;
                IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Semaphore Lock Error");
            }
        }
    }
    else
    {
        rc = IPOD_AUTH_ERROR;
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d",rc);
    }

    return rc;
}


S32 AuthenticationSetAltAddress(IPOD_AUTH_Cfg *dcInfo)
{
    S32 rc = IPOD_AUTH_ERROR;
    int strcmpRes = -1;
    strcmpRes = strcmp((char *)(dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val), "17");
    if (strcmpRes == 0)
    {
        strcpy((char *)dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val,"16");
    }
    else
    {
        strcpy((char *)dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val,"17");
    }
    rc = iPodAuthComIoctl(g_auth_data_fd,
                         IPOD_AUTHCOM_IOCTL_I2C_SLAVE_ADDR,
                         (char*)dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val);
    if(rc != IPOD_AUTH_OK)
    {
        IAP2AUTHDLTLOG(DLT_LOG_WARN, "Ioctl returned %d, error %d %s",rc,errno,strerror(errno));
    }
    return rc;
}




S32 AuthenticationReadDeviceVersion(IPOD_AUTH_Cfg *dcInfo,BOOL alt)
{
    U16 tmp_ver = 0x00;
    S32 rc = IPOD_AUTH_ERROR;
    if(alt==TRUE)
    {
         rc = AuthenticationSetAltAddress(dcInfo);
    }
    rc = AUTH_read_cp(AUTH_DEV_VER_ADDR,
                      &tmp_ver,
                      AUTH_DEV_VER_SIZE);
    if(rc == IPOD_AUTH_OK)
    {
        g_auth_cp_version = (IPODCoProVer_t)(tmp_ver);
        IAP2AUTHDLTLOG(DLT_LOG_INFO, "CP Device version read successfully 0x%X ",g_auth_cp_version);
    }
    return rc;
}

/*
 * For 2.0C, the I2C address is configured in run time based on the
 * timing of the reset/Power signal. Since this is not predictable correctly in
 * many hardwares, we use the retry mechanism to find out the exact address chosen
 * by Copro.
 * Reset will alter the address of 2.0C, so reset must not be done.
 * NOTE: 2.0B CP may not respond without a reset, so on failure to detect any version,
 * 2.0B is chosen as default and handled with reset.
 */

S32 AuthenticationCPAutoDetect(IPOD_AUTH_Cfg *dcInfo)
{
    S32 rc = IPOD_AUTH_ERROR;
    rc =  AuthenticationReadDeviceVersion(dcInfo,FALSE);
    if(rc != IPOD_AUTH_OK)
    {
        IAP2AUTHDLTLOG(DLT_LOG_WARN, "CP Device version read Failed for address %s "
                                        "Retrying with alternate address",dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val);
        /* set alternate i2c address and try reading*/
        rc = AuthenticationReadDeviceVersion(dcInfo,TRUE);
        if(rc != IPOD_AUTH_OK)
        {
            /*Self detect could not find so leave default configurations 2.0B
             *If 2.0B is used, it will probably work after a reset  */
            IAP2AUTHDLTLOG(DLT_LOG_WARN, "CP Device version Read Failed for alternate "
                                                            "address %s",dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val);
            rc = AuthenticationSetAltAddress(dcInfo);
            g_auth_cp_version = CP_2_0_B;
            IAP2AUTHDLTLOG(DLT_LOG_WARN, "Reset to the default CP Device version 0x03"
                                                        " and address %s",dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val);
        }
    }
    if((g_auth_cp_version==CP_2_0_C) || (g_auth_cp_version==CP_3_0))
    {
        /*RESET and READY must not be used for 2.0C and 3.0 CP Versions
          because it will change the device address */
        strcpy((char *)dcInfo[IPOD_AUTH_DC_GPIO_RESET].para.p_val,"-1");
        strcpy((char *)dcInfo[IPOD_AUTH_DC_GPIO_READY].para.p_val,"-1");
    }
    return rc;
}


S32 AuthenticationInit(void)
{
    S32      rc = IPOD_AUTH_OK;
    IPOD_AUTH_Cfg *dcInfo = NULL;
    int strcmpRes = -1;
    S32 fd = -1;

    rc = AuthenticationSyncInit();
    if(rc == IPOD_AUTH_OK)
    {
        IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "AuthenticationInit: Waiting for semaphore");
        if(sem_wait(g_iPodAuthSyncSem) == IPOD_AUTH_OK)
        {
            IAP2AUTHDLTLOG(DLT_LOG_DEBUG, "AuthenticationInit: Got semaphore");
            rc = IPOD_AUTH_OK;
        }
        else
        {
            rc = IPOD_AUTH_ERROR;
            IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Semaphore Lock error");
        }
    }
    if(rc == IPOD_AUTH_OK)
    {
        rc = iPodAuthComInit();
    }
    if(rc == IPOD_AUTH_OK)
    {
        rc = AuthenticationGetDevconfParameter();
    }
    /* read PFCFG file */
    if(rc == IPOD_AUTH_OK)
    {
        /* get PFCFG configuration values */
        dcInfo = AuthenticationGetDevInfo();
    }

    if(NULL != dcInfo)
    {
        fd = iPodAuthComOpen((const char*)dcInfo[IPOD_AUTH_DC_DEV_NAME].para.p_val, IPOD_AUTHCOM_FLAGS_RDWR, IPOD_AUTHCOM_DATA_OPEN);
        if (fd < 0)
        {
            rc = -EINVAL;
            IAP2AUTHDLTLOG(DLT_LOG_ERROR, "Failed to open device driver");
        }
        else
        {
            /* return fd even if ioctl fails to close i2c device afterwards */
            g_auth_data_fd = fd;
            /* set i2c slave register address */
            rc = iPodAuthComIoctl(fd, IPOD_AUTHCOM_IOCTL_I2C_SLAVE_ADDR, (char*)dcInfo[IPOD_AUTH_DC_IOCTL].para.p_val);

            if(rc==IPOD_AUTH_OK)
            {
                strcmpRes = strcmp((char *)(dcInfo[IPOD_AUTH_DC_CP_AUTODETECT].para.p_val), "1");
                if((strcmpRes == 0) && (g_auth_cp_version == CP_NOT_SET))
                {
                    rc = AuthenticationCPAutoDetect(dcInfo);
                }
             }
        }

        if (rc == IPOD_AUTH_OK)
        {
            strcmpRes = strcmp((char *)(dcInfo[IPOD_AUTH_DC_GPIO_RESET].para.p_val), "-1");
            if (strcmpRes != 0)
            {
                g_auth_gpio_reset_fd = iPodAuthComOpen((const char*)dcInfo[IPOD_AUTH_DC_GPIO_RESET].para.p_val, IPOD_AUTHCOM_FLAGS_WRONLY, IPOD_AUTHCOM_GPIO_OPEN);
                if (g_auth_gpio_reset_fd < 0)
                {
                    rc = -EINVAL;
                    IAP2AUTHDLTLOG(DLT_LOG_ERROR, "GPIO Reset Port Open Failed");
                }
            }
        }

        if (rc == IPOD_AUTH_OK)
        {
            strcmpRes = strcmp((char *)(dcInfo[IPOD_AUTH_DC_GPIO_READY].para.p_val) , "-1");
            if (strcmpRes != 0)
            {
                g_auth_gpio_ready_fd = iPodAuthComOpen((const char*)dcInfo[IPOD_AUTH_DC_GPIO_READY].para.p_val, IPOD_AUTHCOM_FLAGS_RDONLY, IPOD_AUTHCOM_GPIO_OPEN);
                if (g_auth_gpio_ready_fd < 0)
                {
                    rc = -EINVAL;
                    IAP2AUTHDLTLOG(DLT_LOG_ERROR, "GPIO Ready Port Open Failed");
                }
            }
        }
    }
    else
    {
        rc = IPOD_AUTH_ERROR;
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }
    return rc;
}

S32 AuthenticationDeinit(void)
{
    S32 rc = IPOD_AUTH_OK;

    if (g_auth_gpio_ready_fd >= 0)
    {
        rc = iPodAuthComClose(g_auth_gpio_ready_fd);
        g_auth_gpio_ready_fd = -1;
    }
    if (g_auth_gpio_reset_fd >= 0)
    {
        rc = iPodAuthComClose(g_auth_gpio_reset_fd);
        g_auth_gpio_reset_fd = -1;
    }
    if (g_auth_data_fd >= 0)
    {
        rc = iPodAuthComClose(g_auth_data_fd);
        g_auth_data_fd = -1;
    }

    /* The allocated memory to store the authentication configuration of type IPOD_AUTH_Cfg
     * kept alive until the process dies and cleans itself. */

    rc = iPodAuthComDeinit();

    if(g_iPodAuthSyncSem != NULL)
    {
        rc = sem_post(g_iPodAuthSyncSem);
    }

    /*
     * The semaphore object is kept alive until the process dies and cleans
     * itself.
     *
     * rc = sem_close(g_iPodAuthSyncSem) is never executed [:(] .
     *
     * TODO: Find a cleaner solution for multiple authentication. A DAEMON
     * dedicated for accessing the authentication co-processor which acts as
     * a gateway and threads ( from a single process or multiple processes )
     * should access the co-processor through the DAEMON.
     */
    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}

/*
 * On Authentication failure, the certificate data must be cleared so it will be again
 * fetched from Copro
 */
S32 AuthenticationClearCertificate(void)
{
    S32 rc = IPOD_AUTH_OK;
    rc = AuthenticationSyncInit();
    if (rc == IPOD_AUTH_OK)
    {
         if( (g_cert_data_len != 0x00) || (g_serial_num_data_len != 0x00) )
         {
             g_cert_data_len = 0x00;
             g_serial_num_data_len = 0x00;
             IAP2AUTHDLTLOG(DLT_LOG_INFO, "Clearing Prefetched certificate data and serial number data");
         }
    }
    if(g_iPodAuthSyncSem != NULL)
    {
        rc = sem_post(g_iPodAuthSyncSem);
    }
    return rc;
}

/*
 * Since the certificate data remains same always, cache it in the memory for performance optimization
 * Reading every time from I2C will cost around 300 ms which can be saved if we cache Certificate data.
 */
void AuthenticationGetCertificate(U16 *cert_data_len, U8 *cert_data)
{
    S32 rc = IPOD_AUTH_OK;
    U16 tmp_len = 0x00;
    U8 CertSerialNum[IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE];

    if((cert_data_len != NULL) && (cert_data != NULL))
    {
        rc = AuthenticationInit();
        if (rc == IPOD_AUTH_OK)
        {
            if(g_cert_data_len == 0x00)
            {
                rc = AUTH_reset_cp();
                if (rc == IPOD_AUTH_OK)
                {
                    rc = AUTH_read_cp(AUTH_ACC_CERT_DAT_LEN_ADDR,
                                      &tmp_len,
                                      AUTH_ACC_CERT_DAT_LEN_SIZE);
                }

                if (rc == IPOD_AUTH_OK)
                {
                    /* PRQA: Lint Message 160: This error is occurred by Linux header. It can not fix in here */
                    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                    *cert_data_len = AUTH_TO_HOST_16(tmp_len); /*lint !e160 !e644 */

                    if (*cert_data_len <= IPOD_AUTH_CP_MAX_CERTLENGTH)
                    {
                        rc = AUTH_transmit_cp_pages(AUTH_ACC_CERT_DAT_ADDR,
                                                    cert_data,
                                                    (U32)*cert_data_len,
                                                    AUTH_ACC_CERT_DAT_PAG_SIZE,
                                                    TRUE);
                        if(rc == IPOD_AUTH_OK)
                        {
                            g_cert_data_len = *cert_data_len;
                            memcpy(g_cert_data, cert_data, g_cert_data_len);
                            IAP2AUTHDLTLOG(DLT_LOG_INFO, "Caching certificate for next attempts");
                        }
                    }
                    else
                    {
                        *cert_data_len = 0;
                    }
                }
            }
            else
            {
                *cert_data_len = g_cert_data_len;
                memcpy(cert_data, g_cert_data, g_cert_data_len);
                IAP2AUTHDLTLOG(DLT_LOG_INFO, "Using already cached certificate");
            }
        }
        AuthenticationDeinit();

        if(g_auth_cp_version >= CP_2_0_C)
        {
            rc = AuthenticationGetSerialNumber(CertSerialNum);
            IAP2AUTHDLTLOG(DLT_LOG_INFO, "AuthenticationGetSerialNumber returns %d", rc);
        }
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK == rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_INFO, "Received Certificate data Successfully ");
    }

    else
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d ",rc);
    }
    /* TODO from Jakob: error handling? zero at least the out parameter */
}

/* tested by calling the API iPodAuthenticateiPod */
S32 AuthenticationSetCertificate(U16 cert_data_len, U8 *cert_data)
{
    S32 rc;
    /* PRQA: Lint Message 160: This error is occurred by Linux header. It can not fix in here */
    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
    U16 tmp_len = AUTH_TO_HOST_16(cert_data_len); /*lint !e160 !e644 */

    if(cert_data != NULL)
    {
        rc = AUTH_reset_cp();
        rc = AUTH_write_cp(AUTH_IPOD_CERT_DAT_LEN_ADDR,
                           &tmp_len,
                           AUTH_IPOD_CERT_DAT_LEN_SIZE);

        if (rc == IPOD_AUTH_OK)
        {
            if (cert_data_len <= AUTH_MAX_IPOD_CERT_LEN)
            {
                rc = AUTH_transmit_cp_pages(AUTH_IPOD_CERT_DAT_ADDR,
                                            cert_data,
                                            (U32)cert_data_len,
                                            AUTH_IPOD_CERT_DAT_PAGE_SIZE,
                                            FALSE);
            }
        }
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK == rc)
	{
		IAP2AUTHDLTLOG(DLT_LOG_INFO, "Certificate data Transfer to CP Success ");
	}
	else
	{
		IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d ",rc);
	}

    return rc;
}

S32 AuthenticationGetSerialNumber(U8 *CertSerialNumber)
{
    S32 rc = IPOD_AUTH_ERROR;

    if(CertSerialNumber != NULL)
    {
        rc = AuthenticationInit();
        if (rc == IPOD_AUTH_OK)
        {
            if(g_auth_cp_version >= CP_2_0_C)
            {
                if(g_serial_num_data_len == 0x00)
                {
                    rc = AUTH_reset_cp();
                    if (rc == IPOD_AUTH_OK)
                    {
                        if(g_auth_cp_version == CP_2_0_C)
                        {
                            rc = AUTH_read_cp(AUTH_IPOD_CERT_SERIAL_NUMBER,
                                    CertSerialNumber,
                                    AUTH_IPOD_CERT_SERIAL_NUMBER_SIZE_CP_2C);
                            g_serial_num_data_len = AUTH_IPOD_CERT_SERIAL_NUMBER_SIZE_CP_2C;
                        }
                        else
                        {
                            rc = AUTH_read_cp(AUTH_IPOD_CERT_SERIAL_NUMBER,
                                      CertSerialNumber,
                                      AUTH_IPOD_CERT_SERIAL_NUMBER_SIZE_CP_3);
                            g_serial_num_data_len = AUTH_IPOD_CERT_SERIAL_NUMBER_SIZE_CP_3;
                        }
                    }
                    if(rc == IPOD_AUTH_OK)
                    {
                        CertSerialNumber[IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE - 1]= '\0';
                        IAP2AUTHDLTLOG(DLT_LOG_INFO, "Certificate serial number %s and size %d", CertSerialNumber, g_serial_num_data_len);
                        memcpy(g_serial_num_data,CertSerialNumber,IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE);
                        IAP2AUTHDLTLOG(DLT_LOG_INFO, "Certificate serial number is cached successfully for next attempts " );
                    }
                }
                else
                {
                    memcpy(CertSerialNumber,g_serial_num_data,IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE);
                    IAP2AUTHDLTLOG(DLT_LOG_INFO, "Using already cached Certificate serial number data %s ",CertSerialNumber);
                }
            }
            else
            {
                rc = CP_2_0_B;
                IAP2AUTHDLTLOG(DLT_LOG_INFO, "Certificate Serial Number cannot be read for 2.0B Copro");
            }
        }
        AuthenticationDeinit();
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK == rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_INFO, "Received Certificate Serial Number Successfully ");
    }
    else if(rc > IPOD_AUTH_OK)
    {
        IAP2AUTHDLTLOG(DLT_LOG_INFO, " returns %d ",rc);
    }
    else
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d ",rc);
    }
    return rc;
}


// TODO: test
S32 AuthenticationGetSignatureData(const U8 *response_buffer,
                                   U16 response_len,
                                   U16 *sig_data_len,
                                   U8 *sig_data)
{
    S32 rc = IPOD_AUTH_OK;
    U8  ctl_byte    = 0x01;
    U8  proc_result = 0x00;
    U16 tmp_sig_data_len = 0;
    U8 i = 0;

    if ((response_buffer == NULL) ||
        (sig_data_len == NULL) ||
        (sig_data == NULL))
    {
        return -EINVAL;
    }
    
    rc = AuthenticationInit();
    if (rc == IPOD_AUTH_OK)
    {
        rc = AUTH_reset_cp();
    }
    // Challenge Data
    rc = AUTH_write_cp(AUTH_CHALL_DAT_ADDR,
                       (VP)response_buffer,
                       response_len);
    if (rc == IPOD_AUTH_OK)
    {
        // Authentication Control and Status
        rc = AUTH_write_cp(AUTH_AUTH_CTL_STAT_ADDR,
                           &ctl_byte,
                           AUTH_AUTH_CTL_STAT_SIZE);
    }
    
    if (rc == IPOD_AUTH_OK)
    {
        for(i = 0; i < IPOD_I2C_RETRY_COUNT; i++)
        {
            // Authentication Control and Status
            rc = AUTH_read_cp(AUTH_AUTH_CTL_STAT_ADDR,
                              &proc_result,
                              sizeof(proc_result));
            if(rc == IPOD_AUTH_OK)
            {
                if(proc_result == 0x10)
                {
                    break;
                }
            }
            AUTH_OSSleep(30);
        }
    }
    
    tmp_sig_data_len = 0;

    if (rc == IPOD_AUTH_OK)
    {
        // Challenge Response Data Length
        rc = AUTH_read_cp(AUTH_SIG_DATA_LEN_ADDR,
                          &tmp_sig_data_len,
                          AUTH_SIG_DATA_LEN_SIZE);
        if (rc == IPOD_AUTH_OK)
        {
            /* PRQA: Lint Message 160: This error is occurred by Linux header. It can not fix in here */
            /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
            tmp_sig_data_len = AUTH_TO_HOST_16(tmp_sig_data_len); /*lint !e160 !e644 */
        }
        else
        {
            tmp_sig_data_len = 0;
        }
    }

    if ((rc == IPOD_AUTH_OK) && (IPOD_AUTH_CP_SIGN_DATA_SIZE >= tmp_sig_data_len))
    {
        // Challenge Response Data
        rc = AUTH_read_cp(AUTH_SIG_DATA_ADDR,
                          sig_data,
                          tmp_sig_data_len);
    }
    else
    {
        rc = IPOD_AUTH_ERROR;
    }

    AuthenticationDeinit();

    if (IPOD_AUTH_OK == rc)
    {
        *sig_data_len = tmp_sig_data_len;
        IAP2AUTHDLTLOG(DLT_LOG_INFO, "Received Challenge Response Successfully ");
    }
    else
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d ",rc);
    }

    return rc;
}

/* tested by calling the API iPodAuthenticateiPod */
S32 AuthenticationGetSignature(U16 sig_data_len, U8 *sig_data)
{
    S32 rc = IPOD_AUTH_OK;
    U8 ctl_byte = 0x03;
    U8 proc_result = 0x00;
    /* length must be converted for the Apple CP register */
    U16 tmp_sig_data_len = 0;

    if(sig_data != NULL)
    {
        /* PRQA: Lint Message 160: This error is occurred by Linux header. It can not fix in here */
        /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
        tmp_sig_data_len = AUTH_TO_HOST_16(sig_data_len); /*lint !e160 !e644 */

        rc = AUTH_write_cp(AUTH_SIG_DATA_LEN_ADDR,
                           &tmp_sig_data_len,
                           AUTH_SIG_DATA_LEN_SIZE);
        if (rc == IPOD_AUTH_OK)
        {
            /* do not use the converted length as number of bytes to write */
            rc = AUTH_write_cp(AUTH_SIG_DATA_ADDR,
                               sig_data,
                               sig_data_len);
        }

        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_write_cp(AUTH_AUTH_CTL_STAT_ADDR,
                               &ctl_byte,
                               AUTH_AUTH_CTL_STAT_SIZE);
            if (rc == IPOD_AUTH_OK)
            {
                rc = AUTH_read_cp(AUTH_AUTH_CTL_STAT_ADDR,
                                  &proc_result,
                                  AUTH_AUTH_CTL_STAT_SIZE);
                if (rc == IPOD_AUTH_OK)
                {
                    if (proc_result != 0x30)
                    {
                        rc = -EINVAL;
                    }
                }
            }
        }
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK == rc)
	{
		IAP2AUTHDLTLOG(DLT_LOG_INFO, "Received Signature data Successfully ");
	}
	else
	{
		IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d ",rc);
	}

    return rc;
}

/* tested by calling the API iPodAuthenticateiPod */
S32 AuthenticationGetChallengeData(U16 *challenge_data_len,
                                   U8 *challenge_data)
{
    S32 rc;
    U16 tmp_challenge_data_len;
    U8 ctl_cmd    = 0x02;
    U8 cmd_status = 0x00;

    if((challenge_data != NULL) && (challenge_data_len != NULL))
    {
        rc = AUTH_write_cp(AUTH_AUTH_CTL_STAT_ADDR,
                           &ctl_cmd,
                           AUTH_AUTH_CTL_STAT_SIZE);

        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_read_cp(AUTH_AUTH_CTL_STAT_ADDR,
                              &cmd_status,
                              AUTH_AUTH_CTL_STAT_SIZE);

            if (rc == IPOD_AUTH_OK)
            {
                if ((cmd_status & 0x20) != 0x20)
                {
                    rc = -EINVAL;
                }
            }
        }

        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_read_cp(AUTH_CHALL_DAT_LEN_ADDR,
                              &tmp_challenge_data_len,
                              AUTH_CHALL_DAT_LEN_SIZE);

            if (rc == IPOD_AUTH_OK)
            {
                /* PRQA: Lint Message 160: This error is occurred by Linux header. It can not fix in here */
                /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                *challenge_data_len = AUTH_TO_HOST_16(tmp_challenge_data_len); /*lint !e160 !e644 */
            }
            else
            {
                *challenge_data_len = 0;
            }
        }

        if ((rc == IPOD_AUTH_OK) && (IPOD_AUTH_CP_SIGN_DATA_SIZE >= *challenge_data_len))
        {
            rc = AUTH_read_cp(AUTH_CHALL_DAT_ADDR,
                              challenge_data,
                              *challenge_data_len);
        }
        else
        {
            rc = IPOD_AUTH_ERROR;
        }
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK == rc)
	{
		IAP2AUTHDLTLOG(DLT_LOG_INFO, "Received Challenge data Successfully ");
	}
    else
    {
    	IAP2AUTHDLTLOG(DLT_LOG_ERROR, " returns %d ",rc);
    }

    return rc;
}

/* Authentication CP test functions */
S32 AuthenticationGetDeviceID(U32 *auth_dev_id)
{
    S32 rc = IPOD_AUTH_OK;
    U32 tmp_id = 0;

    if (auth_dev_id != NULL)
    {
        rc = AuthenticationInit();
        if (rc == 0)
        {

            rc = AUTH_reset_cp();
            if (rc == 0)
            {
                rc = AUTH_read_cp(AUTH_DEV_ID_ADDR, &tmp_id, AUTH_DEV_ID_SIZE);
            }
            
            if (rc == 0)
            {
                /* PRQA: Lint Message 160: This error is occurred by Linux header. It can not fix in here */
                /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                *auth_dev_id = AUTH_TO_HOST_32(tmp_id); /*lint !e160 !e644 */
            }
        }

        AuthenticationDeinit();
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}

S32 AuthenticationGetFirmwareVersion(U8 *majorVer, U8 *minorVer)
{
    S32 rc = IPOD_AUTH_OK;

    if((majorVer != NULL) && (minorVer != NULL))
    {
        rc = AuthenticationInit();
        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_reset_cp();
        }

        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_read_cp(AUTH_FW_VER_ADDR, majorVer, AUTH_FW_VER_SIZE);
            *minorVer = 0;
        }
        AuthenticationDeinit();
    }
    else
    {
        rc = -EINVAL;
    }
    
    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}

S32 AuthenticationGetProtocolVersion(U8 *major_ver, U8 *minor_ver)
{
    S32 rc = IPOD_AUTH_OK;

    if ((major_ver != NULL) && (minor_ver != NULL))
    {
        rc = AuthenticationInit();
        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_reset_cp();
            if (rc == IPOD_AUTH_OK)
            {
                rc = AUTH_read_cp(AUTH_PROT_MAJOR_VERS_ADDR,
                                  major_ver,\
                                  AUTH_PROT_MAJOR_VERS_SIZE);
                if (rc == IPOD_AUTH_OK)
                {
                    rc = AUTH_read_cp(AUTH_PROT_MINOR_VERS_ADDR,
                                      minor_ver,
                                      AUTH_PROT_MINOR_VERS_SIZE);
                }
            }
        }
        AuthenticationDeinit();
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL, "returns %d",rc);
    }

    return rc;
}

// TODO: test
S32 AuthenticationSelftest(U8 *certificate,
                           U8 *private_key,
                           U8 *ram_check,
                           U8 *checksum)
{
    S32 rc = IPOD_AUTH_OK;
    U8 ctl_cmd = 0x01;
    U8 cmd_status;


    if ((certificate != NULL) && (private_key != NULL) &&
    (ram_check != NULL) && (checksum != NULL))
    {
        rc = AuthenticationInit();

        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_reset_cp();
        }

        if (rc == IPOD_AUTH_OK)
        {
            rc = AUTH_write_cp(AUTH_SELF_TEST_CTL_STAT_ADDR,
                       &ctl_cmd,
                       AUTH_SELF_TEST_CTL_STAT_SIZE);

            if (rc == IPOD_AUTH_OK)
            {
                rc = AUTH_read_cp(AUTH_SELF_TEST_CTL_STAT_ADDR,
                      &cmd_status,
                      AUTH_SELF_TEST_CTL_STAT_SIZE);
                if (rc == IPOD_AUTH_OK)
                {
                    *certificate = cmd_status & 0x80;
                    *private_key = cmd_status & 0x40;
                    *ram_check = 0;
                    *checksum = 0;
                }
            }
        }
        
        AuthenticationDeinit();
    }
    else
    {
        rc = -EINVAL;
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL, "returns %d",rc);
    }

    return rc;
}

/* internal functions */
LOCAL S32 AUTH_reset_cp(void)
{
    S32 rc = IPOD_AUTH_ERROR;
    S32 strcmpRes = -1;
    U8 data = 0;
    IPOD_AUTH_Cfg *dcInfo = NULL;

    /* read PFCFG file */
    rc = AuthenticationGetDevconfParameter();
    if(rc == IPOD_AUTH_OK)
    {
        /* get PFCFG configuration values */
        dcInfo = AuthenticationGetDevInfo();
    }

    if(dcInfo == NULL)
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL, "NULL config values");
        return IPOD_AUTH_ERROR;
    }
    
    strcmpRes = strcmp((char *)(dcInfo[IPOD_AUTH_DC_GPIO_RESET].para.p_val), "-1");
    if (strcmpRes != 0)
    {
        if(g_auth_gpio_reset_fd >= 0)
        {
            rc = iPodAuthComWrite(g_auth_gpio_reset_fd, 1, &data, 0);
            if(rc == 1)
            {
                AUTH_OSSleep(IPODAUTH_RESET_HOLD_TIME); /* reset hold time >500us) */
                data = 1;
                rc = iPodAuthComWrite(g_auth_gpio_reset_fd, 1, &data, 0);
                if(rc == 1)
                {
                    rc = IPOD_AUTHCOM_SUCCESS;
                }
            }
            else
            {
                rc = IPOD_AUTH_ERROR;
            }
            
            if(rc == IPOD_AUTHCOM_SUCCESS)
            {
                AUTH_OSSleep(IPODAUTH_RESET_WAIT_TIME); /* reset wait time >30ms */
            }
        }
        else
        {
            rc = IPOD_AUTH_ERROR;
        }
    }
    else
    {
        /* Anyway OK because reset is not needed */
        rc = IPOD_AUTHCOM_SUCCESS;
    }
        
    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}

LOCAL S32 AUTH_ready_cp(U16 *retry_cnt)
{
    S32 rc = IPOD_AUTH_OK;
    U32 length = 1;
    U8 buf[1] = {0};

    if (g_auth_gpio_ready_fd > 0)
    {
        rc = iPodAuthComRead(g_auth_gpio_ready_fd, length, buf, 0);
        if((U32)rc == length)
        {
            if(*buf == 1)
            {
                /* CP is ready */
                *retry_cnt = IPOD_I2C_RETRY_COUNT;
                rc = IPOD_AUTH_OK;
            }
            else
            {
                rc = IPOD_AUTH_ERROR;
            }
        }
        else
        {
            rc = IPOD_AUTH_ERROR;
        }
    }
    else
    {
        /* ready status not available */
        *retry_cnt = 0;
        rc = IPOD_AUTH_OK;
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}

LOCAL S32 AUTH_write_cp(S32 write_addr, VP buf, U32 length)
{
    S32 rc = IPOD_AUTH_OK;
    S32 ret = IPOD_AUTH_ERROR;
    U8 *data = NULL;
    U32 size = 0;
    U16 count = 0;
    IPOD_AUTH_Cfg *dcInfo = AuthenticationGetDevInfo();
    
    if ((buf != NULL) && (dcInfo != NULL))
    {
        /* allocate memory */
        data = (U8 *)calloc((length + 1), sizeof(U8));
        if (data != NULL)
        {

#ifdef IPOD_AUTH_USE_SPI
            U8 address = (U8)write_addr;

            size = 1 + 1;
            address = address | (1 << 7);
            data[0] = (address);
            data[1] = (U8)length;
            
            rc = AUTH_ready_cp(&count);
            if(rc == IPOD_AUTH_OK)
            {
                rc = iPodAuthComWrite(g_auth_data_fd, size, data, 0);
                if(rc == size)
                {
                    memcpy(data, buf, length);
                    size = (U8)length;
                    rc = IPOD_AUTH_OK;
                }
                else
                {
                    rc = IPOD_AUTH_ERROR;
                }
            }
#else
            /* insert register address */
            data[IPOD_AUTH_POS0] = write_addr;
            /* copy data */
            memcpy(&data[IPOD_AUTH_POS1], buf, length);
            /* adjust write size */
            size = length + 1;

            rc = IPOD_AUTH_OK;
#endif
        }
        else
        {
            rc = IPOD_AUTH_ERR_NOMEM;
            IAP2AUTHDLTLOG(DLT_LOG_FATAL, "No memory");
        }

        if(rc == IPOD_AUTH_OK)
        {
            rc = AUTH_ready_cp(&count);
            if(rc == IPOD_AUTH_OK)
            {
                do{
                    ret = iPodAuthComWrite(g_auth_data_fd, size, data, 0);
                    if(size != (U32)ret)
                    {
                        AUTH_OSSleep((U32)dcInfo[IPOD_AUTH_DC_DEV_COM_WAIT].para.val);
                    }
                    else
                    {
                        AUTH_OSSleep((U32)dcInfo[IPOD_AUTH_DC_DEV_COM_SHORT_WAIT].para.val);
                    }
                    count++;
                } while((size != (U32)ret) && (IPOD_I2C_RETRY_COUNT > count));

                /* check if all bytes could be written */
                if(size == (U32)ret)
                {
                    rc = IPOD_AUTH_OK;
                }
                else
                {
                    if(count >= IPOD_I2C_RETRY_COUNT )
                    {
                        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "iPodAuthComWrite failed with maximum retries %d", count);
                    }
                    rc = IPOD_AUTH_ERROR;
                }
            }
        }
        else
        {
            rc = IPOD_AUTH_ERROR;
        }
        /* release allocated memory */
        if(data != NULL)
        {
            free(data);
        }
    }
    else
    {
        rc = IPOD_AUTH_ERROR;
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}

LOCAL S32 AUTH_read_cp(S32 read_addr, VP buf, U32 length)
{
    S32 rc = IPOD_AUTH_OK;
    S32 ret = IPOD_AUTH_ERROR;
    U32 size = 0;
    U16 count = 0;
    IPOD_AUTH_Cfg *dcInfo = AuthenticationGetDevInfo();

    if ((buf != NULL) && (dcInfo != NULL))
    {
        rc = AUTH_ready_cp(&count);
        if(rc == IPOD_AUTH_OK)
        {
        
#ifdef IPOD_AUTH_USE_SPI
            U8 data[2] = {0};
            size = 1 + 1;
            data[0] =(U8)read_addr;
            data[1] = (U8)length;
            ret = iPodAuthComWrite(g_auth_data_fd, size, data, 0);
#else
            /* send read_addr to CP */
            U8 reg_addr = (U8)read_addr;
            size = 1;
            do{
                ret = iPodAuthComWrite(g_auth_data_fd, size, &reg_addr, 0);
                if(size != (U32)ret)
                {
                    /* write read-address fail very often if we did not wait at least 50ms */
                    AUTH_OSSleep((U32)dcInfo[IPOD_AUTH_DC_DEV_COM_LONG_WAIT].para.val);
                }
                else
                {
                    AUTH_OSSleep((U32)dcInfo[IPOD_AUTH_DC_DEV_COM_SHORT_WAIT].para.val);
                }
                count++;
            } while((size != (U32)ret) && (IPOD_I2C_RETRY_COUNT > count));

#endif
        }
        
        /* check if all bytes could be written */
        if((rc == IPOD_AUTH_OK) && ((U32)ret == size))
        {
            /* read from specific register address */
            rc = AUTH_ready_cp(&count);
            if(rc == IPOD_AUTH_OK)
            {
                do{
                    ret = iPodAuthComRead(g_auth_data_fd, length, buf, 0);
                    if(length != (U32)ret)
                    {
                        AUTH_OSSleep((U32)dcInfo[IPOD_AUTH_DC_DEV_COM_WAIT].para.val);
                    }
                    else
                    {
                        AUTH_OSSleep((U32)dcInfo[IPOD_AUTH_DC_DEV_COM_SHORT_WAIT].para.val);
                    }
                    count++;
                } while((length != (U32)ret) && (IPOD_I2C_RETRY_COUNT > count));

                /* check if all bytes could be read */
                if(length == (U32)ret)
                {
                    rc = IPOD_AUTH_OK;
                }
                else
                {
                    if(count >= IPOD_I2C_RETRY_COUNT )
                    {
                        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "iPodAuthComRead failed with maximum retries %d", count);
                    }
                    rc = IPOD_AUTH_ERROR;
                }
            }
        }
        else
        {
            rc = IPOD_AUTH_ERROR;
        }
    }
    else
    {
        rc = IPOD_AUTH_ERROR;
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_WARN, "returns %d",rc);
    }

    return rc;
}


LOCAL S32 AUTH_transmit_cp_pages(S32 addr,
                                 U8 *buf,
                                 U32 length,
                                 U8 page_size,
                                 BOOL do_read)
{
    S32 rc = IPOD_AUTH_OK;
    U8 i;
    U8 num_segments;
    U8 remaining_bytes;
    U8 transfer_len;
    U8 *buf_addr;

    if((buf == NULL) || (page_size == 0))
    {
        IAP2AUTHDLTLOG(DLT_LOG_FATAL, "No memory");
        return IPOD_AUTH_ERROR;
    }
    
    num_segments = length / page_size;
    remaining_bytes = length % page_size;
    for (i = 0; (i < num_segments + 1) && (rc == IPOD_AUTH_OK); i++)
    {
        buf_addr = &buf[i * page_size];

        if (i < num_segments) /* get full pages */
        {
            transfer_len = page_size;
        }
        else /* get remaining bytes */
        {
            transfer_len = remaining_bytes;
        }

        if (transfer_len != 0) /* only if remaining bytes != 0 */
        {
            if (do_read == TRUE)
            {
                rc = AUTH_read_cp(addr + i,
                                  buf_addr,
                                  (U32)transfer_len);
            }
            else
            {
                rc = AUTH_write_cp(addr + i,
                                   buf_addr,
                                   (U32)transfer_len);
            }
        }
    }

    if (IPOD_AUTH_OK != rc)
    {
        IAP2AUTHDLTLOG(DLT_LOG_ERROR, "returns %d",rc);
    }

    return rc;
}


LOCAL void AUTH_OSSleep(U32 sleep_ms)
{
    S32 s32ReturnValue = IPOD_AUTH_ERROR;
    struct timespec req;
    struct timespec remain;

    /* Initialize the structure */
    memset(&req, 0, sizeof(req));
    memset(&remain, 0, sizeof(remain));


    req.tv_sec = sleep_ms / IPOD_AUTH_MSEC;
    req.tv_nsec = (sleep_ms % IPOD_AUTH_MSEC) * IPOD_AUTH_NSEC;

    while(1)
    {
        s32ReturnValue = nanosleep(&req, &remain);

        if (s32ReturnValue == 0)
        {
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                req.tv_sec = remain.tv_sec ;
                req.tv_nsec = remain.tv_nsec;
            }
            else
            {
                break;
            }
        }
    }// end while

}
