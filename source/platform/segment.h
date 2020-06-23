//*****************************************************************************
// File Name:	Segment.h
//
// Description:	The source file of tracking module, which creates and manages
//				the "connections" between the segments and the logical objects.
//
// Created by:	Sea Hou, Jan. 2020
// Revised by:	Rev.1, Sea Hou, Jan. 2020
//
// Copyright 2020 Hynitron, All Rights Reserved.
//*****************************************************************************
#ifndef TRACKING_H
#define TRACKING_H

#include "ctypes.h"
#include "platform.h"
#include "config.h"
//=============================================================================
// Macro
//=============================================================================
#define OBJ_DAM    10
#define OBJ_DAM2   11
#define OBJ_INIT   12
#define OBJ_MASK   13
#define OBJ_INQE   14

#define SEG_EDGE_L 1  // left edge
#define SEG_EDGE_R 2  // righ edge
#define SEG_EDGE_N 0  // not edge
//=============================================================================
// Global Data Type Definition
//=============================================================================
 typedef struct
 {
 	uint16 lab : 4;	// label of the sensor
 	uint16 edge: 2; // edge status: SEG_EDGE_N, SEG_EDGE_L, SEG_EDGE_R
    uint16 nextP:10; // the position of next sensor with same label 0~1024
 }labMap_t; // the struct of label map

 typedef struct
 {
   uint16 headP;//the head pointing to the first pixel
   uint16 tailP;//the tail pointing to the last pixel
   uint16 peakP;//the peak pointing to the peak pixel
   uint16 linksize;//how manys piexl in the object
 }objLink_t;

typedef struct
{
  uint16 headP;//the head pointing to the first pixel
  uint16 tailP;//the tail pointing to the last pixel
  uint16 peakP;//the peak pointing to the peak pixel
  uint16 linksize;//how manys piexl in the object
  uint16 flagbits;//it show what objects are connected by the dam. bit id = object id.

}damLink_t;

typedef struct
{
  labMap_t lableMap[CFG_COL*CFG_ROW]; // label and linker. it has same size of img
  objLink_t objLink[MAX_OBJECTS];// it contains segmented object info.
  damLink_t damLink[MAX_OBJECTS];// it contains dam info.
  uint8 objCount;// how many objects are found.
  uint8 damCount;// how many dam are found.
}segPublic_t;

//=============================================================================
// Global Functions Declaration
//=============================================================================
void segmenting_init(void);

segPublic_t * segmenting_getpubinfo(void);

void segmenting_segment(sigData_t* imgarray, uint16 imgsize);

#endif// TRACKING_H

