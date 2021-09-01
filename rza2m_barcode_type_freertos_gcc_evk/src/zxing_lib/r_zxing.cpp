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
* http://www.renesas.com/disclaimer*
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/**************************************************************************//**
* @file         r_zxing.cpp
* @version      0.01
* @brief        zxing IF
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "r_bcd_main.h"
#include "ImageReaderSource.h"
#include "r_zxing.h"

/******************************************************************************
Macro definitions
******************************************************************************/
#define DECODE_HINTS  (DecodeHints::ONED_HINT | DecodeHints::QR_CODE_HINT | DecodeHints::DATA_MATRIX_HINT | DecodeHints::AZTEC_HINT | DecodeHints::PDF_417_HINT)
//#define DECODE_HINTS  (DecodeHints::QR_CODE_HINT)

/******************************************************************************
Typedef definitions
******************************************************************************/

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/


extern "C" int zxing_decode_image(uint8_t* buf, int width, int height, char * reslut_buf, int reslut_buf_size, barcode_type_t type)
{
    int decode_result;
    int buf_size = width * height;
    vector<Ref<Result> > results;
    std::string decode_str;
    int size;
    DecodeHints hints;

    switch (type)
	{
	case BARCODE_QR:
		hints = zxing::DecodeHints(DecodeHints::QR_CODE_HINT);
		break;
	case BARCODE_AZTEC:
		hints = zxing::DecodeHints(DecodeHints::AZTEC_HINT);
		break;
	case BARCODE_DATA_MATRIX:
		hints = zxing::DecodeHints(DecodeHints::DATA_MATRIX_HINT);
		break;
	default:
		hints = zxing::DecodeHints(DECODE_HINTS);
		break;
	}
//    DecodeHints hints(DECODE_HINTS);
    hints.setTryHarder(false);

    decode_result = ex_decode(buf, buf_size, width, height, true, &results, hints);
    if (decode_result == 0) {
        decode_str = results[0]->getText()->getText();
        size = decode_str.length();
        if (size > reslut_buf_size) {
            memcpy(reslut_buf, decode_str.data(), reslut_buf_size);
            reslut_buf[reslut_buf_size - 1] = '\0';
        } else {
            memcpy(reslut_buf, decode_str.data(), size);
        }
        decode_result = size;
    } else {
        decode_result = -1;
    }

    return decode_result;
}

