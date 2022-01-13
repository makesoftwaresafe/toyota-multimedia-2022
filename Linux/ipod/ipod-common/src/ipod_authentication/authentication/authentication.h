#ifndef AUTHENTICATION_IMPORTAL_H
#define AUTHENTICATION_IMPORTAL_H

/************************************************************************
 * \file authentication.h
 *
 * \version $Id: authentication.h,v 1.9 2011/12/21 13:21:12 kgerhards Exp $
 *
 * \release $Name: IPOD_r2_D49 $
 *
 * \brief This is the header file of the authentication component for ipod
 *
 * \component ipod control
 *
 * \author Jens Lorenz
 *
 * \copyright (c) 2003 - 2010 ADIT Corporation
 *
 ***********************************************************************/

#include "adit_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_AUTH_CP_MAX_CERTLENGTH         1920
#define IPOD_AUTH_CP_SIGN_DATA_SIZE         128
#define IPOD_AUTH_CP_CHALLENGE_DATA_SIZE    20
#define IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE  33


#define IPOD_FAKE_CERTIFICATE_DATE  "/mnt/certificate.bin"
#define IPOD_FAKE_READ_BINALY "rb"
#define IPOD_FAKE_VERSION 2

#define IPOD_I2C_RETRY_COUNT 10


/* define return and error values */
/*! Completed successfully */
#define IPOD_AUTH_OK                                         0
/*! Command failed */
#define IPOD_AUTH_ERROR                                     -1
/*! Insufficient memory */
#define IPOD_AUTH_ERR_NOMEM                                 -33
/*! Processing aborted */
#define IPOD_AUTH_ERR_ABORT                                 -66

#define IPOD_AUTH_COPRO_2_0_B                                1

#define IPOD_AUTH_POS0      0
#define IPOD_AUTH_POS1      1

#define IPOD_AUTH_MSEC      1000
#define IPOD_AUTH_NSEC      1000000


typedef struct
{
    U8* iPodAuthDevicename;
    U8* iPodAuthIoctlRegAddr;
    U8* iPodAuthGPIOReady;
    U8* iPodAuthGPIOReset;
    S32 iPodAuthDevComShortWait;
    S32 iPodAuthDevComWait;
    S32 iPodAuthDevComLongWait;
    U8* iPodAuthAutoDetect;
} AuthenticationConfig_t;


/* Authentication CP use functions */
IMPORT S32 AuthenticationSetConfig(VP ConfigValues);

IMPORT S32  AuthenticationInit(void);
IMPORT S32  AuthenticationDeinit(void);
IMPORT S32  AuthenticationClearCertificate(void);
IMPORT void AuthenticationGetCertificate(U16 *CertLength, U8 *CertData);

/**************************************************************************//**
 * Get the serial number from the Authentication coprocessor
 *
 * \param[out] CertSerialNumber  Pointer where the Serial Number will be copied
 *
 * \return A signed integer value indicating success or failure
 * Returns Zero on success
 * Returns Less than Zero on failure
 * If return values is greater than zero, then it gives the Copro device version
 *
 * \see
 * \note
 ******************************************************************************/
IMPORT S32  AuthenticationGetSerialNumber(U8 *CertSerialNumber);
IMPORT S32 AuthenticationGetSignatureData(const U8  *ResponseBuffer,
                                           U16 ResponseLength,
                                           U16 *SignatureDataLength,
                                           U8 *SignatureData);

IMPORT S32  AuthenticationSetCertificate(U16 certLen, U8 *certData);
IMPORT S32  AuthenticationGetChallengeData(U16 *ChallengeDataLength,
                                           U8 *ChallengeData);
IMPORT S32  AuthenticationGetSignature(U16 sigDataLen, U8 *sigData);

/* Authentication CP test functions */
IMPORT S32  AuthenticationGetDeviceID (U32 *AuthenticationDeviceID);
IMPORT S32  AuthenticationGetFirmwareVersion (U8 *majorVer, U8 *minorVer);
IMPORT S32  AuthenticationGetProtocolVersion (U8 *majorVer, U8 *minorVer);
IMPORT S32  AuthenticationSelftest (U8 *certificate,
                                    U8 *private_key,
                                    U8 *ram_check,
                                    U8 *checksum);


#ifdef __cplusplus
}
#endif

#endif  /* AUTHENTICATION_IMPORTAL_H */
/************************************************************************
 * \history
 * $Log: authentication.h,v $
 * Revision 1.9  2011/12/21 13:21:12  kgerhards
 * SFEIIX-74 [b_MAIN] Support new Apple Spec R44 (ATS 2.2)
 *
 * Revision 1.7.4.1  2011/09/15 11:32:54  serhard
 * SWGIIX-1472: put define for extern c in each header file
 *
 * Revision 1.7  2011/08/11 09:06:26  serhard
 * revert changes to public authentication API (remove iPod prefix)
 *
 *
 * Committed on the Free edition of March Hare Software CVSNT Client.
 * Upgrade to CVS Suite for more features and support:
 * http://march-hare.com/cvsnt/
 *
 * Revision 1.6  2011/08/01 09:30:57  mshibata
 * Fixed for QAC,Lint.
 *
 * Revision 1.5  2011/04/08 07:35:33  jlorenz
 * Merge authentication and upate trace
 *
 * Revision 1.4  2011/04/06 10:26:26  mshibata
 * Added the retry of GetCertificatedata.
 * because GetCertificatedata is failed sometimes.
 * when it is failed, it retry. and it is working ok.
 *
 * Revision 1.3  2011/01/18 05:01:27  mshibata
 * Fixed for QAC,Lint.
 *
 * Revision 1.2  2010/09/30 05:55:09  kgerhards
 * Added Linux Support
 *
 * Revision 1.1  2010/09/20 07:49:39  jlorenz
 * Initial files.
 *
 ***********************************************************************/
