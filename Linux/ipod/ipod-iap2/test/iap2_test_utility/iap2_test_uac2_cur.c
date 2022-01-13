#include "iap2_test_uac2_cur.h"
#include "iap2_test_gstreamer.h"
unsigned int s32ElemInit(struct uac2_ctrl *ctrl, unsigned int u32Index, const char * name)
{
	int err;

	snd_ctl_elem_value_t **elem = &ctrl->elems[u32Index];
	if ((err = snd_ctl_elem_value_malloc(elem)) < 0)
	{
		printf("%s() ERROR: Unable to malloc: %s\n",__PRETTY_FUNCTION__, snd_strerror(err));
	}
	snd_ctl_elem_value_set_interface(*elem, SND_CTL_ELEM_IFACE_CARD);
	snd_ctl_elem_value_set_name(*elem, name);
	return s32ElemUpdate(ctrl, u32Index);
}

unsigned int s32ElemUpdate(struct uac2_ctrl *ctrl, unsigned int u32Index)
{
	int err;
	snd_ctl_elem_value_t **elem = &ctrl->elems[u32Index];
	if ((err = snd_ctl_elem_read(ctrl->ctl, *elem)) < 0)
	{
		printf("%s() ERROR: Unable to read contents: %s\n",__FUNCTION__, snd_strerror(err));
	}
	ctrl->elems_val[u32Index] = snd_ctl_elem_value_get_integer(*elem, 0);
	printf("%s() updated u32Index %d to val %ld\n",__PRETTY_FUNCTION__, u32Index,ctrl->elems_val[u32Index]);
	return err;
}

unsigned int s32ElemVal(struct uac2_ctrl *ctrl, unsigned int u32Index)
{
	return ctrl->elems_val[u32Index];
}

unsigned int s32SndCtrlInit(struct uac2_ctrl *ctrl)
{
	S32 err;

	ctrl->name = "hw:UAC2Gadget";
	err = snd_ctl_open(&ctrl->ctl, ctrl->name, SND_CTL_READONLY/*|SND_CTL_NONBLOCK*/);
	if(err)
	{
		ctrl->ctl = NULL;
		return err;
	}
	err = snd_ctl_subscribe_events(ctrl->ctl, 1);
	if(err)
	{
		snd_ctl_close(ctrl->ctl);
		ctrl->ctl = NULL;
		return err;
	}
	s32ElemInit(ctrl, CTRL_UAC2_ELEM_P_RATE, SND_CTL_NAME_PLAYBACK"sample rate");
	s32ElemInit(ctrl, CTRL_UAC2_ELEM_C_RATE, SND_CTL_NAME_CAPTURE"sample rate");
	s32ElemInit(ctrl, CTRL_UAC2_ELEM_P_ENABLED, SND_CTL_NAME_PLAYBACK"enabled");
	s32ElemInit(ctrl, CTRL_UAC2_ELEM_C_ENABLED, SND_CTL_NAME_CAPTURE"enabled");
	return err;
}

void vElemFree(struct uac2_ctrl *ctrl, U32 u32Index )
{
	snd_ctl_elem_value_t *elem = ctrl->elems[u32Index];

	if( NULL != elem )
	{
		snd_ctl_elem_value_free( elem );
	}
}

S32 s32SndCtrlDeInit(struct uac2_ctrl *ctrl)
{
	S32 err = 0;

	if( NULL == ctrl )
	{
		err = -1;
	}
	else
	{
		vElemFree(ctrl, CTRL_UAC2_ELEM_P_RATE );
		vElemFree(ctrl, CTRL_UAC2_ELEM_C_RATE );
		vElemFree(ctrl, CTRL_UAC2_ELEM_P_ENABLED );
		vElemFree(ctrl, CTRL_UAC2_ELEM_C_ENABLED );

		if( NULL != ctrl->ctl )
		{
			snd_ctl_close(ctrl->ctl);
			ctrl->ctl = NULL;
		}
	}

	return err;
}

void* MonitorThreadFunction( void* pvArg )
{
	U32 u32CaptureSampleRate = 0;
	U32 u32PlaybackSampleRate = 0;
	U32 u32CaptureEnabled = 0;
	U32 u32PlaybackEnabled = 0;
//	bool bCaptureEnabled = false;
	bool bReadFromSndCtl = false;
	S32 s32RetVal = 0;
	S32 s32SndCtlRetVal = 0;
	U32 u32sampleRate=0;

	struct uac2_ctrl sCtrl;
	snd_ctl_event_type_t enSndCtlEventType;
	snd_ctl_event_t *posSndCtlEvent = NULL;
	S32 *thd_stat;

	memset( &sCtrl , 0 , sizeof( struct uac2_ctrl ) );

	thd_stat = (S32 *) pvArg;


	if( 0 == s32RetVal )
	{
		s32SndCtlRetVal = snd_ctl_event_malloc(&posSndCtlEvent);
		if( 0 != s32SndCtlRetVal )
		{
			printf("vMonitorThreadFunction::ERROR: snd_ctl_event_malloc() returned error\n");
			posSndCtlEvent = NULL;
			s32RetVal = -2;
		}
		else if( NULL == posSndCtlEvent )
		{
			printf("vMonitorThreadFunction::ERROR: posSndCtlEvent is NULL!\n");
			s32RetVal = -2;
		}
	}

	if( 0 == s32RetVal )
	{
		printf("vMonitorThreadFunction:: Call s32SndCtrlInit()\n");
		s32SndCtlRetVal = s32SndCtrlInit(&sCtrl);
		if( 0 != s32SndCtlRetVal )
		{
			printf("vMonitorThreadFunction:: ERROR: s32SndCtrlInit() failed\n");
			s32RetVal = -2;
		}
		else
		{

			u32CaptureEnabled = s32ElemVal(&sCtrl, CTRL_UAC2_ELEM_C_ENABLED);
			bool bInitCaptureEnabled;
			bInitCaptureEnabled = ( 0 != u32CaptureEnabled);
			if (bInitCaptureEnabled)
			{
				printf("vMonitorThreadFunction:: Capture enabled (%i) after s32SndCtrlInit\n", bInitCaptureEnabled);
			}
			else
			{
				printf("vMonitorThreadFunction:: Capture disabled (%i) after s32SndCtrlInit\n", bInitCaptureEnabled);
			}
		}
	}

	while( ( 0 == s32RetVal ) && (*thd_stat) )
	{

		bReadFromSndCtl = false;

		s32SndCtlRetVal = snd_ctl_wait( sCtrl.ctl,
										500 /* timeout in ms */ );

		if( 0 > s32SndCtlRetVal )
		{
			printf("vMonitorThreadFunction:: snd_ctl_wait() returned error: %d\n", s32SndCtlRetVal);
			s32RetVal = 3;
		}
		else if( 0 == s32SndCtlRetVal )
		{
			//printf("vMonitorThreadFunction:: snd_ctl_wait() returned timeout\n");
		}
		else
		{
			printf("vMonitorThreadFunction:: snd_ctl_wait() returned error: %d\n", s32SndCtlRetVal);

			s32SndCtlRetVal = snd_ctl_read(sCtrl.ctl, posSndCtlEvent);

			if( 0 > s32SndCtlRetVal )
			{
				printf("vMonitorThreadFunction:: snd_ctl_read() returned error: %d\n", s32SndCtlRetVal);
				s32RetVal = -4;
			}
			else if( 0 == s32SndCtlRetVal )
			{
				printf("vMonitorThreadFunction:: snd_ctl_read() returned no events. Ignore\n");
			}
			else
			{
				enSndCtlEventType = snd_ctl_event_get_type(posSndCtlEvent);

				if (enSndCtlEventType != SND_CTL_EVENT_ELEM)
				{
					printf("vMonitorThreadFunction:: Ignore event of unhandled type %d\n", enSndCtlEventType);
				}
				else
				{
					bReadFromSndCtl = true;
				}
			}
		} /* else if( 0 == s32SndCtlRetVal ) */

		if( ( 0 == s32RetVal ) && ( true == bReadFromSndCtl ) )
		{
			s32ElemUpdate(&sCtrl, CTRL_UAC2_ELEM_C_RATE);
			u32CaptureSampleRate = s32ElemVal(&sCtrl, CTRL_UAC2_ELEM_C_RATE);

			s32ElemUpdate(&sCtrl, CTRL_UAC2_ELEM_P_RATE);
			u32PlaybackSampleRate = s32ElemVal(&sCtrl, CTRL_UAC2_ELEM_P_RATE);

			s32ElemUpdate(&sCtrl, CTRL_UAC2_ELEM_C_ENABLED);
			u32CaptureEnabled = s32ElemVal(&sCtrl, CTRL_UAC2_ELEM_C_ENABLED);

			s32ElemUpdate(&sCtrl, CTRL_UAC2_ELEM_P_ENABLED);
			u32PlaybackEnabled = s32ElemVal(&sCtrl, CTRL_UAC2_ELEM_P_ENABLED);

//			bCaptureEnabled = ( 0 != u32CaptureEnabled);

			printf("CTRL_UAC2_ELEM_C_ENABLED = %d, CTRL_UAC2_ELEM_C_RATE = %d\n", u32CaptureEnabled, u32CaptureSampleRate);
			printf("CTRL_UAC2_ELEM_P_ENABLED = %d, CTRL_UAC2_ELEM_P_RATE = %d\n", u32PlaybackEnabled, u32PlaybackSampleRate);

			if(u32sampleRate != u32CaptureSampleRate)
			{
				printf("Change in sample rate from %d to %d\n", u32sampleRate, u32CaptureSampleRate);
				u32sampleRate = u32CaptureSampleRate;
				iap2SetGstSampleRate2(u32CaptureSampleRate);
				iap2SetGstState(IAP2_GSTREAMER_SAMPLE_RATE_CHANGE);
				usleep(150); // As per code from MediaPlayer

			}
		} /* if( ( 0 == s32RetVal ) && ( true == bReadFromSndCtl ) ) */

	} /* while( ( 0 == s32RetVal ) && ( false == poMyself->_bTerminateMonitorThread ) ) */


	printf("vMonitorThreadFunction:: Cleanup\n");
	s32SndCtrlDeInit(&sCtrl);

	if( NULL != posSndCtlEvent )
	{
		snd_ctl_event_free( posSndCtlEvent );
		posSndCtlEvent = NULL;
	}

	printf("vMonitorThreadFunction:: Exit\n");
	pthread_exit(NULL);
}
