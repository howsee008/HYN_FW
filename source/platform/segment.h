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
#ifndef SEG_H
#define SEG_H

#include "platform.h"
//=============================================================================
// Macro
//=============================================================================
#define OBJ_MINB   10
#define OBJ_DAM    11
#define OBJ_MASK   12
#define OBJ_INQE   13
#define OBJ_INIT   14


#define RES_ERROR0  (-1)
#define RES_OK (0)

#define SEGHOOKSIZE  (CFG_MAX_OBJECTS+5)
#define SEGHOOKID_OBJ 	 (0)   					// 0~CFG_MAX_OBJECTS-1 objects
#define SEGHOOKID_MINB  (CFG_MAX_OBJECTS)		//  unsegment below minpeak
#define SEGHOOKID_DAM 	 (CFG_MAX_OBJECTS+1)	//  dam
#define SEGHOOKID_BCKGND (CFG_MAX_OBJECTS+2)	//  background
#define SEGHOOKID_DIA	 (CFG_MAX_OBJECTS+3)	//  dilate
#define SEGHOOKID_UNSEG  (CFG_MAX_OBJECTS+4)	// unsegment above minpeak

#define END_IDX  (0x3f)
#define RESET_VAL (0xffff)
//=============================================================================
// Global Data Type Definition
//=============================================================================


//=============================================================================
// Global Functions Declaration
//=============================================================================


#endif// TRACKING_H

