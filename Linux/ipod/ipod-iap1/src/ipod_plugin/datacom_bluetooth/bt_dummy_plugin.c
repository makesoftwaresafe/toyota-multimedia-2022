
#include "iPodDataCom.h"
#include "iap1_dlt_log.h"

S32 iPodBTComInit(IPOD_DATACOM_FUNC_TABLE* data_com_function, U32 num_devices)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    num_devices = num_devices;

    data_com_function->open = NULL;
    data_com_function->close = NULL;
    data_com_function->abort = NULL;
    data_com_function->write = NULL;
    data_com_function->read = NULL;
    data_com_function->ioctl = NULL;
    data_com_function->property = NULL;
    IAP1_BTP_LOG(DLT_LOG_INFO, "iPodBTComInit called and exited");

    return rc;
}

S32 iPodBTComDeinit(IPOD_DATACOM_FUNC_TABLE* data_com_function)
{
    S32 rc = IPOD_DATACOM_SUCCESS;

    if (data_com_function != NULL)
    {
        data_com_function->open = NULL;
        data_com_function->close = NULL;
        data_com_function->abort = NULL;
        data_com_function->write = NULL;
        data_com_function->read = NULL;
        data_com_function->ioctl = NULL;
        data_com_function->property = NULL;
    }
    IAP1_BTP_LOG(DLT_LOG_INFO, "iPodBTComDeinit called and exited");

    return rc;
}
