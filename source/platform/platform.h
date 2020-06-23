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

//=============================================================================
// Global Data Type Definition
//=============================================================================

typedef uint16 objectsMask_t;

typedef union
{
	int16 matrix[CFG_NUM_2D_ROWS][CFG_NUM_2D_COLS];
	int16 buffer[CFG_NUM_2D_ROWS * CFG_NUM_2D_COLS];
} touchImage_t;

typedef struct
{
	uint16 segment : 4;
	uint16 nextRow : 6;
	uint16 nextCol : 6;
} segmentLabel_t;

typedef union
{
	segmentLabel_t matrix[CFG_NUM_2D_ROWS][CFG_NUM_2D_COLS];
	segmentLabel_t buffer[CFG_NUM_2D_ROWS * CFG_NUM_2D_COLS];
} segmentLabelImage_t;

//=============================================================================
// Global Functions Declaration
//=============================================================================

//-----------------------------------------------------------------------------
// Function Name:	module_xxx
// Description:		.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Function Name:	module_xxx
// Description:		.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Function Name:	module_xxx
// Description:		.
//-----------------------------------------------------------------------------


#endif	// PLATFORM_H
