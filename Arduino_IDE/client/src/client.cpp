//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
//
// Version:         $Revision: 10346 $,
//                  $Date: 2018-10-29 16:08:18 +0200 (Mon, 29 Oct 2018) $
//                  $Author: gurux01 $
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// More information of Gurux DLMS/COSEM Director: http://www.gurux.org/GXDLMSDirector
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

// Building flags defined in platformio.ini
#if defined NANOboard || defined MEGAboard
#include <Arduino.h>
#elif defined TIVAboard
#include <Energia.h>
#include "rtc_func.h"
#endif

#include <dlmssettings.h>
#include <variant.h>
#include <cosem.h>
#include <client.h>
#include <converters.h>
#include <gxobjects.h>

#include "connection.h"

//Client don't need this.
unsigned char svr_isTarget(
    dlmsSettings *settings,
    unsigned long serverAddress,
    unsigned long clientAddress)
{
  return 0;
}

//Client don't need this.
int svr_connected(
    dlmsServerSettings *settings)
{
  return 0;
}

void time_now(
    gxtime *value)
{
  //Get local time somewhere.
  time_t tm1 = time(NULL);
  struct tm dt = *localtime(&tm1);
  time_init2(value, &dt);
}

gxClock clock1;

// Custom settings for the connection
#define LOGICAL_NAMES 1
#define CLIENT_ADDR 16
#define SERVER_ADDR 1
#define AUTH_DLMS DLMS_AUTHENTICATION_NONE
#define PASS_DLMS NULL

void setup()
{
  #ifdef TIVAboard
  initTime();
  #endif

  bb_init(&frameData);
  //Set frame size.
  bb_capacity(&frameData, 128);
  cl_init(&meterSettings, LOGICAL_NAMES, CLIENT_ADDR, SERVER_ADDR, AUTH_DLMS, PASS_DLMS, DLMS_INTERFACE_TYPE_HDLC);

  cosem_init(&clock1.base, DLMS_OBJECT_TYPE_CLOCK, "0.0.1.0.0.255");

  // start serial port at 9600 bps:
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop()
{
  int ret;
  //Initialize connection.
  ret = com_initializeConnection();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return;
  }
  //Read clock.
  ret = com_read(&clock1.base, 2);
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return;
  }
  com_close();
}
