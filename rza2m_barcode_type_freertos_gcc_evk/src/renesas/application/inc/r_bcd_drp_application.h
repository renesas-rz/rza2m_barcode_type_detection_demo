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

#ifndef R_BCD_DRP_APPLICATION_H_
#define R_BCD_DRP_APPLICATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "r_dk2_if.h"
#include "r_drp_bayer2gray_thinning.h"
#include "r_drp_binarization_adaptive_custom.h"
#include "r_drp_find_contours.h"
#include "r_drp_dilate.h"
#include "r_drp_bayer2gray_cropping.h"
#include "r_drp_binarization_adaptive_bit.h"
#include "r_drp_reed_solomon.h"
#include "r_bcd_main.h"


/*******************************************************************************
Macro definitions
*******************************************************************************/
#define TILE_0            (0)
#define TILE_1            (1)
#define TILE_2            (2)
#define TILE_3            (3)
#define TILE_4            (4)
#define TILE_5            (5)

#define DRP_NOT_FINISH    (0)
#define DRP_FINISH        (1)
#define DRP_DRV_ASSERT(x) if ((x) != 0) while(1)


/******************************************************************************
Functions Prototypes
******************************************************************************/

//function
//call drp library functions
void R_BCD_DRP_Bayer2Gray_thinning(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height);
//void R_BCD_DRP_GaussianBlur(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint8_t times);
void R_BCD_DRP_Binarization(uint32_t input_address, uint32_t output_address, uint32_t work_address, uint32_t input_width, uint32_t input_height, uint8_t range);
//void R_BCD_DRP_Binarization_fixed(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint8_t threshold);

void R_BCD_DRP_FindContours(uint32_t input_address, uint32_t work_address, uint16_t input_width, uint16_t input_height, void* cb, uint8_t frame_id);
void R_BCD_DrpDrawContours(uint32_t contours_no);
void R_BCD_DrpDrawContour(uint32_t contours_no);
void R_BCD_DrpDrawContourBox(R_Box box);
void R_BCD_DrpDrawContourPoints(uint32_t contours_no);
void R_BCD_DrpCleanContours(void);
uint32_t R_BCD_DrpResizeContours(uint8_t frame_id);
uint32_t R_BCD_DrpCheckEmptyContours(uint32_t i);
uint32_t R_BCD_DRP_Contours_2_Box(uint8_t frame_id, uint16_t width, uint16_t height);
void R_BCD_DRP_Dilate(uint32_t input_address, uint32_t output_address1, uint32_t output_address2, uint32_t input_width, uint32_t input_height, uint8_t times);
//void R_BCD_DRP_Cropping(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint32_t offset_x, uint32_t offset_y, uint32_t output_width, uint32_t output_height);
void R_BCD_DRP_Bayer2Gray_Cropping(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint32_t offset_x, uint32_t offset_y, uint32_t output_width, uint32_t output_height);
//void R_BCD_DRP_Keystone_Correction(uint32_t input_address, uint32_t output_address, uint16_t *input_width, uint16_t *input_height, R_Box r_box);
void R_BCD_Keystone_Correction(uint32_t input_address, uint32_t output_address, uint16_t* input_width, uint16_t* input_height, R_Box r_box);
void R_BCD_MainBinarization2(uint32_t in_adr, uint32_t out_adr, uint32_t width, uint32_t height);
bool R_BCD_MainReedsolomon(int8_t * codewordBytes, uint8_t numCodewords, uint8_t numECCodewords);
void R_BCD_MainSetReedsolomonTime(uint32_t time);

#ifdef __cplusplus
}
#endif

#endif /* R_BCD_DRP_APPLICATION_H_ */
