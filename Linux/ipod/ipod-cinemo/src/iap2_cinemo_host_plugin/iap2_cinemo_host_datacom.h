#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <unistd.h>
#include <poll.h>

#include "cinemo_transport.h"

#define IPOD_IAP2_CINEMO_USBHOST_STRING_MAX        256

enum _TransportState
{
    TransportEnabled   = 0,  /**< Enabled State */
    TransportDisabled  = 1,  /**< Aborted State */
};

typedef enum _TransportState CinemoTransportState;

typedef struct _IPOD_IAP2_CINEMO_HOSTDEV_INFO
{
    int read_fd;
    int write_fd;
    /* Audio Device Name e.g. hw:UAC2Gadget,0  */
    char* AudioSource;
    void* audioctx;
    int abort_fd;
} IPOD_IAP2_CINEMO_HOSTDEV_INFO;

int Audio_Create_Custom( ctli_handle tphandle);
int Audio_Abort_Custom( ctli_handle tphandle );
int Audio_Receive_Custom( ctli_handle tphandle, void* pdest, unsigned int npdest );
int Audio_Set_Params_Custom( ctli_handle tphandle, const ctli_audio_params* params );
int Audio_Get_Params_Custom( ctli_handle tphandle, ctli_audio_params* params );
int Audio_Delete_Custom( ctli_handle tphandle );

#ifdef __cplusplus
}
#endif
