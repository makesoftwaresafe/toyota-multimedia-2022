/*****************************************************************************
    Project:        Cinemo Media Engine

    Developed by:   Cinemo GmbH
                    Karlsruhe, Germany

    Copyright (c)   2009-2017 Cinemo GmbH and its licensors.
                    All rights reserved.

    This software is supplied only under the terms of a license agreement,
    nondisclosure agreement or other written agreement with Cinemo GmbH.
    Use, redistribution or other disclosure of any parts of this software
    is prohibited except in accordance with the terms of such written
    agreement with Cinemo GmbH. This software is confidential and proprietary
    information of Cinemo GmbH and its licensors.

    In case of licensing questions please contact: sales@cinemo.com
*****************************************************************************/

#ifndef CINEMO_APPLE_AUTH_INTERFACE_H_INCLUDED
#define CINEMO_APPLE_AUTH_INTERFACE_H_INCLUDED


#define CINEMO_AUTHENTICATION_LIBRARY_PROTOCOL_VERSION 1

#define CALI_NO_ERROR               0
#define CALI_ERROR_ARGUMENTS        -1
#define CALI_ERROR_IO               -2
#define CALI_ERROR_NOTIMPLEMENTED   -3
#define CALI_ERROR_OPEN             -4
#define CALI_ERROR_OVERFLOW         -5
#define CALI_ERROR_RESOURCES        -6

typedef struct cali_create_params {
    const char* szurl;
    const char* szoption;
} cali_create_params;

typedef void* cali_handle;

typedef int (*cali_delete_authentication_fn)(cali_handle auth_handle);
typedef int (*cali_get_serial_number_fn)(cali_handle auth_handle, void* pdest, unsigned int npdest);
typedef int (*cali_get_certificate_fn)(cali_handle auth_handle, void* pdest, unsigned int npdest);
typedef int (*cali_get_challenge_response_fn)(cali_handle auth_handle, void* pdest, unsigned int npdest, const void* pchallenge, unsigned int nbytes);

typedef struct cali_context {
    unsigned short protocol_version;
    cali_handle auth_handle;
    cali_delete_authentication_fn delete_authentication;
    cali_get_serial_number_fn get_serial_number;
    cali_get_certificate_fn get_certificate;
    cali_get_challenge_response_fn get_challenge_response;
} cali_context;

typedef int (*cali_create_apple_auth_fn)(const struct cali_create_params* params, struct cali_context* ctx);


int Create_AppleAuth_Custom(const struct cali_create_params * params,struct cali_context * ctx);
int Delete_AppleAuth_Custom(cali_handle auth_handle);
int Get_Serial_Number_Custom(cali_handle auth_handle,void * pdest,unsigned int npdest);
int Get_Certificate_Custom(cali_handle auth_handle,void * pdest,unsigned int npdest);
int Get_Challenge_Response_Custom(cali_handle auth_handle,void * pdest,unsigned int npdest,const void * pchallenge,unsigned int nbytes);

#endif // CINEMO_APPLE_AUTH_INTERFACE_H_INCLUDED
