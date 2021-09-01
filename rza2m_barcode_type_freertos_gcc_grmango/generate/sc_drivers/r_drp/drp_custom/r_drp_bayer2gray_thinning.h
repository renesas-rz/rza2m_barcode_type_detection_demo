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
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
 * File Name    : r_drp_bayer2gray_thinning.h
 * Description  : This source code is input image conversion from bayer to grayscale
 *                output width/height is 1/4 input size
 *					Use 1 tiles
 * Version     : 1.00
 ******************************************************************************/
#ifndef R_DRP_BAYER2GRAY_THINNING_H
#define R_DRP_BAYER2GRAY_THINNING_H

/******************************************************************************
Global Typedef definitions
*****************************************************************************/
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
    uint32_t src;
    uint32_t dst;

    uint32_t accumulate;
    uint16_t width;
    uint16_t height;

	//accumulation area, coordinates within output thinning image 
	uint16_t area1_offset_x;
    uint16_t area1_offset_y;
    uint16_t area1_width;
    uint16_t area1_height;

//	uint16_t area2_offset_x;
//    uint16_t area2_offset_y;
//    uint16_t area2_width;
//    uint16_t area2_height;
//
//    uint16_t area3_offset_x;
//    uint16_t area3_offset_y;
//    uint16_t area3_width;
//    uint16_t area3_height;
}
r_drp_bayer2gray_thinning_t;

#endif /* R_DRP_BAYER2GRAY_THINNING_H */

/* end of file */
