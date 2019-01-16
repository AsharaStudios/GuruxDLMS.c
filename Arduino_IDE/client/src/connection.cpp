#include <dlmssettings.h>
#include <variant.h>
#include <cosem.h>
#include <client.h>
#include <converters.h>
#include <gxobjects.h>

// Building flags defined in platformio.ini
#if defined NANOboard || defined MEGAboard
#include <Arduino.h>
#elif defined TIVAboard
#include <Energia.h>
#endif

#include "connection.h"

#ifdef DEBUG_MSG
#define BUF_SIZE 100
char buf[BUF_SIZE];

#define debug_printf(str, ...)      \
  sprintf(buf, str, ##__VA_ARGS__); \
  DEBUG_SERIAL.print(buf);
#endif

gxByteBuffer frameData = {};
dlmsSettings meterSettings = {};

//Initialize connection to the meter.
int com_initializeConnection()
{

  int ret = DLMS_ERROR_CODE_OK;
  message messages;
  gxReplyData reply;
#ifdef DEBUG_MSG
  DEBUG_SERIAL.println("Initializing connection...");
#endif
  mes_init(&messages);
  reply_init(&reply);

  //Get meter's send and receive buffers size.
  if ((ret = cl_snrmRequest(&meterSettings, &messages)) != 0 ||
      (ret = com_readDataBlock(&messages, &reply)) != 0 ||
      (ret = cl_parseUAResponse(&meterSettings, &reply.data)) != 0)
  {
    mes_clear(&messages);
    reply_clear(&reply);
#ifdef DEBUG_MSG
    DEBUG_SERIAL.print("SNRMRequest failed ");
    DEBUG_SERIAL.println(err_toString(ret));
#endif
    return ret;
  }
  mes_clear(&messages);
  reply_clear(&reply);
  if ((ret = cl_aarqRequest(&meterSettings, &messages)) != 0 ||
      (ret = com_readDataBlock(&messages, &reply)) != 0 ||
      (ret = cl_parseAAREResponse(&meterSettings, &reply.data)) != 0)
  {
    mes_clear(&messages);
    reply_clear(&reply);
#ifdef DEBUG_MSG
    DEBUG_SERIAL.print("AARQRequest failed ");
    DEBUG_SERIAL.println(err_toString(ret));
#endif
    if (ret == DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED)
    {
#ifdef DEBUG_MSG
      DEBUG_SERIAL.print("Use Logical Name referencing is wrong.Change it return ret; ");
      DEBUG_SERIAL.println(err_toString(ret));
#endif
      return ret;
    }
#ifdef DEBUG_MSG
    DEBUG_SERIAL.print("AARQRequest failed ");
    DEBUG_SERIAL.println(err_toString(ret));
#endif
    return ret;
  }
  mes_clear(&messages);
  reply_clear(&reply);
  // Get challenge Is HLS authentication is used.
  if (meterSettings.authentication > DLMS_AUTHENTICATION_LOW)
  {
    if ((ret = cl_getApplicationAssociationRequest(&meterSettings, &messages)) != 0 ||
        (ret = com_readDataBlock(&messages, &reply)) != 0 ||
        (ret = cl_parseApplicationAssociationResponse(&meterSettings, &reply.data)) != 0)
    {
      mes_clear(&messages);
      reply_clear(&reply);
#ifdef DEBUG_MSG
      DEBUG_SERIAL.print("Get Application Association failed ");
      DEBUG_SERIAL.println(err_toString(ret));
#endif
      return ret;
    }
    mes_clear(&messages);
    reply_clear(&reply);
  }
  return DLMS_ERROR_CODE_OK;
}

//Close connection to the meter.
int com_close()
{
  int ret = DLMS_ERROR_CODE_OK;
  gxReplyData reply;
  message msg;
  reply_init(&reply);
  mes_init(&msg);
  if ((ret = cl_releaseRequest(&meterSettings, &msg)) != 0 ||
      (ret = com_readDataBlock(&msg, &reply)) != 0)
  {
//Show error but continue close.
#ifdef DEBUG_MSG
    DEBUG_SERIAL.print("release Request failed ");
    DEBUG_SERIAL.println(err_toString(ret));
#endif
  }
  reply_clear(&reply);
  mes_clear(&msg);

  if ((ret = cl_disconnectRequest(&meterSettings, &msg)) != 0 ||
      (ret = com_readDataBlock(&msg, &reply)) != 0)
  {
//Show error but continue close.
#ifdef DEBUG_MSG
    DEBUG_SERIAL.print("Disconnect Request failed ");
    DEBUG_SERIAL.println(err_toString(ret));
#endif
  }
  reply_clear(&reply);
  mes_clear(&msg);
  //cl_clear(&meterSettings); // comentado porque borra la contraseña
  return ret;
}

int com_readSerialPort(
    unsigned char eop)
{
  //Read reply data.
  int pos;
  unsigned short available;
  unsigned char eopFound = 0;
  unsigned short lastReadIndex = 0;
  int p = 0;
  frameData.size = 0;
  frameData.position = 0;
  do
  {
    available = Serial1.available();
    if (available != 0)
    {
      MAIN_SERIAL.readBytes((char *)(frameData.data + frameData.size), available); //TODO: desacoplar Arduino
      AUX_SERIAL.write((const uint8_t *)(frameData.data + frameData.size), available);
      frameData.size += available;
      //Search eop.
      if (frameData.size > 5)
      {
        //Some optical strobes can return extra bytes.
        for (pos = frameData.size - 1; pos != lastReadIndex; --pos)
        {
          if (frameData.data[pos] == eop)
          {
            eopFound = 1;
            break;
          }
        }
        lastReadIndex = pos;
      }
    }
    else if (frameData.size > 0)
    {
      delayMicroseconds(9000);
    }
  } while (eopFound == 0);
  return DLMS_ERROR_CODE_OK;
}

// Read DLMS Data frame from the device.
int readDLMSPacket(
    gxByteBuffer *data,
    gxReplyData *reply)
{
  int index = 0, ret = DLMS_ERROR_CODE_OK;
  if (data->size == 0)
  {
    return DLMS_ERROR_CODE_OK;
  }
  reply->complete = 0;
  frameData.size = 0;
  frameData.position = 0;
  //Send data.
  Serial1.write(data->data, data->size); //TODO: desacoplar Arduino
  Serial.write(data->data, data->size);
  //Loop until packet is complete.
  do
  {
    if (com_readSerialPort(0x7E) != 0)
    {
      return DLMS_ERROR_CODE_SEND_FAILED;
    }
    ret = cl_getData(&meterSettings, &frameData, reply);
    if (ret != 0 && ret != DLMS_ERROR_CODE_FALSE)
    {
      break;
    }
  } while (reply->complete == 0);
  return ret;
}

int com_readDataBlock(
    message *messages,
    gxReplyData *reply)
{
  gxByteBuffer rr;
  int pos, ret = DLMS_ERROR_CODE_OK;
  //If there is no data to send.
  if (messages->size == 0)
  {
    return DLMS_ERROR_CODE_OK;
  }
  bb_init(&rr);
  //Send data.
  for (pos = 0; pos != messages->size; ++pos)
  {
    //Send data.
    if ((ret = readDLMSPacket(messages->data[pos], reply)) != DLMS_ERROR_CODE_OK)
    {
      return ret;
    }
    //Check is there errors or more data from server
    while (reply_isMoreData(reply))
    {
      if ((ret = cl_receiverReady(&meterSettings, reply->moreData, &rr)) != DLMS_ERROR_CODE_OK)
      {
        bb_clear(&rr);
        return ret;
      }
      if ((ret = readDLMSPacket(&rr, reply)) != DLMS_ERROR_CODE_OK)
      {
        bb_clear(&rr);
        return ret;
      }
      bb_clear(&rr);
    }
  }
  return ret;
}

//Report error on output;
void com_reportError(const char *description,
                     gxObject *object,
                     unsigned char attributeOrdinal, int ret)
{
  char ln[25];
  char type[30];
#ifdef DEBUG_MSG
#ifndef GX_DLMS_MICROCONTROLLER
  hlp_getLogicalNameToString(object->logicalName, ln);
  obj_typeToString((DLMS_OBJECT_TYPE)object->objectType, type);
#endif
  debug_printf("%s %s %s:%d %s\r\n", description, type, ln, attributeOrdinal, err_toString(ret));
#endif
}

//Get Association view.
int com_getAssociationView()
{
  int ret;
  message data;
  gxReplyData reply;
  mes_init(&data);
  reply_init(&reply);
  if ((ret = cl_getObjectsRequest(&meterSettings, &data)) != 0 ||
      (ret = com_readDataBlock(&data, &reply)) != 0 ||
      (ret = cl_parseObjects(&meterSettings, &reply.data)) != 0)
  {
  }
  mes_clear(&data);
  reply_clear(&reply);
#ifdef DEBUG_MSG
  DEBUG_SERIAL.print("GetObjects failed ");
  DEBUG_SERIAL.println(err_toString(ret));
#endif
  return ret;
}

//Read object.
int com_read(
    gxObject *object,
    unsigned char attributeOrdinal)
{
  int ret;
  message data;
  gxReplyData reply;
  mes_init(&data);
  reply_init(&reply);
  if ((ret = cl_read(&meterSettings, object, attributeOrdinal, &data)) != 0 ||
      (ret = com_readDataBlock(&data, &reply)) != 0 ||
      (ret = cl_updateValue(&meterSettings, object, attributeOrdinal, &reply.dataValue)) != 0)
  {
    com_reportError("ReadObject failed", object, attributeOrdinal, ret);
  }
  mes_clear(&data);
  reply_clear(&reply);
  return ret;
}

int com_write(
    gxObject *object,
    unsigned char attributeOrdinal)
{
  int ret;
  message data;
  gxReplyData reply;
  mes_init(&data);
  reply_init(&reply);
  if ((ret = cl_write(&meterSettings, object, attributeOrdinal, &data)) != 0 ||
      (ret = com_readDataBlock(&data, &reply)) != 0)
  {
    com_reportError("Write failed", object, attributeOrdinal, ret);
  }
  mes_clear(&data);
  reply_clear(&reply);
  return ret;
}

int com_method(
    gxObject *object,
    unsigned char attributeOrdinal,
    dlmsVARIANT *params)
{
  int ret;
  message messages;
  gxReplyData reply;
  mes_init(&messages);
  reply_init(&reply);
  if ((ret = cl_method(&meterSettings, object, attributeOrdinal, params, &messages)) != 0 ||
      (ret = com_readDataBlock(&messages, &reply)) != 0)
  {
#ifdef DEBUG_MSG
#ifndef GX_DLMS_MICROCONTROLLER
    printf("Method failed %s\r\n", hlp_getErrorMessage(ret));
#endif
#endif
  }
  mes_clear(&messages);
  reply_clear(&reply);
  return ret;
}

//Read objects.
int com_readList(
    gxArray *list)
{
  int pos, ret = DLMS_ERROR_CODE_OK;
  gxByteBuffer bb, rr;
  message messages;
  gxReplyData reply;
  if (list->size != 0)
  {
    mes_init(&messages);
    if ((ret = cl_readList(&meterSettings, list, &messages)) != 0)
    {
#ifdef DEBUG_MSG
#ifndef GX_DLMS_MICROCONTROLLER
      debug_printf("ReadList failed %s\r\n", hlp_getErrorMessage(ret));
#endif
#endif
    }
    else
    {
      reply_init(&reply);
      bb_init(&rr);
      bb_init(&bb);
      //Send data.
      for (pos = 0; pos != messages.size; ++pos)
      {
        //Send data.
        reply_clear(&reply);
        if ((ret = readDLMSPacket(messages.data[pos], &reply)) != DLMS_ERROR_CODE_OK)
        {
          break;
        }
        //Check is there errors or more data from server
        while (reply_isMoreData(&reply))
        {
          if ((ret = cl_receiverReady(&meterSettings, reply.moreData, &rr)) != DLMS_ERROR_CODE_OK ||
              (ret = readDLMSPacket(&rr, &reply)) != DLMS_ERROR_CODE_OK)
          {
            break;
          }
          bb_clear(&rr);
        }
        bb_set2(&bb, &reply.data, reply.data.position, -1);
      }
      if (ret == 0)
      {
        ret = cl_updateValues(&meterSettings, list, &bb);
      }
      bb_clear(&bb);
      bb_clear(&rr);
      reply_clear(&reply);
    }
    mes_clear(&messages);
  }
  return ret;
}

int com_readRowsByEntry(
    gxProfileGeneric *object,
    unsigned long index,
    unsigned long count)
{
  int ret;
  message data;
  gxReplyData reply;
  mes_init(&data);
  reply_init(&reply);
  if ((ret = cl_readRowsByEntry(&meterSettings, object, index, count, &data)) != 0 ||
      (ret = com_readDataBlock(&data, &reply)) != 0 ||
      (ret = cl_updateValue(&meterSettings, (gxObject *)object, 2, &reply.dataValue)) != 0)
  {
#ifdef DEBUG_MSG
#ifndef GX_DLMS_MICROCONTROLLER
    debug_printf("ReadObject failed %s\r\n", hlp_getErrorMessage(ret));
#endif
#endif
  }
  mes_clear(&data);
  reply_clear(&reply);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////
int com_readRowsByRange(
    gxProfileGeneric *object,
    struct tm *start,
    struct tm *end)
{
  int ret;
  message data;
  gxReplyData reply;
  mes_init(&data);
  reply_init(&reply);
  if ((ret = cl_readRowsByRange(&meterSettings, object, start, end, &data)) != 0 ||
      (ret = com_readDataBlock(&data, &reply)) != 0 ||
      (ret = cl_updateValue(&meterSettings, (gxObject *)object, 2, &reply.dataValue)) != 0)
  {
#ifdef DEBUG_MSG
#ifndef GX_DLMS_MICROCONTROLLER
    debug_printf("ReadObject failed %s\r\n", hlp_getErrorMessage(ret));
#endif
#endif
  }
  mes_clear(&data);
  reply_clear(&reply);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////
//Read scalers and units. They are static so they are read only once.
int com_readScalerAndUnits()
{
  gxObject *obj;
  int ret, pos;
  objectArray objects;
  gxArray list;
  gxObject *object;
  DLMS_OBJECT_TYPE types[] = {DLMS_OBJECT_TYPE_EXTENDED_REGISTER, DLMS_OBJECT_TYPE_REGISTER, DLMS_OBJECT_TYPE_DEMAND_REGISTER};
  oa_init(&objects);
  //Find registers and demand registers and read them.
  ret = oa_getObjects2(&meterSettings.objects, types, 3, &objects);
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  if ((meterSettings.negotiatedConformance & DLMS_CONFORMANCE_MULTIPLE_REFERENCES) != 0)
  {
    arr_init(&list);
    //Try to read with list first. All meters do not support it.
    for (pos = 0; pos != meterSettings.objects.size; ++pos)
    {
      ret = oa_getByIndex(&meterSettings.objects, pos, &obj);
      if (ret != DLMS_ERROR_CODE_OK)
      {
        oa_empty(&objects);
        arr_clear(&list);
        return ret;
      }
      if (obj->objectType == DLMS_OBJECT_TYPE_REGISTER ||
          obj->objectType == DLMS_OBJECT_TYPE_EXTENDED_REGISTER)
      {
        arr_push(&list, key_init(obj, (void *)3));
      }
      else if (obj->objectType == DLMS_OBJECT_TYPE_DEMAND_REGISTER)
      {
        arr_push(&list, key_init(obj, (void *)4));
      }
    }
    ret = com_readList(&list);
    arr_clear(&list);
  }
  //If read list failed read items one by one.
  if (ret != 0)
  {
    for (pos = 0; pos != objects.size; ++pos)
    {
      ret = oa_getByIndex(&objects, pos, &object);
      if (ret != DLMS_ERROR_CODE_OK)
      {
        oa_empty(&objects);
        return ret;
      }
      ret = com_read(object, object->objectType == DLMS_OBJECT_TYPE_DEMAND_REGISTER ? 4 : 3);
      if (ret != DLMS_ERROR_CODE_OK)
      {
        oa_empty(&objects);
        return ret;
      }
    }
  }
  //Do not clear objects list because it will free also objects from association view list.
  oa_empty(&objects);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////
//Read profile generic columns. They are static so they are read only once.
int com_readProfileGenericColumns()
{
  int ret, pos;
  objectArray objects;
  gxObject *object;
  oa_init(&objects);
  ret = oa_getObjects(&meterSettings.objects, DLMS_OBJECT_TYPE_PROFILE_GENERIC, &objects);
  if (ret != DLMS_ERROR_CODE_OK)
  {
    oa_empty(&objects);
    return ret;
  }
  for (pos = 0; pos != objects.size; ++pos)
  {
    ret = oa_getByIndex(&objects, pos, &object);
    if (ret != DLMS_ERROR_CODE_OK)
    {
      break;
    }
    ret = com_read(object, 3);
    if (ret != DLMS_ERROR_CODE_OK)
    {
      break;
    }
  }
  //Do not clear objects list because it will free also objects from association view list.
  oa_empty(&objects);
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////
//Read profile generics rows.
int com_readProfileGenerics()
{
  gxtime startTime, endTime;
  int ret, pos;
  char str[50];
  char ln[25];
  char *data = NULL;
  gxByteBuffer ba;
  objectArray objects;
  gxProfileGeneric *pg;
  oa_init(&objects);
  ret = oa_getObjects(&meterSettings.objects, DLMS_OBJECT_TYPE_PROFILE_GENERIC, &objects);
  if (ret != DLMS_ERROR_CODE_OK)
  {
    //Do not clear objects list because it will free also objects from association view list.
    oa_empty(&objects);
    return ret;
  }
  bb_init(&ba);
  for (pos = 0; pos != objects.size; ++pos)
  {
    ret = oa_getByIndex(&objects, pos, (gxObject **)&pg);
    if (ret != DLMS_ERROR_CODE_OK)
    {
      //Do not clear objects list because it will free also objects from association view list.
      oa_empty(&objects);
      return ret;
    }
    //Read entries in use.
    ret = com_read((gxObject *)pg, 7);
    if (ret != DLMS_ERROR_CODE_OK)
    {
#ifdef DEBUG_MSG
      debug_printf("Failed to read object %s %s attribute index %d\r\n", str, ln, 7);
#endif
      //Do not clear objects list because it will free also objects from association view list.
      oa_empty(&objects);
      return ret;
    }
    //Read entries.
    ret = com_read((gxObject *)pg, 8);
    if (ret != DLMS_ERROR_CODE_OK)
    {
#ifdef DEBUG_MSG
      debug_printf("Failed to read object %s %s attribute index %d\r\n", str, ln, 8);
#endif
      //Do not clear objects list because it will free also objects from association view list.
      oa_empty(&objects);
      return ret;
    }
#ifdef DEBUG_MSG
    debug_printf("Entries: %ld/%ld\r\n", pg->entriesInUse, pg->profileEntries);
#endif
    //If there are no columns or rows.
    if (pg->entriesInUse == 0 || pg->captureObjects.size == 0)
    {
      continue;
    }
    //Read first row from Profile Generic.
    ret = com_readRowsByEntry(pg, 1, 1);
    //Read last day from Profile Generic.
    time_now(&startTime);
    endTime = startTime;
    time_clearTime(&startTime);
    ret = com_readRowsByRange(pg, &startTime.value, &endTime.value);
  }
  //Do not clear objects list because it will free also objects from association view list.
  oa_empty(&objects);
  return ret;
}

int com_readValue(gxObject *object, unsigned char index)
{
  int ret;
  char *data = NULL;
  ret = com_read(object, index);
  return ret;
}

// This function reads ALL objects that meter have excluded profile generic objects.
// It will loop all object's attributes.
int com_readValues()
{
  gxByteBuffer attributes;
  unsigned char ch;
  char *data = NULL;
  gxObject *object;
  unsigned long index;
  int ret, pos;
  bb_init(&attributes);

  for (pos = 0; pos != meterSettings.objects.size; ++pos)
  {
    ret = oa_getByIndex(&meterSettings.objects, pos, &object);
    if (ret != DLMS_ERROR_CODE_OK)
    {
      bb_clear(&attributes);
      return ret;
    }
    ///////////////////////////////////////////////////////////////////////////////////
    // Profile generics are read later because they are special cases.
    // (There might be so lots of data and we so not want waste time to read all the data.)
    if (object->objectType == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
    {
      continue;
    }
    ret = obj_getAttributeIndexToRead(object, &attributes);
    if (ret != DLMS_ERROR_CODE_OK)
    {
      bb_clear(&attributes);
      return ret;
    }
    for (index = 0; index < attributes.size; ++index)
    {
      ret = bb_getUInt8ByIndex(&attributes, index, &ch);
      if (ret != DLMS_ERROR_CODE_OK)
      {
        bb_clear(&attributes);
        return ret;
      }
      ret = com_read(object, ch);
      if (ret != DLMS_ERROR_CODE_OK)
      {
        //Return error if not DLMS error.
        if (ret != DLMS_ERROR_CODE_READ_WRITE_DENIED)
        {
          bb_clear(&attributes);
          return ret;
        }
        ret = 0;
      }
    }
    bb_clear(&attributes);
  }
  bb_clear(&attributes);
  return ret;
}

//This function reads ALL objects that meter have. It will loop all object's attributes.
int com_readAllObjects()
{
  int ret;

  //Initialize connection.
  ret = com_initializeConnection();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  //Get objects from the meter and read them.
  ret = com_getAssociationView();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  ret = com_readScalerAndUnits();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  ///////////////////////////////////////////////////////////////////////////////////
  //Read Profile Generic columns.
  ret = com_readProfileGenericColumns();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  ret = com_readValues();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  ret = com_readProfileGenerics();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    return ret;
  }
  return ret;
}