/************************************************************************
 * \file authentication_lx.h
 *
 * \version $Id: authentication_lx.h,v
 *
 * \release $Name: IPOD_r2_D49 $
 *
 * \brief This is the header file of the authentication component for ipod
 *
 * \component ipod control
 *
 * \author Norbert Fleischmann
 *
 * \copyright (c) 2003 - 2011 ADIT Corporation
 *
 ***********************************************************************/

#ifndef AUTH_AUTHENTICATION_LX_H
#define AUTH_AUTHENTICATION_LX_H

#include <authentication.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_WAIT_FOR_CP_TIME_MS        10
#define T_SOMI_READY_US                 50

#define AUTH_DEV_VER_SIZE               1
#define AUTH_FW_VER_SIZE                1
#define AUTH_PROT_MAJOR_VERS_SIZE       1
#define AUTH_PROT_MINOR_VERS_SIZE       1
#define AUTH_DEV_ID_SIZE                4
#define AUTH_ERROR_CODE_SIZE            1
#define AUTH_AUTH_CTL_STAT_SIZE         1
#define AUTH_SIG_DATA_LEN_SIZE          2
#define AUTH_SIG_DATA_SIZE              128
#define AUTH_CHALL_DAT_LEN_SIZE         2
#define AUTH_CHALL_DAT_SIZE             20
#define AUTH_ACC_CERT_DAT_LEN_SIZE      2
#define AUTH_ACC_CERT_DAT_PAG_SIZE      128
#define AUTH_SELF_TEST_CTL_STAT_SIZE    1
#define AUTH_IPOD_CERT_DAT_LEN_SIZE     2
#define AUTH_IPOD_CERT_DAT_PAGE_SIZE    128
#define AUTH_IPOD_CERT_SERIAL_NUMBER_SIZE_CP_2C 31
#define AUTH_IPOD_CERT_SERIAL_NUMBER_SIZE_CP_3 32


#define AUTH_DEV_VER_ADDR               0x00
#define AUTH_FW_VER_ADDR                0x01
#define AUTH_PROT_MAJOR_VERS_ADDR       0x02
#define AUTH_PROT_MINOR_VERS_ADDR       0x03
#define AUTH_DEV_ID_ADDR                0x04
#define AUTH_ERROR_CODE_ADDR            0x05
#define AUTH_AUTH_CTL_STAT_ADDR         0x10
#define AUTH_SIG_DATA_LEN_ADDR          0x11
#define AUTH_SIG_DATA_ADDR              0x12
#define AUTH_CHALL_DAT_LEN_ADDR         0x20
#define AUTH_CHALL_DAT_ADDR             0x21
#define AUTH_ACC_CERT_DAT_LEN_ADDR      0x30
#define AUTH_ACC_CERT_DAT_ADDR          0x31
#define AUTH_SELF_TEST_CTL_STAT_ADDR    0x40
#define AUTH_IPOD_CERT_SERIAL_NUMBER    0x4E
#define AUTH_IPOD_CERT_DAT_LEN_ADDR     0x50
#define AUTH_IPOD_CERT_DAT_ADDR         0x51

#define AUTH_MAX_IPOD_CERT_LEN          1024

#ifdef __cplusplus
}
#endif

#endif /* AUTH_AUTHENTICATION_LX_H */
