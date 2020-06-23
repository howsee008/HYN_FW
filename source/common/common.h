//*****************************************************************************
// File Name:	common.h
//
// Description:	The header file for common definitions.
//				
// Created by:	George Zheng, Jan. 2020
// Revised by:	Rev.1, George Zheng, Mar. 2020
//
// Copyright 2020 Hynitron, All Rights Reserved.
//*****************************************************************************

#ifndef COMMON_H
#define COMMON_H

//=============================================================================
// Global Data Type Definition
//=============================================================================

// basic integer types
typedef unsigned char 	uint8;
typedef signed char 	int8;
typedef unsigned short 	uint16;
typedef signed short 	int16;
typedef unsigned int	uint32;
typedef signed int		int32;

// signed 16-bit fix-point variants
typedef int16 int14p1;
typedef int16 int13p2;
typedef int16 int12p3;
typedef int16 int11p4;
typedef int16 int10p5;
typedef int16 int9p6;
typedef int16 int8p7;
typedef int16 int7p8;
typedef int16 int6p9;
typedef int16 int5p10;
typedef int16 int4p11;
typedef int16 int3p12;
typedef int16 int2p13;
typedef int16 int1p14;
typedef int16 int0p15;

// unsigned 16-bit fix-point variants
typedef uint16 uint15p1;
typedef uint16 uint14p2;
typedef uint16 uint13p3;
typedef uint16 uint12p4;
typedef uint16 uint11p5;
typedef uint16 uint10p6;
typedef uint16 uint9p7;
typedef uint16 uint8p8;
typedef uint16 uint7p9;
typedef uint16 uint6p10;
typedef uint16 uint5p11;
typedef uint16 uint4p12;
typedef uint16 uint3p13;
typedef uint16 uint2p14;
typedef uint16 uint1p15;
typedef uint16 uint0p16;

// singed 32-bit fix-point variants
typedef int32 int30p1;
typedef int32 int29p2;
typedef int32 int28p3;
typedef int32 int27p4;
typedef int32 int26p5;
typedef int32 int25p6;
typedef int32 int24p7;
typedef int32 int23p8;
typedef int32 int22p9;
typedef int32 int21p10;
typedef int32 int20p11;
typedef int32 int19p12;
typedef int32 int18p13;
typedef int32 int17p14;
typedef int32 int16p15;
typedef int32 int15p16;
typedef int32 int14p17;
typedef int32 int13p18;
typedef int32 int12p19;
typedef int32 int11p20;
typedef int32 int10p21;
typedef int32 int9p22;
typedef int32 int8p23;
typedef int32 int7p24;
typedef int32 int6p25;
typedef int32 int5p26;
typedef int32 int4p27;
typedef int32 int3p28;
typedef int32 int2p29;
typedef int32 int1p30;
typedef int32 int0p31;

// unsinged 32-bit fix-point variants
typedef uint32 uint31p1;
typedef uint32 uint30p2;
typedef uint32 uint29p3;
typedef uint32 uint28p4;
typedef uint32 uint27p5;
typedef uint32 uint26p6;
typedef uint32 uint25p7;
typedef uint32 uint24p8;
typedef uint32 uint23p9;
typedef uint32 uint22p10;
typedef uint32 uint21p11;
typedef uint32 uint20p12;
typedef uint32 uint19p13;
typedef uint32 uint18p14;
typedef uint32 uint17p15;
typedef uint32 uint16p16;
typedef uint32 uint15p17;
typedef uint32 uint14p18;
typedef uint32 uint13p19;
typedef uint32 uint12p20;
typedef uint32 uint11p21;
typedef uint32 uint10p22;
typedef uint32 uint9p23;
typedef uint32 uint8p24;
typedef uint32 uint7p25;
typedef uint32 uint6p26;
typedef uint32 uint5p27;
typedef uint32 uint4p28;
typedef uint32 uint3p29;
typedef uint32 uint2p30;
typedef uint32 uint1p31;
typedef uint32 uint0p32;

typedef union
{
	uint16 word;
	uint8 byte[2];
} dbyte;

typedef union
{
	uint32 dword;
	uint16 word[2];
	uint8 byte[4];
} qbyte;
#endif	// COMMON_H
