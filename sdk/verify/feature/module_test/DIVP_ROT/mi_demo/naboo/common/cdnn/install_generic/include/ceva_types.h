/*************************************************************************************\
*                                                                                     *
* Copyright (C) CEVA Inc. All rights reserved                                         *
*                                                                                     *
*                                                                                     *
* THIS PRODUCT OR SOFTWARE IS MADE AVAILABLE EXCLUSIVELY TO LICENSEES THAT HAVE       *
* RECEIVED EXPRESS WRITTEN AUTHORIZATION FROM CEVA TO DOWNLOAD OR RECEIVE THE         *
* PRODUCT OR SOFTWARE AND HAVE AGREED TO THE END USER LICENSE AGREEMENT (EULA).       *
* IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE               *
* CEVA EULA, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS PRODUCT OR SOFTWARE.           *
*                                                                                     *
* The information contained in this document is subject to change without notice and  *
* does not represent a commitment on any part of CEVA®, Inc. CEVA®, Inc. and its      *
* subsidiaries make no warranty of any kind with regard to this material, including,  *
* but not limited to implied warranties of merchantability and fitness for a          *
* particular purpose whether arising out of law, custom, conduct or otherwise.        *
*                                                                                     *
* While the information contained herein is assumed to be accurate, CEVA®, Inc.       *
* assumes no responsibility for any errors or omissions contained herein, and         *
* assumes no liability for special, direct, indirect or consequential damage,         *
* losses, costs, charges, claims, demands, fees or expenses, of any nature or kind,   *
* which are incurred in connection with the furnishing, performance or use of this    *
* material.                                                                           *
*                                                                                     *
* This document contains proprietary information, which is protected by U.S. and      *
* international copyright laws. All rights reserved. No part of this document may be  *
* reproduced, photocopied, or translated into another language without the prior      *
* written consent of CEVA®, Inc.                                                      *
*                                                                                     *
**************************************************************************************/
#ifndef _CEVA_TYPES_
#define _CEVA_TYPES_

#ifdef __cplusplus
	#include <cstddef>
#else
	#include <stddef.h>
#endif
#include <assert.h>
#include <stdint.h>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

typedef struct
{
	short x;    /** x-coordinate */
	short y;	/** y-coordinate */
} ShortPoint;

typedef struct
{
	unsigned short x;   /** x-coordinate */
	unsigned short y;	/** y-coordinate */
} UShortPoint;

typedef struct
{
	int x;  /** x-coordinate */
	int y;	/** y-coordinate */
} IntPoint;

typedef struct
{
	unsigned int x; /** x-coordinate */
	unsigned int y;	/** y-coordinate */
} UIntPoint;

typedef struct
{
	unsigned short width;  /** width of the rectangle  */
	unsigned short height; /** height of the rectangle */
} UShortSize;

typedef struct
{
	int width;  /** width of the rectangle  */
	int height; /** height of the rectangle */
} IntSize;

typedef struct
{
	unsigned int width;  /** width of the rectangle  */
	unsigned int height; /** height of the rectangle */
} UIntSize;

typedef struct
{
	unsigned short x;      /** x-coordinate of top left corner of rectangle */
	unsigned short y;      /** y-coordinate of top left corner of rectangle */
	unsigned short width;  /** width of the rectangle  */
	unsigned short height; /** height of the rectangle */
} UShortRect;

typedef struct
{
	short	x;      /** x-coordinate of top left corner of rectangle */
	short	y;      /** y-coordinate of top left corner of rectangle */
	unsigned short	width;  /** width of the rectangle  */
	unsigned short	height; /** height of the rectangle */
} ShortRect;

typedef struct
{
	int		x;      /** x-coordinate of top left corner of rectangle */
	int		y;      /** y-coordinate of top left corner of rectangle */
	unsigned int	width;  /** width of the rectangle  */
	unsigned int	height; /** height of the rectangle */
} IntRect;

#endif