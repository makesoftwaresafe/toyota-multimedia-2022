/**
 * \file: uac2Monitor.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * cinemo audio streaming monitoring sample rate change events
 *
 * \component: ipod cinemo
 *
 * \author: Vanitha.channaiah@in.bosch.com
 *
 * \copyright (c) 2019 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/
#ifndef UAC2MONITOR_H_
#define UAC2MONITOR_H_

#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include "cinemoAudio.h"

/* uac2 control elements */
#define CTRL_UAC2_ELEMENTS 4
#define CTRL_UAC2_ELEM_C_RATE 0
#define CTRL_UAC2_ELEM_P_RATE 1
#define CTRL_UAC2_ELEM_P_ENABLED  2
#define CTRL_UAC2_ELEM_C_ENABLED  3
#define DEFAULT_RATE 44100

typedef struct uac2CtrlData {
    const char *name;
    snd_ctl_t *ctl;
    snd_ctl_elem_value_t *elems[CTRL_UAC2_ELEMENTS];
    unsigned long elems_val[CTRL_UAC2_ELEMENTS];
} uac2_ctrl;

int uac2CtlElemInit(uac2_ctrl *ctrl, uint32_t idx,
        const char * name);
int uac2CtlElemUpdate(uac2_ctrl *ctrl, uint32_t idx);
int getUac2ElemVal(uac2_ctrl *ctrl, uint32_t idx);
int uac2CtrlInit(uac2_ctrl *ctrl);
void* UAC2DeviceMonitorThread( void* arg );
int s32SndCtrlDeInit(uac2_ctrl *ctrl);

#endif /* UAC2MONITOR_H_ */
