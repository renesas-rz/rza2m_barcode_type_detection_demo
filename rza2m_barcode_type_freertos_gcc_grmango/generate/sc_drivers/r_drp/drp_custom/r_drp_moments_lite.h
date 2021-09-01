/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
* Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name    : r_drp_moments.h
* Description  : This source code is the header file of moments processing.
* Version      : 0.20
*******************************************************************************/
#ifndef R_DRP_MOMENTS_LITE_H
#define R_DRP_MOMENTS_LITE_H

/*******************************************************************************
Global Typedef definitions
*******************************************************************************/
#ifdef BDL
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed long long int64_t;
typedef signed long int32_t;
typedef signed short int16_t;
typedef signed char int8_t;
#else /* BDL */
#include <stdint.h>
#endif /* BDL */

/* Structure of interface parameters between CPU and DRP library. */
typedef struct
{
    uint32_t src;      	/* Address of input image. */
    uint32_t dst;       /* Address of output moment calculation result */
    uint16_t width;     /* The horizontal size (pixels) of image. should be multiple of 4 */
    uint16_t height;    /* The vertical size (pixels) of image. */
	/* y offset for segment operation 
	0 												- 1 tile
	tile[0] height 									- 2 tile
	tile[0]+tile[1] height							- 3 tile
	tile[0]+tile[1]+tile[2] height					- 4 tile
	tile[0]+tile[1]+tile[2]+tile[3] height			- 5 tile
	tile[0]+tile[1]+tile[2]+tile[3]+tile[4] height	- 6 tile
	*/
	uint16_t height_offset;
} r_drp_moments_lite_t;

typedef struct
{
	/* When segmented processing enabled, calculate the sum of these value for each tiles*/
	uint64_t m00;
	uint64_t m10;
	uint64_t m01;
}r_drp_moments_lite_result_t;

#endif /* R_DRP_MOMENTS_LITE_H */

/* end of file */
