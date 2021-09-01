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

#include "main.h"
#include "r_bcd_main.h"
#include "r_bcd_drp_application.h"
#include "r_bcd_contour.h"
#include <string.h>

#define CONTOUR_MAX_CHILD		(2)


typedef struct
{
	uint32_t area;
	uint8_t total_child;   		//total child number
	uint8_t childs[CONTOUR_MAX_CHILD];		//first 8 childs
}R_CONTOUR_CHILD;

typedef enum
{
	BST_QR,
	BST_HANXIN,
	BST_INVALID,
}R_BARCODE_SYMBOL_TYPE;

static R_CONTOUR_CHILD		app_output_data_contour_child[DRP_R_FINDC_BUFF_RECT_NUM];
static int 					g_position_symbol[8];

static void find_child_rect(R_Rect *p_rect, uint32_t num)
{
	uint8_t childs = 0;
	uint32_t i, j, k;
	uint16_t x, y;
	uint16_t x0, y0, x1, y1;

	for (i=0; i<num; i++)
	{
		x0 = p_rect[i].x;
		x1 = x0 +  p_rect[i].width;
		y0 = p_rect[i].y;
		y1 = y0 + p_rect[i].height;

		childs = 0;
		for (j=i+1; j<num; j++)
		{
			//check center
			x = p_rect[j].x + (p_rect[j].width >> 1);
			y = p_rect[j].y + (p_rect[j].height >> 1);
			if (x > x0 && x < x1 && y > y0 && y < y1 )
			{
				if (childs < CONTOUR_MAX_CHILD)
				{
					app_output_data_contour_child[i].childs[childs] = j;
				}
				childs++;
			}
		}
		app_output_data_contour_child[i].total_child = childs;
	}
}


static void center_delta(R_Rect rect1, R_Rect rect2, uint16_t* dx, uint16_t* dy)
{
	*dx = abs((rect1.x + (rect1.width >> 1)) - (rect2.x + (rect2.width >> 1)));
	*dy = abs((rect1.y + (rect1.height >> 1)) - (rect2.y + (rect2.height >> 1)));
}

static uint16_t area_ratio(uint32_t contour1, uint32_t contour2, uint32_t tor_low, uint32_t tor_high)
{
	uint32_t area0, area1;
	uint32_t ratio;

	area0 = 100 * app_output_data_contour_child[contour1].area;
	area1 = app_output_data_contour_child[contour2].area;
	ratio = area0 / area1;

	return ((ratio >= tor_low) && (ratio <= tor_high));
}


static barcode_type_t check_qr_symbol(R_Rect *p_rect, uint8_t symbol_num, uint16_t width, uint16_t height)
{
	barcode_type_t ret = BARCODE_UNKNOWN;
	int i;
	uint16_t x, y;

	if (symbol_num == 1)
	{
		x = p_rect[g_position_symbol[0]].x + (p_rect[g_position_symbol[0]].width >> 1);
		y = p_rect[g_position_symbol[0]].y + (p_rect[g_position_symbol[0]].height >> 1);
		if ((x < (width / 3) || x > (2*width/3)) && (y < (height / 3) || y > (2*height/3)))
		{
			ret = BARCODE_MICRO_QR;
		}
		else if ((x > (width / 3)) && ( x < (2*width/3)) && (y > (height / 3)) && ( y < (2*width/3)))
		{
			ret = BARCODE_AZTEC;
		}
	}
	else if (symbol_num == 3 || symbol_num == 2)
	{
		ret = BARCODE_QR;
		for (i=0; i<symbol_num; i++)
		{
			x = p_rect[g_position_symbol[i]].x + (p_rect[g_position_symbol[i]].width >> 1);
			y = p_rect[g_position_symbol[i]].y + (p_rect[g_position_symbol[i]].height >> 1);
			if ((x > (width / 3) && x < (2*width/3)) || (y > (height / 3) && (y < (2*height/3))))
			{
				ret = BARCODE_UNKNOWN;
				break;
			}
		}
	}
	else
	{
		uint8_t pos = 0;

		for (i=0; i<symbol_num; i++)
		{
			x = p_rect[g_position_symbol[i]].x + (p_rect[g_position_symbol[i]].width >> 1);
			y = p_rect[g_position_symbol[i]].y + (p_rect[g_position_symbol[i]].height >> 1);
			if ((x < (width / 3) || x > (2*width/3)) && (y < (height / 3) || (y > (2*height/3))))
			{
				pos++;
			}
		}
		if (pos >= 3)
			ret = BARCODE_QR;
		else
			ret = BARCODE_UNKNOWN;
	}
	return ret;
}

static barcode_type_t check_hanxin_symbol(R_Rect *p_rect, uint8_t symbol_num, uint16_t width, uint16_t height)
{
	barcode_type_t ret = BARCODE_UNKNOWN;
	int i;
	uint16_t x, y;

	if (symbol_num == 4)
		ret = BARCODE_HANXIN;

	return ret;
}

static R_BARCODE_SYMBOL_TYPE symbol_detect_child_2(R_Rect *p_rect, uint8_t index, uint16_t width, uint16_t height)
{
	R_BARCODE_SYMBOL_TYPE ret = BST_INVALID;
	uint16_t dx0, dy0, dx1, dy1;
	uint16_t area_ratio1, area_ratio2;
	uint16_t center_offset_thres_x = p_rect[index].width * 100 / 7 / 2;
	uint16_t center_offset_thres_y = p_rect[index].height * 100 / 7 / 2;
	uint16_t area_ratio1_low=136, area_ratio1_high=306;
	uint16_t area_ratio2_low=156, area_ratio2_high=625;
	uint16_t x, y;

	x = p_rect[index].x + (p_rect[index].width >> 1);
	y = p_rect[index].y + (p_rect[index].height >> 1);

	center_delta(p_rect[index], p_rect[app_output_data_contour_child[index].childs[0]], &dx0, &dy0);
	center_delta(p_rect[index], p_rect[app_output_data_contour_child[index].childs[1]], &dx1, &dy1);

	area_ratio1 = 100 * app_output_data_contour_child[index].area / app_output_data_contour_child[app_output_data_contour_child[index].childs[0]].area;
	area_ratio2 = 100 * app_output_data_contour_child[app_output_data_contour_child[index].childs[0]].area / app_output_data_contour_child[app_output_data_contour_child[index].childs[1]].area;

	if (100*dx0 < center_offset_thres_x && 100*dx1 < center_offset_thres_x &&
		100*dy1 < center_offset_thres_y && 100*dy1 < center_offset_thres_y &&
		area_ratio1 >= 136 && area_ratio1 <= 306 &&
		area_ratio2 >= 156 && area_ratio2 <= 625 &&
		(((x <= width/3 || x >= 2*width/3) && (y <= height/3 || y >= 2*height/3)) || (x > width/3 && x < 2*width/3 && y > height/3 && y < 2*height/3)))
	{
		//check two contour's center delta
		//check area ratio
		//condition2: check parent contour's center position (should be located at barcode corner 1/3, or center)
		//QR(1)100*7*7/4*4 ~ 100*7*7/6*6  = 136 ~ 306
		//QR(2)100*5*5/4*4 ~ 100*5*5/2*2 = 156 ~ 625
		ret = BST_QR;
	}
	else if (100*dx0 >= center_offset_thres_x && 100*dx0 < (3 * center_offset_thres_x) &&
			100*dy0 >= center_offset_thres_y && 100*dy0 < (3 * center_offset_thres_y) &&
			100*dx1 >= (3 * center_offset_thres_x) && 100*dx0 < (5 * center_offset_thres_x) &&
			100*dy1 >= (3 * center_offset_thres_y) && 100*dy0 < (5 * center_offset_thres_y) &&
			area_ratio1 >= 136 && area_ratio1 <= 306 &&
			area_ratio2 >= 156 && area_ratio2 <= 625 &&
			((x < (width / 3) || x > (2*width/3)) && (y < (height / 3) || y > (2*height/3))))
	{
		//check two contour's center delta
		//check area ratio
		//check parent contour's center position (should be located at barcode corner 1/3)
		ret = BST_HANXIN;
	}
	else
	{

	}

	return ret;
}

static R_BARCODE_SYMBOL_TYPE symbol_detect_child_1(R_Rect *p_rect, uint8_t index, uint16_t width, uint16_t height)
{
	int i, j;
	R_BARCODE_SYMBOL_TYPE ret = BST_INVALID;
	uint16_t dx0, dy0;
	uint16_t center_offset_thres_x = p_rect[index].width * 100 / 7 / 2;
	uint16_t center_offset_thres_y = p_rect[index].height * 100 / 7 / 2;
	uint16_t x, y;

	for (i=0; i<8; i++)
	{
		if (g_position_symbol[i] == -1)
			break;
		//if current rect already been marked, mostly in 2 childs checking, just ignore it
		for (j=0; j<app_output_data_contour_child[g_position_symbol[i]].total_child; j++)
		{
			if (index == app_output_data_contour_child[g_position_symbol[i]].childs[j])
				return BST_INVALID;
		}
	}

	x = p_rect[index].x + (p_rect[index].width >> 1);
	y = p_rect[index].y + (p_rect[index].height >> 1);

	center_delta(p_rect[index], p_rect[app_output_data_contour_child[index].childs[0]], &dx0, &dy0);

	if (100*dx0 < center_offset_thres_x && 100*dy0 < center_offset_thres_y &&
		(((x <= width/3 || x >= 2*width/3) && (y <= height/3 || y >= 2*height/3)) || (x > width/3 && x < 2*width/3 && y > height/3 && y < 2*height/3)))
	{
		//condition1: check two contour's center delta
		//condition2: check parent contour's center position (should be located at barcode corner 1/3, or center)
		//QR(1)100*7*7/4*4 ~ 100*7*7/6*6  = 136 ~ 306
		//QR(2)100*5*5/4*4 ~ 100*5*5/2*2 = 156 ~ 625
		ret = BST_QR;
	}
	else if (100*dx0 >= center_offset_thres_x && 100*dx0 < (3 * center_offset_thres_x) && 100*dy0 >= center_offset_thres_y && 100*dy0 < (3 * center_offset_thres_y) &&
			(x <= width/3 || x >= 2*width/3) &&	(y <= height/3 || y >= 2*height/3))
	{
		//condition1: check two contour's center delta
		//condition2: check parent contour's center position (should be located at barcode corner 1/3)
		ret = BST_HANXIN;
	}
	else
	{

	}

	return ret;
}

/******************************************************************************
* Function Name: R_BCD_SortContours
* Description  : sort output contours
* Arguments    : number of contours
* Return Value : valid number of contours
******************************************************************************/
uint32_t R_BCD_SortContours(R_Rect *p_rect, uint32_t contours_no)
{

	uint32_t i;
	uint32_t j;
	uint32_t valid_contours = 0;
	R_Rect buff;
	uint32_t area2;

	/* data sort */
	//Descending Order sort
	for(i=0; i<contours_no; i++)
	{
		app_output_data_contour_child[i].area = p_rect[i].width * p_rect[i].height;
		for(j=i+1; j<contours_no; j++)
		{
			area2 = p_rect[j].width * p_rect[j].height;
			if(app_output_data_contour_child[i].area < area2)
			{
				buff = p_rect[i];
				p_rect[i] = p_rect[j];
				p_rect[j] = buff;
				app_output_data_contour_child[i].area = area2;
			}
		}
	}
	/* eliminate small area contour */
	for(i=0; i<contours_no; i++)
	{
		if(app_output_data_contour_child[i].area >= DRAW_CONTOURS_THRESHOLD_AREA)
		{
			valid_contours++;
		}
	}
	return valid_contours;
}

barcode_type_t R_BCD_FindPositionSymbo(R_Rect *p_rect, uint32_t num, uint16_t width, uint16_t height)
{
	barcode_type_t ret = BARCODE_UNKNOWN;
	uint32_t i, j;
	R_BARCODE_SYMBOL_TYPE symbol1, symbol2;
	uint8_t symbol_num = 0;

	memset((uint8_t*)&g_position_symbol[0], -1, 32);

	//get child rect number and save result in app_output_data_contour_child
	find_child_rect(p_rect, num);
	symbol2 = BST_INVALID;

	for (i=0; i<num; i++)
	{
		symbol1 = BST_INVALID;

		switch (app_output_data_contour_child[i].total_child)
		{
		case 1:
			symbol1 = symbol_detect_child_1(p_rect, i, width, height);
			break;
		case 2:
			symbol1 = symbol_detect_child_2(p_rect, i, width, height);
			break;
		default:
			break;
		}

		if (symbol1 == BST_INVALID)
			continue;
		if (symbol2 == BST_INVALID)
			symbol2 = symbol1;
		if (symbol1 != symbol2)
			return BARCODE_UNKNOWN;

		g_position_symbol[symbol_num] = i;
		symbol_num++;
	}

	if (symbol2 == BST_QR)
	{
		ret = check_qr_symbol(p_rect, symbol_num, width, height);
	}
	else if (symbol2 == BST_HANXIN)
	{
		ret = check_hanxin_symbol(p_rect, symbol_num, width, height);
	}
	else
	{
		return BARCODE_UNKNOWN;
	}

	return ret;
}

