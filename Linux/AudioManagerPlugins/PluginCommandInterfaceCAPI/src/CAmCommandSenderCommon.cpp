/**
 *  Copyright (c) 2012 BMW
 *
 *  \author Aleksandar Donchev, aleksander.donchev@partner.bmw.de BMW 2013
 *
 *  \copyright
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
 *  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 */


#include "CAmCommandSenderCommon.h"

/**
 * Utility functions
 */
void CAmConvertAvailablility(const am_Availability_s & amAvailability, am_types::am_Availability_s & result)
{
	result.setAvailability(CAmConvert2CAPIType(amAvailability.availability));
	result.setAvailabilityReason(amAvailability.availabilityReason);
}

void CAmConvertMainSoundProperty(const am_MainSoundProperty_s & amMainSoundProperty, am_types::am_MainSoundProperty_s & result)
{
	result.setType(amMainSoundProperty.type);
	result.setValue(amMainSoundProperty.value);
}

void CAmConvertSystemProperty(const am_SystemProperty_s & amSystemProperty, am_types::am_SystemProperty_s & result)
{
	result.setType(amSystemProperty.type);
	result.setValue(amSystemProperty.value);
}

am_types::am_Availability_e CAmConvert2CAPIType(const am_Availability_e & availability)
{
	am_types::am_Availability_e result((am_types::am_Availability_e::Literal)availability);
	return result;
}

am_types::am_MuteState_e CAmConvert2CAPIType(const am_MuteState_e & muteState)
{
	am_types::am_MuteState_e result((am_types::am_MuteState_e::Literal)muteState);
	return result;
}

am_MuteState_e CAmConvertFromCAPIType(const am_types::am_MuteState_e & muteState)
{
	int32_t state = (int32_t)muteState;
	return (am_MuteState_e)state;
}

am_types::am_ConnectionState_e CAmConvert2CAPIType(const am_ConnectionState_e & connectionState)
{
	am_types::am_ConnectionState_e result((am_types::am_ConnectionState_e::Literal)connectionState);
	return result;
}

am_ConnectionState_e CAmConvertFromCAPIType(const am_types::am_ConnectionState_e & connectionState)
{
	int32_t state = (int32_t)connectionState;
	return (am_ConnectionState_e)state;
}

am_types::am_NotificationStatus_e CAmConvert2CAPIType(const am_NotificationStatus_e & notificationStatus)
{
	am_types::am_NotificationStatus_e result((am_types::am_NotificationStatus_e::Literal)notificationStatus);
	return result;
}

am_NotificationStatus_e CAmConvertFromCAPIType(const am_types::am_NotificationStatus_e & notificationStatus)
{
	int32_t state = (int32_t)notificationStatus;
	return (am_NotificationStatus_e)state;
}

am_types::am_Error_e CAmConvert2CAPIType(const am_Error_e & error)
{
	am_types::am_Error_e result((am_types::am_Error_e::Literal)error);
	return result;
}

am_Error_e CAmConvertFromCAPIType(const am_types::am_Error_e & error)
{
	int32_t e = (int32_t)error;
	return (am_Error_e)e;
}

