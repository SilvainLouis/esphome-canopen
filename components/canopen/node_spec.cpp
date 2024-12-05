/******************************************************************************
   Copyright 2020 Embedded Office GmbH & Co. KG
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
******************************************************************************/

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "node_spec.h"
#include "driver_can.h"
#include "co_storage.h"

/******************************************************************************
 * PRIVATE DEFINES
 ******************************************************************************/

/* Define some default values for our CANopen node: */
#define APP_NODE_ID 1u             /* CANopen node ID             */
#define APP_BAUDRATE 250000u       /* CAN baudrate                */
#define APP_TMR_N 16u              /* Number of software timers   */
#define APP_TICKS_PER_SEC 1000000u /* Timer clock frequency in Hz */

#ifndef APP_OBJ_N
#define APP_OBJ_N 512u             /* Object dictionary max size  */
#endif

/******************************************************************************
 * PRIVATE VARIABLES
 ******************************************************************************/

/* allocate variables for dynamic runtime value in RAM */
static uint8_t Obj1001_00_08 = 0;

/* allocate variables for constant values in FLASH */
const uint32_t Obj1000_00_20 = 0x00000000L;

const uint32_t Obj1014_00_20 = 0x00000080L;

const uint32_t VENDOR_ID = 0xa59a08f5;
const uint32_t PRODUCT_CODE = 0x6bdfa1d9;
const uint32_t REVISION_NUMBER = 0x00000000;
const uint32_t SERIAL_NUMBER = 0x00000000;

const uint32_t Obj1200_01_20 = CO_COBID_SDO_REQUEST();
const uint32_t Obj1200_02_20 = CO_COBID_SDO_RESPONSE();

uint8_t rpdo_buf[CO_RPDO_N][41];

#define RPDO_PARAM(n) \
  {CO_KEY(0x1400 + n, 0, CO_OBJ_____RW), CO_TUNSIGNED8, (CO_DATA) rpdo_buf[n] + 0}, \
      {CO_KEY(0x1400 + n, 1, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 1}, \
      {CO_KEY(0x1400 + n, 2, CO_OBJ_____RW), CO_TUNSIGNED8, (CO_DATA) rpdo_buf[n] + 5}, { \
    CO_KEY(0x1400 + n, 3, CO_OBJ_____RW), CO_TUNSIGNED16, (CO_DATA) rpdo_buf[n] + 6 \
  }

#define RPDO_MAPPING(n) \
  {CO_KEY(0x1600 + n, 0, CO_OBJ_____RW), CO_TUNSIGNED8, (CO_DATA) rpdo_buf[n] + 8}, \
      {CO_KEY(0x1600 + n, 1, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 9}, \
      {CO_KEY(0x1600 + n, 2, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 13}, \
      {CO_KEY(0x1600 + n, 3, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 17}, \
      {CO_KEY(0x1600 + n, 4, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 21}, \
      {CO_KEY(0x1600 + n, 5, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 25}, \
      {CO_KEY(0x1600 + n, 6, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 29}, \
      {CO_KEY(0x1600 + n, 7, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 33}, { \
    CO_KEY(0x1600 + n, 8, CO_OBJ_____RW), CO_TUNSIGNED32, (CO_DATA) rpdo_buf[n] + 37 \
  }

/* define the static object dictionary */
static struct CO_OBJ_T ObjectDictionary[APP_OBJ_N] = {
    {CO_KEY(0x1000, 0, CO_OBJ_____R_), CO_TUNSIGNED32, (CO_DATA) (&Obj1000_00_20)},
    {CO_KEY(0x1001, 0, CO_OBJ_____R_), CO_TUNSIGNED8, (CO_DATA) (&Obj1001_00_08)},
    {CO_KEY(0x100a, 0, CO_OBJ_D___R_), CO_TSTRING, (CO_DATA) 0},
    {CO_KEY(0x1010, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) 2},
    {CO_KEY(0x1010, 1, CO_OBJ_D___RW), CO_TSTORE, (CO_DATA) 0},
    {CO_KEY(0x1010, 2, CO_OBJ_D___RW), CO_TSTORE, (CO_DATA) 0},
    {CO_KEY(0x1011, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) 2},
    {CO_KEY(0x1011, 1, CO_OBJ_D___RW), CO_TRESET, (CO_DATA) 0},
    {CO_KEY(0x1011, 2, CO_OBJ_D___RW), CO_TRESET, (CO_DATA) 0},

    {CO_KEY(0x1014, 0, CO_OBJ__N__R_), CO_TEMCY_ID, (CO_DATA) (&Obj1014_00_20)},
    {CO_KEY(0x1016, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) 0x00000000},

    {CO_KEY(0x1018, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (4)},
    {CO_KEY(0x1018, 1, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA) (VENDOR_ID)},
    {CO_KEY(0x1018, 2, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA) (PRODUCT_CODE)},
    {CO_KEY(0x1018, 3, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA) (REVISION_NUMBER)},
    {CO_KEY(0x1018, 4, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA) (SERIAL_NUMBER)},

    {CO_KEY(0x1200, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (2)},
    {CO_KEY(0x1200, 1, CO_OBJ__N__R_), CO_TUNSIGNED32, (CO_DATA) (&Obj1200_01_20)},
    {CO_KEY(0x1200, 2, CO_OBJ__N__R_), CO_TUNSIGNED32, (CO_DATA) (&Obj1200_02_20)},
#if CO_RPDO_N > 0
    RPDO_PARAM(0),
#endif
#if CO_RPDO_N > 1
    RPDO_PARAM(1),
#endif
#if CO_RPDO_N > 2
    RPDO_PARAM(2),
#endif
#if CO_RPDO_N > 3
    RPDO_PARAM(3),
#endif
#if CO_RPDO_N > 4
    RPDO_PARAM(4),
#endif
#if CO_RPDO_N > 5
    RPDO_PARAM(5),
#endif
#if CO_RPDO_N > 6
    RPDO_PARAM(6),
#endif
#if CO_RPDO_N > 7
    RPDO_PARAM(7),
#endif

#if CO_RPDO_N > 0
    RPDO_MAPPING(0),
#endif
#if CO_RPDO_N > 1
    RPDO_MAPPING(1),
#endif
#if CO_RPDO_N > 2
    RPDO_MAPPING(2),
#endif
#if CO_RPDO_N > 3
    RPDO_MAPPING(3),
#endif
#if CO_RPDO_N > 4
    RPDO_MAPPING(4),
#endif
#if CO_RPDO_N > 5
    RPDO_MAPPING(5),
#endif
#if CO_RPDO_N > 6
    RPDO_MAPPING(6),
#endif
#if CO_RPDO_N > 7
    RPDO_MAPPING(7),
#endif

    {CO_KEY(0x1800, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1801, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1802, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1803, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1804, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1805, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1806, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1807, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},

    {CO_KEY(0x1A00, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A01, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A02, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A03, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A04, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A05, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A06, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},
    {CO_KEY(0x1A07, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) (0)},

    CO_OBJ_DICT_ENDMARK /* mark end of used objects */
};

static void ObjSet(CO_OBJ *obj, uint32_t key, const CO_OBJ_TYPE *type, CO_DATA data) {
  obj->Key = key;
  obj->Type = type;
  obj->Data = data;
}

static void ObjCpy(CO_OBJ *a, CO_OBJ *b) {
  a->Key = b->Key;
  a->Type = b->Type;
  a->Data = b->Data;
}

static void ObjSwap(CO_OBJ *a, CO_OBJ *b) {
  CO_OBJ x;
  ObjCpy(&x, a);
  ObjCpy(a, b);
  ObjCpy(b, &x);
}

static int16_t ObjCmp(CO_OBJ *a, CO_OBJ *b) {
  int16_t result = 1;

  if (CO_GET_DEV(a->Key) == CO_GET_DEV(b->Key)) {
    result = 0;

  } else if (CO_GET_DEV(a->Key) < CO_GET_DEV(b->Key)) {
    result = -1;
  }

  return (result);
}

CO_OBJ *ODFind(CO_OBJ *od, uint32_t key) {
  CO_OBJ temp;
  CO_OBJ *last = od + APP_OBJ_N;
  ObjSet(&temp, key, 0, 0);
  while (od < last && ObjCmp(od, &temp) < 0) {
    od++;
  }
  if (od == last || ObjCmp(od, &temp) != 0)
    return 0;
  return od;
}

CO_OBJ *ODAddUpdate(CO_OBJ *od, uint32_t key, const CO_OBJ_TYPE *type, CO_DATA data) {
  uint8_t sub = CO_GET_SUB(key);
  uint16_t idx = CO_GET_IDX(key);

  CO_OBJ *dict = od;
  CO_OBJ *sub_cnt = 0;
  CO_OBJ temp;
  CO_OBJ *last = od + APP_OBJ_N;
  CO_OBJ *ret = 0;

  if (key == 0) {
    return ret;
  }
  ObjSet(&temp, key, type, data);

  /* find position in dictionary */
  while (od < last && (od->Key != 0) && (ObjCmp(od, &temp) < 0)) {
    if (sub && CO_GET_DEV(od->Key) == CO_DEV(idx, 0))
      sub_cnt = od;
    od++;
  }
  if (od == last) {
    return ret;
  }

  if (ObjCmp(od, &temp) == 0) { /* Change existing entry */
    ObjSet(od, key, type, data);
    ret = od;
  } else if (od->Key == 0) { /* Append at end of dictionary */
    ObjSet(od, key, type, data);
    ret = od;
    od++;
    ObjSet(od, 0, 0, 0);
  } else { /* Insert in middle of dictionary */
    do {
      ObjSwap(od, &temp);
      od++;
    } while (od->Key != 0);
    ret = od;
    ObjCpy(od, &temp);
    od++;
    ObjSet(od, 0, 0, 0);
  }

  if (sub) {
    // update max index
    if (sub_cnt) {
      if (sub_cnt->Data < sub)
        sub_cnt->Data = sub;
    } else {
      // sub_cnt obj doesn't exit, insert it
      ODAddUpdate(dict, CO_KEY(idx, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA) sub);
      // new entry addeed before ret, so increment it
      ret++;
    }
  }

  return ret;
}

/* Each software timer needs some memory for managing
 * the lists and states of the timed action events.
 */
static CO_TMR_MEM TmrMem[APP_TMR_N];

/* Each SDO server needs memory for the segmented or
 * block transfer requests.
 */
static uint8_t SdoSrvMem[CO_SSDO_N * CO_SDO_BUF_BYTE];

/* Specify the EMCY error codes with the corresponding
 * error register bit. There is a collection of defines
 * for the predefined emergency codes CO_EMCY_CODE...
 * and for the error register bits CO_EMCY_REG... for
 * readability. You can use plain numbers, too.
 */
static CO_EMCY_TBL AppEmcyTbl[APP_ERR_ID_NUM] = {
    {CO_EMCY_REG_GENERAL, CO_EMCY_CODE_HW_ERR} /* APP_ERR_ID_EEPROM */
};

/******************************************************************************
 * PUBLIC VARIABLES
 ******************************************************************************/

/* Collect all node specification settings in a single
 * structure for initializing the node easily.
 */
struct CO_NODE_SPEC_T NodeSpec = {
    APP_NODE_ID,                  /* default Node-Id                */
    APP_BAUDRATE,                 /* default Baudrate               */
    ObjectDictionary,             /* pointer to object dictionary   */
    APP_OBJ_N,                    /* object dictionary max length   */
    AppEmcyTbl,                   /* EMCY code & register bit table */
    TmrMem,                       /* pointer to timer memory blocks */
    APP_TMR_N,                    /* number of timer memory blocks  */
    APP_TICKS_PER_SEC,            /* timer clock frequency in Hz    */
    &CanOpenStack_Driver, /* select drivers for application */
    SdoSrvMem                     /* SDO Transfer Buffer Memory     */
};
