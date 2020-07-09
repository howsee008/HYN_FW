//*****************************************************************************
// File Name:	platform.h
//
// Description:	The header file of platform.
//
// Created by:	George, Mar. 2020
// Revised by:	Rev.1, George, Mar. 2020
//
// Copyright 2020 Hynitron, All Rights Reserved.
//*****************************************************************************

#ifndef PLATFORM_H
#define PLATFORM_H

#include "../common/common.h"
#include "../project/config.h"
#include "string.h"

//=============================================================================
// Global Data Type Definition
//=============================================================================

typedef uint16 objectsMask_t;

typedef union
{
	int16 matrix[CFG_NUM_2D_ROWS][CFG_NUM_2D_COLS];
	int16 buffer[CFG_NUM_2D_ROWS * CFG_NUM_2D_COLS];
} touchImage_t;

//-----------------------------------------------------------------------------
// Function Name:	segmentation data type
// Description:		.
//-----------------------------------------------------------------------------


typedef union
{
  struct{
	uint16 lable : 4;
	uint16 nextRow : 6;
	uint16 nextCol : 6;
  	} feilds;
  uint16 value;
} segmentLabel_t;


typedef segmentLabel_t * segmentLabel_ptr;

typedef union
{
	segmentLabel_t matrix[CFG_NUM_2D_ROWS][CFG_NUM_2D_COLS];
	segmentLabel_t buffer[CFG_NUM_2D_ROWS * CFG_NUM_2D_COLS];
} segmentLabelImage_t;


typedef struct
{
  segmentLabelImage_t const * lableMap; // label and linker. it has same size of img
  segmentLabel_t const * segments;// it contains object info.
  segmentLabel_t const * background; // it is link of background sensors or non-ROI sensor
  segmentLabel_t const * unsegmented; // it is a link of small segments
}segPublic_t;

typedef struct
{
 int16 segResolution;
 int16 segSigMax;
 int16 segSigROI;
 int16 segMinPeak;
 int16 segMergeCoef;
}segConfig_t;


//=============================================================================
// Global Functions Declaration
//=============================================================================

//-----------------------------------------------------------------------------
// Function Name:	segmentation
// Description:		.
//-----------------------------------------------------------------------------
void segmentation_init(void);
void segmentation_getConfig(segConfig_t const * segConfigPtr);
segmentLabel_ptr segmentation_searchRoi(touchImage_t* imgarray);
segPublic_t * segmentation_getSegmentInfo(void);
uint16 segmentation_segments(touchImage_t* imgarray);



//-----------------------------------------------------------------------------
// Function Name:	module_xxx
// Description:		.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Function Name:	module_xxx
// Description:		.
//-----------------------------------------------------------------------------


#endif	// PLATFORM_H
