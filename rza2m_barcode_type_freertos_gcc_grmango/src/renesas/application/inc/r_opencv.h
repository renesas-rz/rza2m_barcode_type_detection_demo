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
#ifndef R_OPENCV_H
#define R_OPENCV_H

#ifdef __cplusplus
extern "C" {
#endif



/************ opencv function **********/
int cv_moments(uint8_t* src, uint16_t width, uint16_t height, uint16_t* center_x, uint16_t* center_y, double* angle);
int cv_morphologyEx(uint8_t *img, uint16_t width, uint16_t height, uint16_t kw, uint16_t kh, int type, uint8_t *out);
int cv_findcontours_boundingrect(uint8_t *img, uint16_t width, uint16_t height, uint32_t frameId);
int cv_affine_transform(uint8_t *src, uint8_t *dst, uint16_t* w, uint16_t* h, R_Box r_box);



int cv_bitwise_not(uint8_t *src, uint16_t in_w, uint16_t in_h);
int cv_threshold(uint8_t *src, uint16_t in_w, uint16_t in_h, uint8_t thres, uint8_t mode);
int cv_crop(uint8_t *src, uint16_t in_w, uint16_t in_h, uint16_t out_x, uint16_t out_y, uint16_t out_w, uint16_t out_h, uint8_t color_channel);
int cv_crop_copy_make_border_resize(uint8_t *src, uint16_t in_w, uint16_t in_h, uint16_t area_x, uint16_t area_y, uint16_t area_w, uint16_t area_h, uint8_t* dst, uint16_t out_w, uint16_t out_h);

#ifdef __cplusplus
}
#endif

#endif  /* R_OPENCV_H */

/* End of File */
