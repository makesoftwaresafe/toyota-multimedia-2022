
/* **********************  includes  ********************** */

#include "iap2_usb_udc.h"
#include "iap2_usb_role_switch.h"
#include <string.h>
#include <glob.h>



/* **********************  defines   ********************** */



/* **********************  locals    ********************** */

LOCAL S32 getUtbridgeUdc(char* pGlobPath, char* pUtbridgeUdc);
LOCAL BOOL unwiredHubCommonWrite(const char* pPath, const char* pSubPath, const char* pValue, unsigned int writeBytes, BOOL bCheckResult);
LOCAL S32 ctrlBridgeFunc(char* pUtBridgeBusDev, const char* pPortNum);
LOCAL S32 getBridgeUdcDevice(char* pUdevPath, udcParamInfo_t* pUdcParam);

#if defined (IPOD_ARCH_ARM) || defined (IPOD_ARCH_ARM64)
LOCAL S32 getOtgUdcDevice(char* pUdevPath, udcParamInfo_t* pUdcParam);
#else
LOCAL BOOL checkNativeOTG(char* pUdevPath, char *port);
LOCAL BOOL getOtgPciDevice(char* pUdevPath, udcParamInfo_t* pUdcParam);
#endif /* #ifndef IPOD_ARCH_ARM */

LOCAL S32 getUtbridgeUdc(char* pGlobPath, char* pUtbridgeUdc)
{
    S32 ret = -1;
    char* pRetSlash = NULL;
    glob_t found;

    /* find utbridge_udc* path */
    if (0 == (ret = glob(pGlobPath, 0, NULL, &found)) && found.gl_pathc > 0)
    {
        if (found.gl_pathc > 1){
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " more than one VBUS found;     use: %s", found.gl_pathv[0]);
        }

        pRetSlash = (char *)strrchr((const char*)found.gl_pathv[0], STR_SLASH);
        if (pRetSlash != NULL){
            strncpy(pUtbridgeUdc, &pRetSlash[1], strnlen(pRetSlash, STR_MAX_LENGTH) - 1);
            ret = 0;
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  strrchr() failed", __FUNCTION__, __LINE__);
            ret = -1;
        }

        globfree(&found);
    } else if (ret == GLOB_NOMATCH){
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " glob does not found %s  rc = %d", IAP2_VBUS_POWER, ret);
        ret = -1;
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " glob failed with rc = %d", ret);
        ret = -1;
    }

    return ret;
}

LOCAL BOOL unwiredHubCommonWrite(const char* pPath, const char* pSubPath, const char* pValue, unsigned int writeBytes, BOOL bCheckResult)
{
    S32 ret = -1;
    BOOL status = FALSE;
    char valuePath[STR_MAX_LENGTH];
    S32 file = 0;

    ret = snprintf(valuePath, STR_MAX_LENGTH, "%s/%s", pPath, pSubPath);
    if (ret >= 0 && ret < STR_MAX_LENGTH){
        file = open(valuePath, O_RDWR);
        if (file >= 0){
            ret = write(file, pValue, writeBytes);
            if (ret == (int)writeBytes){
                /* successful write */
                if (TRUE == bCheckResult){
                    close(file);
                    file = open(valuePath, O_RDONLY);
                    if (file >= 0){
                        char buffer[writeBytes + 1];
                        memset(&buffer[0], 0, writeBytes + 1);
                        ret = read(file, buffer, sizeof(buffer));
                        if (ret > 0){
                            if (0 == strncmp(buffer, pValue, writeBytes)){
                                /* successful write */
                                status = TRUE;
                            }
                        } else{
                            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  read:  ret = %d | errno: %d %s ",
                                   __FUNCTION__, __LINE__, ret, errno, strerror(errno));
                        }
                        close(file);
                    } else{
                        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  open(%s): %d %s ",
                                __FUNCTION__, __LINE__, valuePath, errno, strerror(errno));
                    }
                } else{
                    status = TRUE;
                }
            } else if (ret < 0){
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write:  ret = %d | errno: %d %s",
                       __FUNCTION__, __LINE__, ret, errno, strerror(errno));
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write not all bytes:  ret = %d | errno: %d %s",
                       __FUNCTION__, __LINE__, ret, errno, strerror(errno));
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  open(%s): %d %s",
                    __FUNCTION__, __LINE__, valuePath, errno, strerror(errno));
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
    }

    return status;
}

/* activate bridge-function
 * write port number into "bridgeport". e.g port 4
 * echo 4 > /sys/bus/usb/devices/2-1.5\:1.0/bridgeport
 */
/* deactivate bridge-function
 * write 0 into "bridgeport".
 * echo 0 > /sys/bus/usb/devices/2-1.5\:1.0/bridgeport
 */
LOCAL S32 ctrlBridgeFunc(char* pUtBridgeBusDev, const char* pPortNum)
{
    S32 ret = -1;
    char bridgePort[STR_MAX_LENGTH] = {0};

    ret = snprintf(bridgePort, STR_MAX_LENGTH, "%s%s", pUtBridgeBusDev, STR_BRIDGE_CONFIG_PORT);
    if (ret >= 0 && ret < STR_MAX_LENGTH){

        if (TRUE == unwiredHubCommonWrite(&bridgePort[0], STR_BRIDGE_PORT, pPortNum, strnlen(pPortNum, STR_MAX_LENGTH), FALSE)){
            ret = 0;
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write utbridge_udc failed", __FUNCTION__, __LINE__);
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
        ret = -1;
    }

    return ret;
}

/* switch power off
 * write port number and 0 into "portpower". e.g port 4
 * echo 4=0 > /sys/bus/usb/devices/2-1.5\:1.0/portpower
 */
/* switch power on
 * write port number and 1 into "portpower". e.g port 4
 * echo 4=1 > /sys/bus/usb/devices/2-1.5\:1.0/portpower
 */
S32 switchBridgePower(char* pUtBridgeBusDev, const char* pPortNum, BOOL bOnOff)
{
    S32 ret = -1;
    char bridgePowerPort[STR_MAX_LENGTH] = {0};
    char bridgePowerPortPath[STR_MAX_LENGTH] = {0};
    char value[5] = {0};


    /* Check for "/portpower"
     * If available:        use older Unwired Hub sysfs attribute.
     * If not available:    use newer Unwired Hub sysfs attribute. */
    ret = snprintf(bridgePowerPortPath, STR_MAX_LENGTH, "%s%s/%s", pUtBridgeBusDev, STR_BRIDGE_CONFIG_PORT, STR_BRIDGE_PORT_POWER);
    if (ret >= 0 && ret < STR_MAX_LENGTH){
        int file = open(&bridgePowerPortPath[0], O_RDWR);
        if (file >= 0){
            /* "/portpower" is  available --> use older sysfs attribute. */
            close(file);

            /* get the utBridge config port. e.g. /sys/bus/usb/devices/2-1.5:1.0 */
            ret = snprintf(bridgePowerPort, STR_MAX_LENGTH, "%s%s", pUtBridgeBusDev, STR_BRIDGE_CONFIG_PORT);
            if (ret >= 0 && ret < STR_MAX_LENGTH){

                if (TRUE == bOnOff){
                    /* bOnOff = TRUE, switch power on*/
                    ret = snprintf(value, 5, "%s%s", pPortNum, STR_BRIDGE_PORT_POWER_ON);
                } else{
                    /* bOnOff = FALSE, switch power ooff*/
                    ret = snprintf(value, 5, "%s%s", pPortNum, STR_BRIDGE_PORT_POWER_OFF);
                }

                if (ret >= 0 && ret < STR_MAX_LENGTH){
                    /* write value into e.g. /sys/bus/usb/devices/2-1.5:1.0/portpower to switch the port power on/off */
                    if (TRUE == unwiredHubCommonWrite(&bridgePowerPort[0], STR_BRIDGE_PORT_POWER, &value[0], strnlen(value, 5), FALSE)){
                        ret = 0;
                    } else{
                        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write value to portpower failed", __FUNCTION__, __LINE__);
                        ret = -1;
                    }
                } else{
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
                    ret = -1;
                }
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
                ret = -1;
            }
        } else if ((0 > file) && (ENOENT == errno)){
            /* "/portpower" is not available --> use newer sysfs attribute. */

            ret = snprintf(bridgePowerPort, STR_MAX_LENGTH, "%s%s/%s%s", pUtBridgeBusDev, ":1.0", STR_PORT, pPortNum);
            if (ret >= 0 && ret < STR_MAX_LENGTH){
                if (TRUE == bOnOff){
                    /* bOnOff = TRUE, switch power on*/
                    ret = snprintf(value, 5, STR_BRIDGE_PORT_POWER_CTRL_ON);
                } else{
                    /* bOnOff = FALSE, switch power ooff*/
                    ret = snprintf(value, 5, STR_BRIDGE_PORT_POWER_CTRL_OFF);
                }

                if (TRUE == unwiredHubCommonWrite(&bridgePowerPort[0], STR_CONTROL, &value[0], strnlen(value, 5), FALSE)){
                    ret = 0;
                } else{
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write value to portX/control failed", __FUNCTION__, __LINE__);
                    ret = -1;
                }
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
                ret = -1;
            }
        } else{
            /* there was another error while open() */
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  open(%s): %d %s",
                __FUNCTION__, __LINE__, &bridgePowerPortPath[0], errno, strerror(errno));
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
        ret = -1;
    }

    return ret;
}

LOCAL S32 getBridgeUdcDevice(char* pUdevPath, udcParamInfo_t* pUdcParam)
{
    S32 ret = 0;
    S32 file = -1;

    size_t strLen = 0;
    size_t strLenToCpy = 0;
    char tmpPath[STR_MAX_LENGTH] = {0};
    char* pRetSlash = NULL;
    char *retDot = NULL;

    char buffer[STR_MAX_LENGTH];

    char utBridge[STR_MAX_LENGTH] = {0};
    char tmpBusDev[STR_MAX_LENGTH] = {0};

    /* get port number */;
    pRetSlash = (char*)strrchr((const char*)pUdevPath, STR_SLASH);
    if (NULL != pRetSlash){
        strLen = strnlen(pRetSlash, STR_MAX_LENGTH);
        pUdcParam->pSysDevicesPortNum = malloc(strLen +1);
        if (NULL != pUdcParam->pSysDevicesPortNum){
            memset(pUdcParam->pSysDevicesPortNum, 0, strLen);
            strncpy(pUdcParam->pSysDevicesPortNum, pRetSlash, strLen);
            pUdcParam->pSysDevicesPortNum[strLen] = STR_NULL_TERMINATED;
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pSysDevicesPortNum", __FUNCTION__, __LINE__);
            ret = -1;
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  strrchr() failed", __FUNCTION__, __LINE__);
        ret = -1;
    }

    if (0 == ret){
        retDot = (char*)strrchr((const char*)pUdcParam->pSysDevicesPortNum, STR_DOT);
        if (NULL != retDot){
            /* get device port number. e.g. 2, 3 or 4 */
            strLen = strnlen(retDot, STR_MAX_LENGTH);
            strLenToCpy = strLen - 1;
            pUdcParam->pDevicePortNum = malloc(strLenToCpy+1);
            if (NULL != pUdcParam->pDevicePortNum){
                strncpy(pUdcParam->pDevicePortNum, &retDot[1], strLenToCpy );
                pUdcParam->pDevicePortNum[strLenToCpy] = STR_NULL_TERMINATED;
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pDevicePortNum", __FUNCTION__, __LINE__);
                ret = -1;
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  strrchr() failed", __FUNCTION__, __LINE__);
            ret = -1;
        }
    }

    if (0 == ret){
        /* get utBridge port. e.g. 1-1 or 2-1 */
        strLenToCpy = (strnlen(pUdcParam->pSysDevicesPortNum, STR_MAX_LENGTH) - strLen);
        pUdcParam->pBridgePortNum = malloc(strLenToCpy+1);
        if (NULL != pUdcParam->pBridgePortNum){
            strncpy(pUdcParam->pBridgePortNum, pUdcParam->pSysDevicesPortNum, strLenToCpy);
            pUdcParam->pBridgePortNum[strLenToCpy] = STR_NULL_TERMINATED;
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pBridgePortNum", __FUNCTION__, __LINE__);
            ret = -1;
        }
    }

    if (0 == ret){
        /* get /sys/bus/usb/devices/"utBridge port" */
        ret = snprintf(tmpBusDev, STR_MAX_LENGTH, "%s%s", STR_SYS_BUS_USB_DEVICES, pUdcParam->pBridgePortNum );
        if (ret >= 0 && ret < STR_MAX_LENGTH){
            memset(tmpPath, 0, STR_MAX_LENGTH);
            /* get /sys/bus/usb/devices/"utBridge port"/product */
            snprintf(tmpPath, STR_MAX_LENGTH, "%s%s/%s", tmpBusDev, STR_BRIDGE_PORT_NUMBER, STR_PRODUCT);
            if (ret >= 0 && ret < STR_MAX_LENGTH){
                /* check if it's a Host to Host Bridge */
                file = open(&tmpPath[0], O_RDONLY);
                if (file >= 0){
                    memset(&buffer[0], 0, STR_MAX_LENGTH);
                    /* read the product name */
                    ret = read(file, buffer, sizeof(buffer));
                    if (ret > 0){
                        /* compare if it is the expected Host to Host Bridge */
                        if (0 == strncmp(&buffer[0], STR_HOST_TO_HOST_BRIDGE, strnlen(STR_HOST_TO_HOST_BRIDGE, STR_MAX_LENGTH))){
                            /* everything is fine */
                            ret = 0;
                        } else{
                            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " it's not the expected  %s", STR_HOST_TO_HOST_BRIDGE);
                            /* it's not the expected Host to Host Bridge */
                            ret = -1;
                        }
                    } else{
                        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  read() fail", __FUNCTION__, __LINE__);
                    }
                    close(file);
                } else{
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  open(%s) failed error %d %s",  __FUNCTION__, __LINE__, &tmpPath[0],errno, strerror(errno));
                }
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
        }
    }

    if (0 == ret){
        /* get path to the utBridge */
        strLen = strnlen(&tmpBusDev[0], STR_MAX_LENGTH);
        pUdcParam->pUtBridgePath = malloc(strLen +1);
        if (NULL != pUdcParam->pUtBridgePath){
            strncpy(pUdcParam->pUtBridgePath, &tmpBusDev[0], strLen);
            pUdcParam->pUtBridgePath[strLen] = STR_NULL_TERMINATED;

        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pUtBridgePath", __FUNCTION__, __LINE__);
            ret = -1;
        }
    }

    if (0 == ret){
        /* get the name of the utbridge_udc. e.g. utbridge_udc.0 */
        memset(tmpPath, 0, STR_MAX_LENGTH);
        ret = snprintf(tmpPath, STR_MAX_LENGTH, "%s%s/%s", pUdcParam->pUtBridgePath, STR_BRIDGE_CONFIG_PORT, STR_UTBRIDGE_UDC);
        if (ret >= 0 && ret < STR_MAX_LENGTH){
            ret = getUtbridgeUdc(&tmpPath[0], &utBridge[0]);
            if (0 == ret){
                /* get udc configuration path */
                strLen = strnlen(&tmpPath[0], STR_MAX_LENGTH);
                pUdcParam->pUtBridgeConfigPath = malloc(strLen +1);
                if (NULL != pUdcParam->pUtBridgeConfigPath){
                    strncpy(pUdcParam->pUtBridgeConfigPath, &tmpPath[0], strLen);
                    pUdcParam->pUtBridgeConfigPath[strLen] = STR_NULL_TERMINATED;
                } else{
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pUtBridgeConfigPath", __FUNCTION__, __LINE__);
                    ret = -1;
                }
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  getUtbridgeUdc() = %d", __FUNCTION__, __LINE__, ret);
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
        }
    }

    if (0 == ret){
        /* get udcDevice of utBridge */
        strLen = strnlen(&utBridge[0], STR_MAX_LENGTH);
        pUdcParam->pUdcDevice = malloc(strLen +1);
        if (NULL != pUdcParam->pUdcDevice){
            strncpy(pUdcParam->pUdcDevice, &utBridge[0], strLen);
            pUdcParam->pUdcDevice[strLen] = STR_NULL_TERMINATED;
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pUdcDevice", __FUNCTION__, __LINE__);
            ret = -1;
        }
    }

    return ret;
}

#ifdef IPOD_ARCH_ARM
LOCAL S32 getOtgUdcDevice(char* pUdevPath, udcParamInfo_t* pUdcParam)
{
    S32 ret = 0;

    size_t strLen = 0;
    size_t strLenToCpy = 0;
    char tmpPath[STR_MAX_LENGTH] = {0};
    char* pRetSlash = NULL;
    char* pRetStr = NULL;

    pRetStr = strstr(pUdevPath, STR_CI_HDRC);
    if (NULL != pRetStr){
        pRetSlash = (char*)strchr((const char*)pRetStr, STR_SLASH);
        if (NULL != pRetSlash){
            strLen = strnlen(pRetSlash, STR_MAX_LENGTH);
            strLenToCpy = (strnlen(pRetStr, STR_MAX_LENGTH) - strLen);
            strncpy(&tmpPath[0], pRetStr, strLenToCpy);
            tmpPath[strLenToCpy] = STR_NULL_TERMINATED;

            strLen = strnlen(&tmpPath[0], STR_MAX_LENGTH);
            pUdcParam->pUdcDevice = malloc(strLen +1);
            if (NULL != pUdcParam->pUdcDevice){
                memset(pUdcParam->pUdcDevice, 0, strLen);
                strncpy(pUdcParam->pUdcDevice, &tmpPath[0], strLen);
                pUdcParam->pUdcDevice[strLen] = STR_NULL_TERMINATED;

                ret = 0;
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pSysDevicesPortNum", __FUNCTION__, __LINE__);
                ret = -1;
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  strrchr() failed", __FUNCTION__, __LINE__);
            ret = -1;
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " %s(line %d)  strstr() failed", __FUNCTION__, __LINE__);
        ret = -1;
    }

    return ret;
}

#elif defined IPOD_ARCH_ARM64
LOCAL S32 getOtgUdcDevice(char* pUdevPath, udcParamInfo_t* pUdcParam)
{
	S32 ret = 0;

	size_t strLen = 0;
	char tmpPath[STR_MAX_LENGTH] = {0};
	char* pRetStr = NULL;

    IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " %s pUdevPath", pUdevPath);

	pRetStr = strstr(pUdevPath, "ee080");
	if (NULL != pRetStr)
	{
		strncpy(&tmpPath[0], STR_NATIVE_OTG, STR_MAX_LENGTH);
		strLen = strnlen(&tmpPath[0], STR_MAX_LENGTH);
		pUdcParam->pUdcDevice = malloc(strLen +1);
		if (NULL != pUdcParam->pUdcDevice)
		{
			memset(pUdcParam->pUdcDevice, 0, strLen);
			strncpy(pUdcParam->pUdcDevice, &tmpPath[0], strLen);
			pUdcParam->pUdcDevice[strLen] = STR_NULL_TERMINATED;

			ret = 0;
		}
		else
		{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pSysDevicesPortNum", __FUNCTION__, __LINE__);
            ret = -1;
		}
	}
	else
	{
		IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " %s(line %d)  strstr() failed", __FUNCTION__, __LINE__);
		ret = -1;
	}

	return ret;
}

#else

LOCAL BOOL checkNativeOTG(char* pUdevPath, char *port)
{
    BOOL rc = FALSE;
    char *chkPath = NULL;
    
    chkPath = (char *)strrchr(pUdevPath, STR_SLASH);
    if(chkPath != NULL)
    {
        /* check USB port number to detect OTG port */
        if(strncmp(chkPath + 1, port, strlen(port)) == 0)
        {
            rc = TRUE;
        }
        else
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "Plugged port isn't OTG port.(path = %s)", pUdevPath);
        }
    }

    return rc;
}

LOCAL S32 getOtgPciDevice(char* pUdevPath, udcParamInfo_t* pUdcParam)
{
    S32 ret = -1;
    char* pTmpPath = NULL;
    char* pRetStr = NULL;
    glob_t otgPath;
    int rc = 0;

    pTmpPath = calloc(STR_MAX_LENGTH, sizeof(char));
    if(pTmpPath != NULL)
    {
        pRetStr = strstr(pUdevPath, "/usb");
        if (NULL != pRetStr){
            memcpy(pTmpPath, pUdevPath, (pRetStr - (pUdevPath + 1)));
            pTmpPath[strnlen(pTmpPath, STR_MAX_LENGTH)] = '1';   /* change from host to gadget  */
        }
        pUdcParam->pUdcDevice = calloc(STR_MAX_LENGTH, sizeof(char));
        if(pUdcParam->pUdcDevice != NULL)
        {
            /* find pci device on sysfs */
            strncat(pTmpPath, STR_DWC3_UDC, STR_MAX_LENGTH); /* create the file name of UDC */

            /* check UDC path */
            rc = glob(pTmpPath, 0, NULL, &otgPath);
            if(rc != GLOB_NOMATCH)
            {
                /* detect top of udc name */
                pRetStr = strstr(otgPath.gl_pathv[0], STR_DWC);
                if(pRetStr != NULL)
                {
                    strncpy(pUdcParam->pUdcDevice, pRetStr, STR_MAX_LENGTH);
                    if(checkNativeOTG(pUdevPath, STR_OTG_PORT))
                    {
                        ret = 0;
                    }
                }
            }
            if(ret == -1)
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, " %s(line %d)  UDC path not found!", __FUNCTION__, __LINE__);
            }
            globfree(&otgPath);
        }
        free(pTmpPath);
    }

    return ret;
}
#endif /* ifndef IPOD_ARCH_ARM */

/* **********************  functions ********************** */


void freeUdcParam(udcParamInfo_t* pUdcParam)
{
    if(NULL != pUdcParam->pSysDevicesPortNum){
        free(pUdcParam->pSysDevicesPortNum);
        pUdcParam->pSysDevicesPortNum = NULL;
    }
    if(NULL != pUdcParam->pBridgePortNum){
        free(pUdcParam->pBridgePortNum);
        pUdcParam->pBridgePortNum = NULL;
    }
    if(NULL != pUdcParam->pDevicePortNum){
        free(pUdcParam->pDevicePortNum);
        pUdcParam->pDevicePortNum = NULL;
    }
    if(NULL != pUdcParam->pUtBridgeConfigPath){
        free(pUdcParam->pUtBridgeConfigPath);
        pUdcParam->pUtBridgeConfigPath = NULL;
    }
    if(NULL != pUdcParam->pUtBridgePath){
        free(pUdcParam->pUtBridgePath);
        pUdcParam->pUtBridgePath = NULL;
    }
    if(NULL != pUdcParam->pUdevPath){
        free(pUdcParam->pUdevPath);
        pUdcParam->pUdevPath = NULL;
    }
    if(NULL != pUdcParam->pUdcDevice){
        free(pUdcParam->pUdcDevice);
        pUdcParam->pUdcDevice = NULL;
    }
}

usbConnectStateType_t getConnectStateType(char* pUdevPath, udcParamInfo_t* pUdcParam)
{
    usbConnectStateType_t connectType = NOT_CONNECTED;
    S32 ret = 0;
    S32 rc  = -1;
    size_t strLen = 0;
    size_t strLenToCpy = 0;
    char tmpPath[STR_MAX_LENGTH] = {0};
    char* pRetSlash = NULL;

    char findUsbDevice[STR_MAX_LENGTH] = {0};

    if ((NULL == pUdevPath) || (NULL == pUdcParam)){
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  input param are NULL", __FUNCTION__, __LINE__);
        return CON_ERROR;
    }

    /* store udev path */
    strLen = strnlen((const char*)pUdevPath, STR_MAX_LENGTH);
    pUdcParam->pUdevPath = malloc(strLen +1);
    if (NULL != pUdcParam->pUdevPath){
        memset(pUdcParam->pUdevPath, 0, strLen);
        strncpy(pUdcParam->pUdevPath, pUdevPath, strLen);
        pUdcParam->pUdevPath[strLen] = STR_NULL_TERMINATED;
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  no memory for pUdevPath", __FUNCTION__, __LINE__);
        return CON_ERROR;
    }

    /* get upper USB device */
    pRetSlash = (char*)strrchr((const char*)pUdevPath, STR_SLASH);
    if (NULL != pRetSlash){
        strLen = strnlen(pRetSlash, STR_MAX_LENGTH);
        strLenToCpy = strnlen((const char*)pUdevPath, STR_MAX_LENGTH) - strLen;
        strncpy(tmpPath, (const char*)pUdevPath, strLenToCpy);

        /* get product name of upper USB device */
        ret = snprintf(findUsbDevice, STR_MAX_LENGTH, "%s/%s", tmpPath, STR_PRODUCT);
        if (ret >= 0 && ret < STR_MAX_LENGTH){
            /* check if it's an Unwired Hub */
            int file = open(findUsbDevice, O_RDONLY);
            if (file >= 0){
                char buffer[STR_MAX_LENGTH]; /* to capture longer entries */
                memset(&buffer[0], 0, STR_MAX_LENGTH);
                rc = read(file, buffer, sizeof(buffer));
                if (rc > 0){
                    if (0 == strncmp(&buffer[0], STR_UNWIRED_HUB, strnlen(STR_UNWIRED_HUB, STR_MAX_LENGTH))){
                        connectType = UNWIRED_HUB_CONNECTED;

                        /* get udc device from utbridge */
                        ret = getBridgeUdcDevice(pUdevPath, pUdcParam);
                        if (0 == ret){
                            pUdcParam->type = UNWIRED_HUB_CONNECTED;
                        }
                        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " %s(line %d)  getBridgeUdcDevice() = %d | pUdcDevice:  %s",
                                __FUNCTION__, __LINE__, ret, pUdcParam->pUdcDevice);
                    } else{

                        /* get udc device from OTG port */
#if defined (IPOD_ARCH_ARM) || defined (IPOD_ARCH_ARM64)
                        ret = getOtgUdcDevice(pUdevPath, pUdcParam);
#else
                        ret = getOtgPciDevice(pUdevPath, pUdcParam);
#endif /* #ifdef IPOD_ARCH_ARM */
                        if (0 == ret && (0 == strncmp(pUdcParam->pUdcDevice,STR_NATIVE_OTG,strnlen(STR_NATIVE_OTG, STR_MAX_LENGTH)))){
                            connectType = OTG_CONNECTED;
                            pUdcParam->type = OTG_CONNECTED;
                        }
                        IAP2USBROLESWITCHDLTLOG(DLT_LOG_INFO, " %s(line %d)  getOtgUdcDevice() = %d | pUdcDevice:  %s",
                                __FUNCTION__, __LINE__, ret, pUdcParam->pUdcDevice);
                    }
                } else{
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  read() fail", __FUNCTION__, __LINE__);
                }
                close(file);
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  open(%s) failed error %d %s",  __FUNCTION__, __LINE__, findUsbDevice,errno, strerror(errno));
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  snprintf() failed", __FUNCTION__, __LINE__);
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  strrchr() failed", __FUNCTION__, __LINE__);
        ret = -1;
    }

    /* indicate that there was an error */
    if (0 > ret){
        connectType = NOT_CONNECTED;
    }

    return connectType;
}

BOOL setUdcForce(char* pUdcDevice)
{
    BOOL status = FALSE;

    /* set the udc devive e.g. utbridge_udc.* or ci_hdrc.* to force the role switch */
    status = unwiredHubCommonWrite(STR_SYS_CLASS_UDC, STR_UDC_FORCE, pUdcDevice, strnlen(pUdcDevice,STR_MAX_LENGTH), TRUE);
    if (TRUE != status){
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write %s to %s failed",
                __FUNCTION__, __LINE__, pUdcDevice, STR_UDC_FORCE);
    }

    return status;
}

S32 udcSwitchToHostMode(udcParamInfo_t* pUdcParam)
{
    S32 ret = 0;

    /* check if the parameters already there */
    if ((NULL != pUdcParam->pDevicePortNum)
        && (NULL != pUdcParam->pUtBridgePath)
        && (NULL != pUdcParam->pUdcDevice))
    {
        /* set the utbridge_udc to force the role switch */
        if (TRUE == setUdcForce(pUdcParam->pUdcDevice)){

            /* activate bridge-function */
            ret = ctrlBridgeFunc(pUdcParam->pUtBridgePath, pUdcParam->pDevicePortNum);
            if (0 == ret){
                /* switch bridge power on */
                ret = switchBridgePower(pUdcParam->pUtBridgePath, pUdcParam->pDevicePortNum, TRUE);
                if (0 > ret){
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  switchBridgePower() failed ", __FUNCTION__, __LINE__);
                }
            } else{
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  ctrlBridgeFunc() failed ", __FUNCTION__, __LINE__);
            }
        } else{
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  setUdcForce() failed ", __FUNCTION__, __LINE__);
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  input param are NULL ", __FUNCTION__, __LINE__);
        ret = -1;
    }

    return ret;
}

S32 udcSwitchToDeviceMode(udcParamInfo_t* pUdcParam)
{
    S32 ret = -1;
    char tmpPath[STR_MAX_LENGTH] = {0};

    /* check if the parameters already there */
    if ((NULL != pUdcParam->pDevicePortNum)
        && (NULL != pUdcParam->pUtBridgePath)
        && (NULL != pUdcParam->pUdcDevice))
    {

        /* set the utbridge_udc to force the role switch */
        if (TRUE == unwiredHubCommonWrite(STR_SYS_CLASS_UDC, STR_UDC_FORCE, pUdcParam->pUdcDevice, strnlen(pUdcParam->pUdcDevice, STR_MAX_LENGTH), TRUE)){
                /* switch bridge power off */
                ret = switchBridgePower(pUdcParam->pUtBridgePath, pUdcParam->pDevicePortNum, FALSE);
            if (0 == ret){
                /* deactivate bridge-function */
                memset(tmpPath, 0, STR_MAX_LENGTH);
                tmpPath[0] = '0';
                ret = ctrlBridgeFunc(pUdcParam->pUtBridgePath, &tmpPath[0]);
            }

            /* sleep may not necessary */
            sleep(2);

            /* switch bridge power on */
            ret = switchBridgePower(pUdcParam->pUtBridgePath, pUdcParam->pDevicePortNum, TRUE);

        } else{
            ret = -1;
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  write utbridge_udc failed ", __FUNCTION__, __LINE__);
        }
    } else{
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, " %s(line %d)  input param are NULL ", __FUNCTION__, __LINE__);
        ret = -1;
    }

    return ret;
}

