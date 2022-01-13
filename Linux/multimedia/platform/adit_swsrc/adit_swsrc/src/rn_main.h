/**
 * \file: rn_main.h
 *
 * ADIT SRC core implementation.
 *
 * author: Andreas Pape / ADIT / SW1 / apape@de.adit-jv.com
 *
 * copyright (c) 2013 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#ifndef __RN_MAIN_H__
#define __RN_MAIN_H__

typedef struct {
	signed int in_range;/*additional input frames*/
	signed int out_range;/*additional output frames*/
}rate_core_pitch_init_t;

typedef struct {
	signed int in_pitch;/*additional input frames*/
	signed int out_pitch;/*additional output frames*/
}rate_core_pitch_t;


#define SRC_DOWNSAMPLING_BROKEN
//#define SRC_FILE_OUTPUT "/tmp/src_out.raw"
//#define SRC_FILE_INPUT "/tmp/src_in.raw"


#define PITCH_INIT_NO_PITCH (rate_core_pitch_init_t){ 0 }
#endif /* __RN_MAIN_H__*/


