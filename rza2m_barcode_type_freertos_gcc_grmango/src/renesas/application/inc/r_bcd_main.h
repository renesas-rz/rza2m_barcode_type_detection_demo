/**********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO
* THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
* Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
**********************************************************************************************************************/
/**********************************************************************************************************************
* File Name    : r_bcd_main.h
**********************************************************************************************************************/

/**********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
**********************************************************************************************************************/
#include <stdbool.h>

/**********************************************************************************************************************
Typedef definitions
**********************************************************************************************************************/

/**********************************************************************************************************************
Macro definitions
**********************************************************************************************************************/
#ifndef R_BCD_MAIN_H
#define R_BCD_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

//add DRP Library Parameters
//#define DRP_R_BINARY_THRESHOLD       (50)
#define DRP_R_BINARY_RANGE       	 (0x1C)
#define DRP_R_FINDC_BUFF_RECT_NUM	 (60)
#define DRP_R_FINDC_BUFF_REGION_NUM  (3) //Region Info is not used in this demo
#define DRP_R_FINDC_WIDTH_THRESHOLD  (5)
#define DRP_R_FINDC_HEIGHT_THRESHOLD (5)
#define DRAW_CONTOURS_THRESHOLD_AREA (81)

//#define KEYSTONE_FIXED_SIZE_ENABLE
#define KEYSTONE_FIXED_SIZE			(192) //(96)


//Use Dilate & Erode or Median Filter
//#define DRP_USE_OPENING
//#define DILATE_ERODE_TIME 5
#define GAUSSIAN_TIME 3


//Debug Draw contours to VDC_LAYER_ID_0_RD
//#define DEBUG_DRAW_CONTOURS




/*******************************************************************************
Private global variables and functions
*******************************************************************************/
typedef enum barcode_type_t
{
	BARCODE_QR,
	BARCODE_MICRO_QR,
	BARCODE_AZTEC,
	BARCODE_HANXIN,
	BARCODE_DATA_MATRIX,
	BARCODE_PDF417,
	BARCODE_MICRO_PDF417,
	BARCODE_MAXICODE,
	BARCODE_CODABLOK_F,
	BARCODE_DOTCODE,
	BARCODE_UNKNOWN
}barcode_type_t;

typedef enum exec_times_t
{
	TIME_TOTAL,								//0
	TIME_B2G_TOTAL,							//1
	TIME_B2G_EXE,							//2
	TIME_B2G_DL,							//3
	TIME_GAUSSIAN_TOTAL,					//4
	TIME_GAUSSIAN_EXE,						//5
	TIME_GAUSSIAN_DL,						//6
	TIME_BINARIZATION_TOTAL,				//7
	TIME_BINARIZATION_DL,					//8
	TIME_BINARIZATION_EXE,					//9
	TIME_FINDC_TOTAL,						//10
	TIME_FINDC_EXE,							//11
	TIME_FINDC_DL,							//12
	TIME_DILATE_TOTAL,						//13
	TIME_DILATE_EXE,						//14
	TIME_DILATE_DL,							//15
	TIME_CROPPING_TOTAL,					//16
	TIME_CROPPING_EXE,						//17
	TIME_CROPPING_DL,						//18
	TIME_AFFINE_TOTAL,						//19
	TIME_AFFINE_DL,							//20
	TIME_AFFINE_EXE,						//21
	TIME_FINDC2_TOTAL,						//22
	TIME_FINDC2_EXE,						//23
	TIME_FINDC2_DL,							//24
	TIME_FIND_POSITION_SYMBOL,				//25
	TIME_GAUSSIAN2_TOTAL,					//26
	TIME_BINARIZATION2_TOTAL,				//27
	TIME_CAMERA_FRAMERATE,					//28
	TIME_ZXING_BINARIZATION_TOTAL,			//29
	TIME_ZXING_BINARIZATION_DL,				//30
	TIME_ZXING_BINARIZATION_EXE,			//31
	TIME_ZXING_REEDSOLOMON_TOTAL,			//32
	TIME_ZXING_REEDSOLOMON_DL,				//33
	TIME_ZXING_REEDSOLOMON_EXE,				//34
	TIME_ZXING_TOTAL,						//35
	TIME_CONTOURS_DRAW,						//36
	TIME_DEBUG_DRAW,						//37

	TIME_INVALID,

}exec_times_t;

typedef struct _R_Rect
{
    int x;
    int y;
    int width;
    int height;
    //float angle;
}R_Rect;

typedef struct _R_Box
{
	int box[4][2];
}R_Box;

typedef struct _R_Point
{
	int16_t x;
	int16_t y;
}R_Point;

typedef struct _R_Contour
{
	R_Rect	rect[DRP_R_FINDC_BUFF_RECT_NUM];
	R_Box	box[DRP_R_FINDC_BUFF_RECT_NUM];
	uint8_t contours_no;
}R_Contour;

/*******************************************************************************
Functions Prototypes
*******************************************************************************/
void R_BCD_Histogram_HV(uint8_t* pbuf, uint16_t width, uint16_t height);
void sample_main(void);

#ifdef __cplusplus
}
#endif

#endif  /* R_BCD_MAIN_H */

/* End of File */
