#include <alsa/asoundlib.h>
#include <adit_typedef.h>
#include <stdbool.h>

#define CTRL_UAC2_ELEM_P_RATE 0
#define CTRL_UAC2_ELEMENTS 4
#define CTRL_UAC2_ELEM_C_RATE 1
#define CTRL_UAC2_ELEM_C_ENABLED 3
#define CTRL_UAC2_ELEM_P_ENABLED 2

struct uac2_ctrl {
	const char *name;
	snd_ctl_t *ctl;
	snd_ctl_elem_value_t *elems[CTRL_UAC2_ELEMENTS];
	unsigned long elems_val[CTRL_UAC2_ELEMENTS];
};

unsigned int s32ElemInit(struct uac2_ctrl *ctrl, unsigned int u32Index, const char * name);
unsigned int s32ElemUpdate(struct uac2_ctrl *ctrl, unsigned int u32Index);
unsigned int s32ElemVal(struct uac2_ctrl *ctrl, unsigned int u32Index);
unsigned int s32SndCtrlInit(struct uac2_ctrl *ctrl);
void* MonitorThreadFunction( void* pvArg );
S32 s32SndCtrlDeInit(struct uac2_ctrl *ctrl);

