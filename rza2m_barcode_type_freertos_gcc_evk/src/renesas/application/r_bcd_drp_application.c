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

#include <string.h>

#include "r_bcd_drp_application.h"
#include "r_bcd_lcd.h"
#include "perform.h"
#include "r_bcd_main.h"
#include "r_mmu_lld_rza2m.h"
#include "r_cache_lld_rza2m.h"

#define BARCODE_SQUARE_MIN		((R_BCD_CAMERA_WIDTH/DRP_RESIZE/5)*(R_BCD_CAMERA_HEIGHT/DRP_RESIZE/5))
#define SCREEN_CENTER_X			(R_BCD_CAMERA_WIDTH/DRP_RESIZE/2)
#define SCREEN_CENTER_Y			(R_BCD_CAMERA_HEIGHT/DRP_RESIZE/2)


typedef struct {
	uint16_t top;
	uint16_t left;
	uint16_t bottom;
	uint16_t right;
}R_Rect_Pos;


typedef void(*CB)();


/*******************************************************************************
Imported global variables and functions (from other files)
*******************************************************************************/
extern uint8_t g_drp_lib_bayer2gray_thinning[];
//extern uint8_t g_drp_lib_gaussian_blur[];
//extern uint8_t g_drp_lib_binarization_fixed[];
extern uint8_t g_drp_lib_binarization[];
extern uint8_t g_drp_lib_find_contours[];
extern uint8_t g_drp_lib_dilate[];
//extern uint8_t g_drp_lib_keystone_correction[];
extern uint8_t g_drp_lib_bayer2gray_cropping[];
//extern uint8_t g_drp_lib_affine[];
extern uint8_t g_drp_lib_binarization_adaptive_bit[];
extern uint8_t g_drp_lib_reed_solomon[];

/*******************************************************************************
Private global variables and functions
*******************************************************************************/
static uint8_t drp_lib_id[R_DK2_TILE_NUM] = {0};
static volatile uint8_t drp_lib_status[R_DK2_TILE_NUM] = {DRP_NOT_FINISH};
static r_drp_bayer2gray_thinning_t		param_b2graythinning[R_DK2_TILE_NUM] __attribute__ ((section("UNCACHED_BSS")));
//static r_drp_gaussian_blur_t        	param_gaussian[R_DK2_TILE_NUM] __attribute__ ((section("UNCACHED_BSS")));
//static r_drp_binarization_fixed_t		param_binarization_fixed[R_DK2_TILE_NUM] __attribute__ ((section("UNCACHED_BSS")));
static r_drp_binarization_adaptive_custom_t	param_binarization __attribute__ ((section("UNCACHED_BSS")));
static r_drp_find_contours_t      		param_findc   __attribute__ ((section("UNCACHED_BSS")));
static r_drp_find_contours_dst_rect_t 	param_findc_rect[DRP_R_FINDC_BUFF_RECT_NUM]  __attribute__ ((section("UNCACHED_BSS")));
static r_drp_find_contours_dst_region_t	param_findc_region[DRP_R_FINDC_BUFF_REGION_NUM]  __attribute__ ((section("UNCACHED_BSS")));
static r_drp_dilate_t					param_dilate[R_DK2_TILE_NUM] __attribute__ ((section("UNCACHED_BSS")));
//static r_drp_keystone_correction_t		param_keystone_correction __attribute__ ((section("UNCACHED_BSS")));
static r_drp_bayer2gray_cropping_t		param_bayer2gray_cropping __attribute__ ((section("UNCACHED_BSS")));
//static r_drp_affine_t					param_affine __attribute__ ((section("UNCACHED_BSS")));
static r_drp_binarization_adaptive_bit_t param_bin __attribute__ ((section("UNCACHED_BSS")));
static r_drp_reed_solomon_t 			param_rs __attribute__ ((section("UNCACHED_BSS")));

static void cb_drp_finish(uint8_t id);

static uint8_t drp_work_uncache[256] __attribute__ ((section("UNCACHED_BSS")));
static uint8_t drp_binarization_work_ram[((R_BCD_CAMERA_WIDTH * R_BCD_CAMERA_HEIGHT)/64)+2];
static uint32_t ReedsolomonTime = 0;

extern uint32_t ave_result[6] __attribute__ ((section("UNCACHED_BSS")));
extern R_Contour    app_contours[2];

/*******************************************************************************
* Function Name: cb_drp_finish
* Description  : This function is a callback function called from the
*              : DRP driver at the finish of the DRP library processing.
* Arguments    : id
*              :   The ID of the DRP library that finished processing.
* Return Value : -
*******************************************************************************/
static void cb_drp_finish(uint8_t id)
{
    uint32_t tile_no;

    /* Change the operation state of the DRP library notified by the argument to finish */
    for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
    {
        if (drp_lib_id[tile_no] == id)
        {
            drp_lib_status[tile_no] = DRP_FINISH;
            break;
        }
    }

    return;
}


/* Function Name: R_BCD_DRP_Bayer2Gray_thinning
 * Description  : Bayer to Gray and tinning Processing on DRP
 * Arguments    : input_address
 * 					Address of input image
 * 				 output_address
 * 				 	Address of output image
 * 				 input_width
 * 				 	width of input data image
 * 				 input_height
 * 				 	height of input data image
 * Return Value : -
 ******************************************************************************/
void R_BCD_DRP_Bayer2Gray_thinning(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height)
{
    int32_t ret_val;
    uint32_t tile_no;

	PerformSetStartTime(TIME_B2G_DL);
    /*********************************/
    /* Load DRP Library              */
    /*        +--------------------+ */
    /* tile 0 | Bayer2Gray_thinning| */
    /*        +--------------------+ */
    /* tile 1 | Bayer2Gray_thinning| */
    /*        +--------------------+ */
    /* tile 2 | Bayer2Gray_thinning| */
    /*        +--------------------+ */
    /* tile 3 | Bayer2Gray_thinning| */
    /*        +--------------------+ */
    /* tile 4 | Bayer2Gray_thinning| */
    /*        +--------------------+ */
    /* tile 5 | Bayer2Gray_thinning| */
    /*        +--------------------+ */
    /*********************************/
	ret_val = R_DK2_Load(&g_drp_lib_bayer2gray_thinning[0],
			R_DK2_TILE_0 | R_DK2_TILE_1 | R_DK2_TILE_2 | R_DK2_TILE_3 | R_DK2_TILE_4 | R_DK2_TILE_5,
			R_DK2_TILE_PATTERN_1_1_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2] | drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5], 0);
	DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_B2G_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_B2G_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/
	for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
	{
		/* Set the address of buffer to be read/write by DRP */
		param_b2graythinning[tile_no].src = input_address  + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
		param_b2graythinning[tile_no].dst = output_address + (input_width/DRP_RESIZE * (input_height/DRP_RESIZE / R_DK2_TILE_NUM)) * tile_no;
		R_MMU_VAtoPA((uint32_t)&ave_result[tile_no], (uint32_t*)&param_b2graythinning[tile_no].accumulate);

		/* Set Image size */
		param_b2graythinning[tile_no].width  = (uint16_t)input_width;
		param_b2graythinning[tile_no].height = (uint16_t)input_height / R_DK2_TILE_NUM;
		param_b2graythinning[tile_no].area1_offset_x = 0;
		param_b2graythinning[tile_no].area1_offset_y = 0;
		param_b2graythinning[tile_no].area1_width = input_width / DRP_RESIZE;
		param_b2graythinning[tile_no].area1_height = input_height / DRP_RESIZE / R_DK2_TILE_NUM;

		/* Initialize variables to be used in termination judgment of the DRP application */
		drp_lib_status[tile_no] = DRP_NOT_FINISH;

		/*********************/
		/* Start DRP Library */
		/*********************/
		ret_val = R_DK2_Start(drp_lib_id[tile_no], (void *)&param_b2graythinning[tile_no], sizeof(r_drp_bayer2gray_thinning_t));
		DRP_DRV_ASSERT(ret_val);
	}

	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
	{
		while (drp_lib_status[tile_no] == DRP_NOT_FINISH);
	}

	for (tile_no = 1; tile_no < R_DK2_TILE_NUM; tile_no++)
	{
		ave_result[0] += ave_result[tile_no];
	}

	/* Set end time of process */
	PerformSetEndTime(TIME_B2G_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2]
						| drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5], &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);
}

#if 0
/******************************************************************************
* Function Name: R_BCD_DRP_GaussianBlur
* Description  : Gaussian Filter Processing on DRP
* Arguments    : input_address
* 					Address of input image
* 				 output_address
* 				 	Address of output image
* 				 input_width
* 				 	width of input data image
* 				 input_height
* 				 	height of input data image
* Return Value : -
******************************************************************************/
void R_BCD_DRP_GaussianBlur(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint8_t times)
{
    int32_t ret_val;
    uint32_t tile_no;
    uint8_t loop;

    if(times == 0)
    {
    	return;
    }
	/**************************/
	/* Load DRP Library       */
	/*        +-------------+ */
	/* tile 0 |Gaussian blur| */
	/*        +-------------+ */
	/* tile 1 |Gaussian blur| */
	/*        +-------------+ */
	/* tile 2 |Gaussian blur| */
	/*        +-------------+ */
	/* tile 3 |Gaussian blur| */
	/*        +-------------+ */
	/* tile 4 |Gaussian blur| */
	/*        +-------------+ */
	/* tile 5 |Gaussian blur| */
	/*        +-------------+ */
	/**************************/
	PerformSetStartTime(TIME_GAUSSIAN_DL);

	ret_val = R_DK2_Load(&g_drp_lib_gaussian_blur[0],
			R_DK2_TILE_0 | R_DK2_TILE_1 | R_DK2_TILE_2 | R_DK2_TILE_3 | R_DK2_TILE_4 | R_DK2_TILE_5,
			R_DK2_TILE_PATTERN_1_1_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2] |
			drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5], 0);
	//DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_GAUSSIAN_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_GAUSSIAN_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/
	for (loop =0; loop < times; loop++)
	{
		for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
		{
			if(loop%2 == 0)
			{
				/* Set the address of buffer to be read/write by DRP */
				param_gaussian[tile_no].src = input_address  + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
				param_gaussian[tile_no].dst = output_address + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;

				/* Set Image size */
				param_gaussian[tile_no].width  = (uint16_t)input_width;
				param_gaussian[tile_no].height = (uint16_t)input_height / R_DK2_TILE_NUM;

				/* Set whether to perform top or bottom edge border processing. */
				param_gaussian[tile_no].top    = (tile_no == TILE_0) ? 1 : 0;
				param_gaussian[tile_no].bottom = (tile_no == TILE_5) ? 1 : 0;

				/* Initialize variables to be used in termination judgment of the DRP library */
				drp_lib_status[tile_no] = DRP_NOT_FINISH;

				/*********************/
				/* Start DRP Library */
				/*********************/
				ret_val = R_DK2_Start(drp_lib_id[tile_no], (void *)&param_gaussian[tile_no], sizeof(r_drp_gaussian_blur_t));
				//DRP_DRV_ASSERT(ret_val);
			}
			else
			{
				/* Set the address of buffer to be read/write by DRP */
				param_gaussian[tile_no].src = output_address  + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
				param_gaussian[tile_no].dst = input_address + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;

				/* Set Image size */
				param_gaussian[tile_no].width  = (uint16_t)input_width;
				param_gaussian[tile_no].height = (uint16_t)input_height / R_DK2_TILE_NUM;

				/* Set whether to perform top or bottom edge border processing. */
				param_gaussian[tile_no].top    = (tile_no == TILE_0) ? 1 : 0;
				param_gaussian[tile_no].bottom = (tile_no == TILE_5) ? 1 : 0;

				/* Initialize variables to be used in termination judgment of the DRP library */
				drp_lib_status[tile_no] = DRP_NOT_FINISH;

				/*********************/
				/* Start DRP Library */
				/*********************/
				ret_val = R_DK2_Start(drp_lib_id[tile_no], (void *)&param_gaussian[tile_no], sizeof(r_drp_gaussian_blur_t));
				//DRP_DRV_ASSERT(ret_val);
			}
		}

		/***************************************/
		/* Wait until DRP processing is finish */
		/***************************************/
		for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
		{
			while (drp_lib_status[tile_no] == DRP_NOT_FINISH);
		}

	}
    /* Set end time of process */
    PerformSetEndTime(TIME_GAUSSIAN_EXE);

    /**********************/
    /* Unload DRP library */
    /**********************/
    ret_val = R_DK2_Unload(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2] | drp_lib_id[TILE_3] |
    		drp_lib_id[TILE_4] | drp_lib_id[TILE_5], &drp_lib_id[TILE_0]);
    //DRP_DRV_ASSERT(ret_val);
}
#endif

void R_BCD_DRP_Binarization(uint32_t input_address, uint32_t output_address, uint32_t work_address, uint32_t input_width, uint32_t input_height, uint8_t range)
{
	int32_t ret_val;

	PerformSetStartTime(TIME_BINARIZATION_DL);
	/******************************/
	/* Load DRP Library           */
	/*        +-----------------+ */
	/* tile 0 |                 | */
	/*        +                 + */
	/* tile 1 | Binarization    | */
	/*        +                 + */
	/* tile 2 |                 | */
	/*        +-----------------+ */
	/* tile 3 |                 | */
	/*        +-----------------+ */
	/* tile 4 |                 | */
	/*        +-----------------+ */
	/* tile 5 |                 | */
	/*        +-----------------+ */
	/******************************/
	ret_val = R_DK2_Load(&g_drp_lib_binarization[0],
			R_DK2_TILE_0,
			R_DK2_TILE_PATTERN_3_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0], 0);
	//DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_BINARIZATION_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_BINARIZATION_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/

	/* Set the address of buffer to be read/write by DRP */
	param_binarization.src = input_address;
	param_binarization.dst = output_address;
	param_binarization.work = work_address;

	/* Set Image size */
	param_binarization.width  = (uint16_t)input_width & (~31);
	param_binarization.height = (uint16_t)input_height & (~7);
	param_binarization.range = range;
	param_binarization.invert = 1;

	/* Initialize variables to be used in termination judgment of the DRP application */
	drp_lib_status[TILE_0] = DRP_NOT_FINISH;

	/*********************/
	/* Start DRP Library */
	/*********************/
	ret_val = R_DK2_Start(drp_lib_id[TILE_0], (void *)&param_binarization, sizeof(r_drp_binarization_adaptive_custom_t));
	//DRP_DRV_ASSERT(ret_val);


	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	while (drp_lib_status[TILE_0] == DRP_NOT_FINISH);

	/* Set end time of process */
	PerformSetEndTime(TIME_BINARIZATION_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0], &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);
}

/******************************************************************************
* Function Name: R_BCD_DRP_Dilate
* Description  : Dilate Processing on DRP
* Arguments    : input_address
* 					Address of input image
* 				 output_address
* 				 	Address of output image
* 				 input_width
* 				 	width of input data image
* 				 input_height
* 				 	height of input data image
* 				 times
* 				 	number of runs
* Return Value : -
******************************************************************************/
void R_BCD_DRP_Dilate(uint32_t input_address, uint32_t output_address1, uint32_t output_address2, uint32_t input_width, uint32_t input_height, uint8_t times)
{
    int32_t ret_val;
    uint32_t tile_no;
    uint8_t loop;
    if(times == 0)
    {
    	return;
    }

	/**************************/
	/* Load DRP Library       */
	/*        +-------------+ */
	/* tile 0 |   Dilate    | */
	/*        +-------------+ */
	/* tile 1 |   Dilate    | */
	/*        +-------------+ */
	/* tile 2 |   Dilate    | */
	/*        +-------------+ */
	/* tile 3 |   Dilate    | */
	/*        +-------------+ */
	/* tile 4 |   Dilate    | */
	/*        +-------------+ */
	/* tile 5 |   Dilate    | */
	/*        +-------------+ */
	/**************************/
	PerformSetStartTime(TIME_DILATE_DL);

	ret_val = R_DK2_Load(&g_drp_lib_dilate[0],
			R_DK2_TILE_0 | R_DK2_TILE_1 | R_DK2_TILE_2 | R_DK2_TILE_3 | R_DK2_TILE_4 | R_DK2_TILE_5,
			R_DK2_TILE_PATTERN_1_1_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2] |
			drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5], 0);
	//DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_DILATE_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_DILATE_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/
	for (loop =0; loop < times; loop++){
		for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
		{
			if(loop == 0)
			{
				/* Set the address of buffer to be read/write by DRP */
				param_dilate[tile_no].src = input_address  + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
				param_dilate[tile_no].dst = output_address1 + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;

			}
			else
			{
				if (loop%2 == 1)
				{
					/* Set the address of buffer to be read/write by DRP */
					param_dilate[tile_no].src = output_address1  + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
					param_dilate[tile_no].dst = output_address2 + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;

				}
				else
				{
					/* Set the address of buffer to be read/write by DRP */
					param_dilate[tile_no].src = output_address2  + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
					param_dilate[tile_no].dst = output_address1 + (input_width * (input_height / R_DK2_TILE_NUM)) * tile_no;
				}
			}
			/* Set Image size */
			param_dilate[tile_no].width  = (uint16_t)input_width;
			param_dilate[tile_no].height = (uint16_t)input_height / R_DK2_TILE_NUM;

			/* Set whether to perform top or bottom edge border processing. */
			param_dilate[tile_no].top    = (tile_no == TILE_0) ? 1 : 0;
			param_dilate[tile_no].bottom = (tile_no == TILE_5) ? 1 : 0;

			/* Initialize variables to be used in termination judgment of the DRP library */
			drp_lib_status[tile_no] = DRP_NOT_FINISH;

			/*********************/
			/* Start DRP Library */
			/*********************/
			ret_val = R_DK2_Start(drp_lib_id[tile_no], (void *)&param_dilate[tile_no], sizeof(r_drp_dilate_t));
			//DRP_DRV_ASSERT(ret_val);
		}

		/***************************************/
		/* Wait until DRP processing is finish */
		/***************************************/
		for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
		{
			while (drp_lib_status[tile_no] == DRP_NOT_FINISH);
		}
	}
    /* Set end time of process */
    PerformSetEndTime(TIME_DILATE_EXE);

    /**********************/
    /* Unload DRP library */
    /**********************/
    ret_val = R_DK2_Unload(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2] |
    		drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5], &drp_lib_id[TILE_0]);
    //DRP_DRV_ASSERT(ret_val);
}


/* Function Name: R_BCD_DRP_Bayer2Gray_Cropping
 * Description  : Cropping Processing on DRP
 * Arguments    : input_address
 * 					Address of input image
 * 				 output_address
 * 				 	Address of output image
 * 				 input_width
 * 				 	width of input data image
 * 				 input_height
 * 				 	height of input data image
 * 				 offset_x
 * 				    x coordinate input image
 * 				 offset_y
 * 				 	y coordinate input image
 * 				 output_width
 * 				 	Output image width (pixels)
 * 				 output_height
 * 				 	Output image height (height)
 * Return Value : -
 ******************************************************************************/
#if 0
void R_BCD_DRP_Bayer2Gray_Cropping(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint32_t offset_x, uint32_t offset_y, uint32_t output_width, uint32_t output_height)
{
    int32_t ret_val;
    uint32_t tile_no;

	PerformSetStartTime(TIME_CROPPING_DL);
    /*********************************/
    /* Load DRP Library              */
    /*        +--------------------+ */
    /* tile 0 | Cropping           | */
    /*        +--------------------+ */
    /* tile 1 |                    | */
    /*        +--------------------+ */
    /* tile 2 |                    | */
    /*        +--------------------+ */
    /* tile 3 |                    | */
    /*        +--------------------+ */
    /* tile 4 |                    | */
    /*        +--------------------+ */
    /* tile 5 |                    | */
    /*        +--------------------+ */
    /*********************************/
	ret_val = R_DK2_Load(&g_drp_lib_bayer2gray_cropping[0],
			R_DK2_TILE_0 ,
			R_DK2_TILE_PATTERN_1_1_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] , 0);
	DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_CROPPING_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_CROPPING_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/

	/* Set the address of buffer to be read/write by DRP */
	param_bayer2gray_cropping.src = input_address;
	param_bayer2gray_cropping.dst = output_address;

	param_bayer2gray_cropping.accumulate = 0;
	param_bayer2gray_cropping.width = input_width;
	param_bayer2gray_cropping.height = input_height;

	param_bayer2gray_cropping.area_offset_x = offset_x;
	param_bayer2gray_cropping.area_offset_y = offset_y;
	param_bayer2gray_cropping.area_width = output_width;
	param_bayer2gray_cropping.area_height = output_height;


	/* Initialize variables to be used in termination judgment of the DRP application */
	drp_lib_status[TILE_0] = DRP_NOT_FINISH;

	/*********************/
	/* Start DRP Library */
	/*********************/
	ret_val = R_DK2_Start(drp_lib_id[TILE_0], (void *)&param_bayer2gray_cropping, sizeof(r_drp_bayer2gray_cropping_t));
	DRP_DRV_ASSERT(ret_val);


	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	while (drp_lib_status[TILE_0] == DRP_NOT_FINISH);

	/* Set end time of process */
	PerformSetEndTime(TIME_CROPPING_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0] , &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);
}
#else
void R_BCD_DRP_Bayer2Gray_Cropping(uint32_t input_address, uint32_t output_address, uint32_t input_width, uint32_t input_height, uint32_t offset_x, uint32_t offset_y, uint32_t output_width, uint32_t output_height)
{
    int32_t ret_val;
    uint32_t tile_no;
//    uint8_t block[6];
//    uint8_t tiles = 0;
//    uint16_t start_y;
//    uint16_t tile_height;
//    uint8_t lib_id = 0;
//    uint32_t dst_offset = 0;
//    uint16_t remaining_height = 0;
//    uint32_t dst_len[6];
    //This is 6 tiles cropping, make sure input_address is not same with output_address
    if (input_address == output_address)
    	return;

	PerformSetStartTime(TIME_CROPPING_DL);
    /*********************************/
    /* Load DRP Library              */
    /*        +--------------------+ */
    /* tile 0 | Cropping           | */
    /*        +--------------------+ */
    /* tile 1 | Cropping           | */
    /*        +--------------------+ */
    /* tile 2 | Cropping           | */
    /*        +--------------------+ */
    /* tile 3 | Cropping           | */
    /*        +--------------------+ */
    /* tile 4 | Cropping           | */
    /*        +--------------------+ */
    /* tile 5 | Cropping           | */
    /*        +--------------------+ */
    /*********************************/
	ret_val = R_DK2_Load(&g_drp_lib_bayer2gray_cropping[0],
			R_DK2_TILE_0 | R_DK2_TILE_1 | R_DK2_TILE_2 | R_DK2_TILE_3 | R_DK2_TILE_4 | R_DK2_TILE_5,
			R_DK2_TILE_PATTERN_1_1_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);

	DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2] | drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5] , 0);
	DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_CROPPING_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_CROPPING_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/
	for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
	{
		/* Set the address of buffer to be read/write by DRP */
		param_bayer2gray_cropping.src = input_address;
		param_bayer2gray_cropping.accumulate = 0;
		param_bayer2gray_cropping.width = input_width;
		param_bayer2gray_cropping.height = input_height;
		param_bayer2gray_cropping.area_offset_x = offset_x;
		param_bayer2gray_cropping.area_width = output_width;

		param_bayer2gray_cropping.area_offset_y = offset_y + tile_no*output_height/R_DK2_TILE_NUM;
		param_bayer2gray_cropping.area_height = output_height/R_DK2_TILE_NUM;
		param_bayer2gray_cropping.dst = output_address + tile_no * output_height/R_DK2_TILE_NUM * output_width;

		/* Initialize variables to be used in termination judgment of the DRP application */
		drp_lib_status[tile_no] = DRP_NOT_FINISH;

		/*********************/
		/* Start DRP Library */
		/*********************/
		ret_val = R_DK2_Start(drp_lib_id[tile_no], (void *)&param_bayer2gray_cropping, sizeof(r_drp_bayer2gray_cropping_t));
		DRP_DRV_ASSERT(ret_val);
	}

	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
	{
		while (drp_lib_status[tile_no] == DRP_NOT_FINISH);
	}

	/* Set end time of process */
	PerformSetEndTime(TIME_CROPPING_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0] | drp_lib_id[TILE_1] | drp_lib_id[TILE_2]
			| drp_lib_id[TILE_3] | drp_lib_id[TILE_4] | drp_lib_id[TILE_5] , &drp_lib_id[0]);
}
#endif

void R_BCD_DRP_FindContours1(uint32_t input_address, uint32_t work_address, uint16_t input_width, uint16_t input_height, void* cb, uint8_t frame_id)
{
    int32_t ret_val;
    uint32_t tile_no;
    uint32_t findc_region_bufadr;
    uint32_t findc_rect_bufadr;
    CB callback = cb;
	/******************************/
	/*      Load DRP Library      */
	/*        +-----------------+ */
	/* tile 0 |                 | */
	/*        +  FindContours   + */
	/* tile 1 |    (2 Tile)     | */
	/*        +-----------------+ */
	/* tile 2 |                 | */
	/*        +                 + */
	/* tile 3 |                 | */
	/*        +                 + */
	/* tile 4 |                 | */
	/*        +                 + */
	/* tile 5 |                 | */
	/*        +-----------------+ */
	/******************************/
    PerformSetStartTime(TIME_FINDC_DL);

	ret_val = R_DK2_Load(&g_drp_lib_find_contours[0],
							R_DK2_TILE_0,
							R_DK2_TILE_PATTERN_2_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	//DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0], 0);
	//DRP_DRV_ASSERT(ret_val);

	/* Set start time of process(DL)*/
    PerformSetEndTime(TIME_FINDC_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_FINDC_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameter  */
	/***************************************/
	/* Set the address of buffer to be read/write by DRP */
	R_MMU_VAtoPA((uint32_t)&param_findc_region[0], (uint32_t*)&findc_region_bufadr );
	R_MMU_VAtoPA((uint32_t)&param_findc_rect[0], (uint32_t*)&findc_rect_bufadr );

	param_findc.src        = input_address;
	param_findc.dst_region = findc_region_bufadr;
	param_findc.dst_rect   = findc_rect_bufadr;
	param_findc.work       = work_address;

	/* Set Image size */
	param_findc.width  = input_width;
	param_findc.height = input_height;

	/* Set destination size(rectangle/region) */
	param_findc.dst_region_size  = DRP_R_FINDC_BUFF_REGION_NUM;
	param_findc.dst_rect_size    = DRP_R_FINDC_BUFF_RECT_NUM;

	/* Set threshold size */
	if (frame_id == 0)
	{
		param_findc.threshold_width  = input_width / 5;
		param_findc.threshold_height = input_height / 5;
	}
	else
	{
		param_findc.threshold_width  = DRP_R_FINDC_WIDTH_THRESHOLD;
		param_findc.threshold_height = DRP_R_FINDC_HEIGHT_THRESHOLD;
	}

	/* Initialize variables to be used in termination judgment of the DRP application */
	drp_lib_status[TILE_0] = DRP_NOT_FINISH;

	/*********************/
	/* Start DRP Library */
	/*********************/
	ret_val = R_DK2_Start(drp_lib_id[TILE_0], (void *)&param_findc, sizeof(r_drp_find_contours_t));
	//DRP_DRV_ASSERT(ret_val);

	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	while (drp_lib_status[TILE_0] == DRP_NOT_FINISH)
	{
		if (callback != NULL)
		{
			//R_BCD_Histogram_HV((uint8_t*)input_address, input_width, input_height);
			callback((uint8_t*)input_address, input_width, input_height);
			callback = NULL; //only run once
		}
	}

	/* Set end time of process */
	PerformSetEndTime(TIME_FINDC_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0], &drp_lib_id[TILE_0]);
	//DRP_DRV_ASSERT(ret_val);

}

void R_BCD_DRP_FindContours(uint32_t input_address, uint32_t work_address, uint16_t input_width, uint16_t input_height, void* cb, uint8_t frame_id)
{
    int32_t ret_val;
    uint32_t tile_no;
    uint32_t findc_region_bufadr;
    uint32_t findc_rect_bufadr;
    CB callback = cb;
    uint32_t loop, use_6tiles;
    uint32_t i,j;
    uint16_t height;
	/******************************/
	/*      Load DRP Library      */
	/*        +-----------------+ */
	/* tile 0 |                 | */
	/*        +  FindContours   + */
	/* tile 1 |    (2 Tile)     | */
	/*        +-----------------+ */
	/* tile 2 |if frame_id == 1 | */
	/*        +  FindContours   + */
	/* tile 3 |    (2 Tile)     | */
	/*        +                 + */
	/* tile 4 |if frame_id == 1 | */
	/*        +  FindContours   + */
	/* tile 5 |    (2 Tile)     | */
	/*        +-----------------+ */
	/******************************/
    PerformSetStartTime(TIME_FINDC_DL);

    //TODO 6 tiles find contour can improve the performance
    //But in MicroQR case, some barcode symbol is larger then 1/3 whole barcode image
    //Use 2 tiles here
//    if (frame_id == 1 && (input_height / 3) > 32)
//    {
//    	use_6tiles = 1;
//    }
//    else
    {
    	use_6tiles = 0;
    }
    if (use_6tiles == 1)
    {
    	ret_val = R_DK2_Load(&g_drp_lib_find_contours[0],
							R_DK2_TILE_0 | R_DK2_TILE_2 | R_DK2_TILE_4,
							R_DK2_TILE_PATTERN_2_2_2, NULL, &cb_drp_finish, &drp_lib_id[0]);
    	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] | drp_lib_id[TILE_2] | drp_lib_id[TILE_4], 0);
    	loop = 3;
    	height = input_height / 3;
    }
    else
    {
    	ret_val = R_DK2_Load(&g_drp_lib_find_contours[0],
								R_DK2_TILE_0,
								R_DK2_TILE_PATTERN_2_1_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
		ret_val = R_DK2_Activate(drp_lib_id[TILE_0], 0);
		loop = 1;
		height = input_height;
		//DRP_DRV_ASSERT(ret_val);
    }
	//DRP_DRV_ASSERT(ret_val);

	/* Set start time of process(DL)*/
    PerformSetEndTime(TIME_FINDC_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_FINDC_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameter  */
	/***************************************/
	for (tile_no=0; tile_no<loop; tile_no++)
	{
		/* Set the address of buffer to be read/write by DRP */
		R_MMU_VAtoPA((uint32_t)&param_findc_region[tile_no*DRP_R_FINDC_BUFF_REGION_NUM/loop], (uint32_t*)&findc_region_bufadr );
		R_MMU_VAtoPA((uint32_t)&param_findc_rect[tile_no*DRP_R_FINDC_BUFF_RECT_NUM/loop], (uint32_t*)&findc_rect_bufadr );

		param_findc.src        = input_address + tile_no*height*input_width;
		param_findc.dst_region = findc_region_bufadr;
		param_findc.dst_rect   = findc_rect_bufadr;
		param_findc.work       = work_address + tile_no*height*input_width;

		/* Set Image size */
		param_findc.width  = input_width;
		param_findc.height = (tile_no == 2) ? (input_height - 2 * height) : height;

		/* Set destination size(rectangle/region) */
		param_findc.dst_region_size  = DRP_R_FINDC_BUFF_REGION_NUM / loop;
		param_findc.dst_rect_size    = DRP_R_FINDC_BUFF_RECT_NUM / loop;

		/* Set threshold size */
		if (frame_id == 0)
		{
			param_findc.threshold_width  = input_width / 5;
			param_findc.threshold_height = input_height / 5;
		}
		else
		{
			param_findc.threshold_width  = DRP_R_FINDC_WIDTH_THRESHOLD;
			param_findc.threshold_height = DRP_R_FINDC_HEIGHT_THRESHOLD;
		}

		/* Initialize variables to be used in termination judgment of the DRP application */
		drp_lib_status[tile_no*2] = DRP_NOT_FINISH;

		/*********************/
		/* Start DRP Library */
		/*********************/
		ret_val = R_DK2_Start(drp_lib_id[tile_no*2], (void *)&param_findc, sizeof(r_drp_find_contours_t));
		//DRP_DRV_ASSERT(ret_val);
	}
	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	for (tile_no = 0; tile_no < loop; tile_no++)
	{
		while (drp_lib_status[2*tile_no] == DRP_NOT_FINISH)
		{
			if (callback != NULL)
			{
				//R_BCD_Histogram_HV((uint8_t*)input_address, input_width, input_height);
				callback((uint8_t*)input_address, input_width, input_height);
				callback = NULL; //only run once
			}
		}
	}

	/* Set end time of process */
	PerformSetEndTime(TIME_FINDC_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	if (use_6tiles == 1)
	{
		ret_val = R_DK2_Unload(drp_lib_id[TILE_0] | drp_lib_id[TILE_2] | drp_lib_id[TILE_4], &drp_lib_id[TILE_0]);
	}
	else
	{
		ret_val = R_DK2_Unload(drp_lib_id[TILE_0], &drp_lib_id[TILE_0]);
	}
	//DRP_DRV_ASSERT(ret_val);

	i = 0;
	if (use_6tiles == 0)
	{
		while(R_BCD_DrpCheckEmptyContours(i) && i<DRP_R_FINDC_BUFF_RECT_NUM)
		{
			app_contours[frame_id].rect[i].x = (int)param_findc_rect[i].x_coordinate;
			app_contours[frame_id].rect[i].y = (int)param_findc_rect[i].y_coordinate;
			app_contours[frame_id].rect[i].width = (int)param_findc_rect[i].width;
			app_contours[frame_id].rect[i].height = (int)param_findc_rect[i].height;
			i++;
		}
		app_contours[frame_id].contours_no = i;
	}
	else
	{
		//merge find contour result
		uint32_t j;
		for (uint32_t tile_no=0; tile_no<3; tile_no++)
		{
			j = tile_no*DRP_R_FINDC_BUFF_RECT_NUM/3;
			while(R_BCD_DrpCheckEmptyContours(j) && j < (tile_no+1)*DRP_R_FINDC_BUFF_RECT_NUM/3)
			{
				app_contours[frame_id].rect[i].x = (int)param_findc_rect[j].x_coordinate;
				app_contours[frame_id].rect[i].y = (int)param_findc_rect[j].y_coordinate + tile_no*height;
				app_contours[frame_id].rect[i].width = (int)param_findc_rect[j].width;
				app_contours[frame_id].rect[i].height = (int)param_findc_rect[j].height;
				i++;
				j++;
			}
		}
		app_contours[frame_id].contours_no = i;
	}

}

/******************************************************************************
* Function Name: R_BCD_DrpCleanContours
* Description  : Clear output contours buffer
* Arguments    : -
* Return Value : -
******************************************************************************/
void R_BCD_DrpCleanContours(void)
{
	memset(&param_findc_rect[0], 0, sizeof(param_findc_rect));
	memset(&param_findc_region[0], 0, sizeof(param_findc_region));
}

/******************************************************************************
* Function Name: R_BCD_DrpResizeContours
* Description  : Resize output contours fit Camera size
* Arguments    : -
* Return Value : -
******************************************************************************/
uint32_t R_BCD_DrpResizeContours(uint8_t frame_id)
{
	uint32_t i;

	 i = 0;

	/* resize size */
	while(R_BCD_DrpCheckEmptyContours(i))
	{
		param_findc_rect[i].x_coordinate  = (uint16_t)((param_findc_rect[i].x_coordinate  * DRP_RESIZE) - DRP_RESIZE);
		param_findc_rect[i].y_coordinate  = (uint16_t)((param_findc_rect[i].y_coordinate  * DRP_RESIZE) - DRP_RESIZE);
		param_findc_rect[i].width         = (uint16_t)((param_findc_rect[i].width  * DRP_RESIZE) + DRP_RESIZE);
		param_findc_rect[i].height        = (uint16_t)((param_findc_rect[i].height * DRP_RESIZE) + DRP_RESIZE);
		i++;
	}
	return i;
}

uint32_t R_BCD_DrpCheckEmptyContours(uint32_t i)
{
	if (i >= DRP_R_FINDC_BUFF_RECT_NUM)
		return 0;

	if(param_findc_rect[i].x_coordinate)
	{
		return 1;
	}
	if(param_findc_rect[i].width)
	{
		return 1;
	}
	if(param_findc_rect[i].height)
	{
		return 1;
	}
	if(param_findc_rect[i].y_coordinate)
	{
		return 1;
	}
	if(param_findc_rect[i].count)
	{
		return 1;
	}
	if(param_findc_rect[i].addr)
	{
		return 1;
	}
	//printf("#-#-#-#-# contours is empty!!#-#-#-#-#\n");
	return 0;
}

/******************************************************************************
 * Function Name: R_BCD_DrpDrawContours
 * Description  : Draw Contours
 * Arguments    : -
 * Return Value : -
 ******************************************************************************/
void R_BCD_DrpDrawContours(uint32_t contours_no)
{
	for(int i = 0; i < (int)contours_no; i++)
	{
		DrawRect( VDC_LAYER_ID_3_RD, (int32_t)param_findc_rect[i].x_coordinate, (int32_t)param_findc_rect[i].y_coordinate,
				(int32_t)param_findc_rect[i].width, (int32_t)param_findc_rect[i].height, RED);
		//DrawRect( VDC_LAYER_ID_3_RD, (int32_t)(param_findc_rect[i].x_coordinate-1), (int32_t)(param_findc_rect[i].y_coordinate-1),
		//		(int32_t)(param_findc_rect[i].width+2), (int32_t)(param_findc_rect[i].height+2), RED);
	}
}

void R_BCD_DrpDrawContour(uint32_t contours_no)
{
//	int i;
//	R_Point* points;
	DrawRect( VDC_LAYER_ID_3_RD, (int32_t)param_findc_rect[contours_no].x_coordinate, (int32_t)param_findc_rect[contours_no].y_coordinate,
			(int32_t)param_findc_rect[contours_no].width, (int32_t)param_findc_rect[contours_no].height, RED);
	//DrawRect( VDC_LAYER_ID_3_RD, (int32_t)(param_findc_rect[i].x_coordinate-1), (int32_t)(param_findc_rect[i].y_coordinate-1),
	//		(int32_t)(param_findc_rect[i].width+2), (int32_t)(param_findc_rect[i].height+2), RED);

}

void R_BCD_DrpDrawContourBox(R_Box box)
{
	DrawLine( VDC_LAYER_ID_3_RD, box.box[0][0], box.box[0][1], box.box[1][0], box.box[1][1], GREEN );
	DrawLine( VDC_LAYER_ID_3_RD, box.box[1][0], box.box[1][1], box.box[2][0], box.box[2][1], GREEN );
	DrawLine( VDC_LAYER_ID_3_RD, box.box[2][0], box.box[2][1], box.box[3][0], box.box[3][1], GREEN );
	DrawLine( VDC_LAYER_ID_3_RD, box.box[3][0], box.box[3][1], box.box[0][0], box.box[0][1], GREEN );
}

void R_BCD_DrpDrawContourPoints(uint32_t contours_no)
{
	R_Point* points;
	uint32_t i;
	points = (R_Point*)param_findc_rect[contours_no].addr;
	for (i=0; i<param_findc_rect[contours_no].count; i++)
	{
		DrawPoint( VDC_LAYER_ID_3_RD, points[i].x, points[i].y, RED);
	}
}

#if 0
	int left = 0, bottom = 0, right = 0, top = 0;
	int left_x, right_x, top_y, bottom_y;
	float inv_vect_length[512];
	R_Point vect[512];
static int get_contour_box(R_Box* p_box, uint32_t n, uint16_t width, uint16_t height)
{
	float minarea = width * height;
	R_Point *p_points = (R_Point *)param_findc_rect[n].addr;
	uint16_t i, k;
	float orientation = 0;
	float base_a;
	float base_b = 0;

	float *abuf = NULL;
//	float* inv_vect_length;
//	R_Point* vect;
	int seq[4] = { -1, -1, -1, -1 };
	float buffer[7];
	float out[6];
	int pindex;

//	abuf = R_OS_Malloc(param_findc_rect[n].count * 2 * sizeof(float), R_MEMORY_REGION_DEFAULT);
//	if (abuf == NULL)
//		return -1;

//	inv_vect_length = abuf;
//	vect = (R_Point*)(inv_vect_length + param_findc_rect[n].count);

//	for (i=0; i<param_findc_rect[n].count; i++)
//	{
//		p_points[i].y = height - 1 - p_points[i].y;
//	}

	R_Point pt0 = p_points[0];
	left_x = right_x = pt0.x;
	top_y = bottom_y = pt0.y;

	for( i = 0; i < param_findc_rect[n].count; i++ )
	{
		int16_t dx, dy;

		if( pt0.x < left_x )
		{
			left_x = pt0.x;
			left = i;
		}
		if( pt0.x > right_x )
		{
			right_x = pt0.x;
			right = i;
		}
		if( pt0.y > top_y )
		{
			top_y = pt0.y;
			top = i;
		}
		if( pt0.y < bottom_y )
		{
			bottom_y = pt0.y;
			bottom = i;
		}
		R_Point pt = p_points[(i+1) & (i+1 < param_findc_rect[n].count ? -1 : 0)];

		dx = pt.x - pt0.x;
		dy = pt.y - pt0.y;

		vect[i].x = dx;
		vect[i].y = dy;
		inv_vect_length[i] = (float)(1./sqrt((double)(dx*dx) + (double)dy*dy));

		pt0 = pt;
	}

	// find convex hull orientation
	{
		int ax = (int)vect[param_findc_rect[n].count-1].x;
		int ay = (int)vect[param_findc_rect[n].count-1].y;

		for( i = 0; i < param_findc_rect[n].count; i++ )
		{
			int bx = (int)vect[i].x;
			int by = (int)vect[i].y;

			int convexity = ax * by - ay * bx;

			if( convexity != 0 )
			{
				orientation = (convexity > 0) ? 1.f : (-1.f);
				break;
			}
			ax = bx;
			ay = by;
		}
		if( orientation == 0 )
		{
//			R_OS_Free((void*)&abuf);
			return -1;
		}
	}
	base_a = orientation;

	/*****************************************************************************************/
	/*                         init calipers position                                        */
	seq[0] = bottom;
	seq[1] = right;
	seq[2] = top;
	seq[3] = left;

	/*****************************************************************************************/
	/*                         Main loop - evaluate angles and rotate calipers               */

	/* all of edges will be checked while rotating calipers by 90 degrees */
	for( k = 1; k < param_findc_rect[n].count; k++ )
	{
		/* sinus of minimal angle */
		/*float sinus;*/
#if 1
		/* compute cosine of angle between calipers side and polygon edge */
		/* dp - dot product */
		float dp[4] = {
			+base_a * (float)vect[seq[0]].x + base_b * (float)vect[seq[0]].y,
			-base_b * (float)vect[seq[1]].x + base_a * (float)vect[seq[1]].y,
			-base_a * (float)vect[seq[2]].x - base_b * (float)vect[seq[2]].y,
			+base_b * (float)vect[seq[3]].x - base_a * (float)vect[seq[3]].y,
		};

		float maxcos = dp[0] * inv_vect_length[seq[0]];

		/* number of calipers edges, that has minimal angle with edge */
		int main_element = 0;

		/* choose minimal angle */
		for ( i = 1; i < 4; ++i )
		{
			float cosalpha = dp[i] * inv_vect_length[seq[i]];
			if (cosalpha > maxcos)
			{
				main_element = i;
				maxcos = cosalpha;
			}
		}

		/*rotate calipers*/
		{
			//get next base
			pindex = seq[main_element];
			float lead_x = (float)(vect[pindex].x)*inv_vect_length[pindex];
			float lead_y = (float)(vect[pindex].y)*inv_vect_length[pindex];
			switch( main_element )
			{
			case 0:
				base_a = lead_x;
				base_b = lead_y;
				break;
			case 1:
				base_a = lead_y;
				base_b = lead_x;
				break;
			case 2:
				base_a = -lead_x;
				base_b = -lead_y;
				break;
			case 3:
				base_a = -lead_y;
				base_b = lead_x;
				break;
			default:
				break;
			}
		}
		/* change base point of main edge */
		seq[main_element] += 1;
		seq[main_element] = (seq[main_element] == param_findc_rect[n].count) ? 0 : seq[main_element];
#else
		int16_t ddx, ddy;
		float length;
		pindex = (bottom + k >= param_findc_rect[n].count) ? bottom + k - param_findc_rect[n].count : bottom + k;
		ddx = p_points[pindex].x - p_points[bottom].x;
		ddy = p_points[pindex].y - p_points[bottom].y;
		length = (float)(sqrt((double)(ddx*ddx) + (double)ddy*ddy));
		base_a = (float)ddx / length;
		base_b = (float)ddy / length;
#endif
		/* find area of rectangle */
		{
		float h;
		float area;

		/* find vector left-right */
		float dx = p_points[seq[1]].x - p_points[seq[3]].x;
		float dy = p_points[seq[1]].y - p_points[seq[3]].y;

		/* dotproduct */
		float w = dx * base_a + dy * base_b;

		/* find vector top-bottom */
		dx = p_points[seq[2]].x - p_points[seq[0]].x;
		dy = p_points[seq[2]].y - p_points[seq[0]].y;

		/* dotproduct */
		h = -dx * base_b + dy * base_a;

		area = w * h;
		if( area <= minarea )
		{
			minarea = area;
			/* leftist point */
			buffer[0] = (float)seq[3];
			buffer[1] = base_a;
			buffer[2] = w;
			buffer[3] = base_b;
			buffer[4] = h;
			/* bottom point */
			buffer[5] = (float)seq[0];
			buffer[6] = area;
		}
		}
	}

	{
	float A1 = buffer[1];
	float B1 = buffer[3];

	float A2 = -buffer[3];
	float B2 = buffer[1];

	float C1 = A1 * p_points[(int)buffer[0]].x + p_points[(int)buffer[0]].y * B1;
	float C2 = A2 * p_points[(int)buffer[5]].x + p_points[(int)buffer[5]].y * B2;

	float idet = 1.f / (A1 * B2 - A2 * B1);

	float px = (C1 * B2 - C2 * B1) * idet;
	float py = (A1 * C2 - A2 * C1) * idet;

	out[0] = px;
	out[1] = py;

	out[2] = A1 * buffer[2];
	out[3] = B1 * buffer[2];

	out[4] = A2 * buffer[4];
	out[5] = B2 * buffer[4];
	}

	{
	uint16_t center_x, center_y, box_width, box_height;
	double angle;
	center_x = (uint16_t)(out[0] + (out[2] + out[4])*0.5f);
	center_y = (uint16_t)(out[1] + (out[3] + out[5])*0.5f);
	box_width = (uint16_t)sqrt((double)out[2]*out[2] + (double)out[3]*out[3]);
	box_height = (uint16_t)sqrt((double)out[4]*out[4] + (double)out[5]*out[5]);
	angle = (float)atan2( (double)out[3], (double)out[2] );

    float b = (float)cos(angle)*0.5f;
    float a = (float)sin(angle)*0.5f;

    p_box->box[0][0] = center_x - a*box_height - b*box_width;
    p_box->box[0][1] = center_y + b*box_height - a*box_width;
    p_box->box[1][0] = center_x + a*box_height - b*box_width;
    p_box->box[1][1] = center_y - b*box_height - a*box_width;
    p_box->box[2][0] = 2*center_x - p_box->box[0][0];
    p_box->box[2][1] = 2*center_y - p_box->box[0][1];
    p_box->box[3][0] = 2*center_x - p_box->box[1][0];
    p_box->box[3][1] = 2*center_y - p_box->box[1][1];
	}

//	for (i=0; i<4; i++)
//	{
//		p_box->box[i][1] = height - 1 - p_box->box[i][1];
//	}
//	R_OS_Free((void*)&abuf);
	return 0;
}
#else
float inv_vect_length[512];
R_Point vect[512];
R_Point debug_contour[12] =
{
		//test1 (12 points)
		{10,10},{10,11},{10,12},{10,13},
		{11,13},{12,13},{13,13},{14,12},
		{13,11},{13,10},{12,9},{11,10}
		//test2 (8 points)
//		{12,3},{11,4},{10,5},{11,6},
//		{12,7},{13,6},{14,5},{13,4}
		//test3 (12 points)
//		{11,19},{10,20},{9,20}, {8,21},
//		{8,22}, {9,23}, {10,23},{11,22},
//		{12,22},{13,21},{13,20},{12,19}
};

//0x04 0x02 0x01
//0x08      0x80
//0x10 0x20 0x40
static const uint8_t delta_value[9] =
{0x04, 0x02, 0x01, 0x08, 0x00, 0x80, 0x10, 0x20, 0x40};

//top_left_skip
static const uint8_t tls[12] =
{  0x05,0x09,0x11,0x82,0x84,0x42,0x44,0x0A,0x12,0x24,0x14,0x44 };
//top_left_skip_start
static const uint8_t tlss[12] =
{  2,2,2,5,5,8,8,1,1,0,0,0 };

//bottom_left_skip
static const uint8_t bls[12] =
{  0x05,0x09,0x11,0x0A,0x12,0x14,0x24,0x44,0x28,0x48,0x50,0x90 };
//bottom_left_skip_start
static const uint8_t blss[12] =
{  2,2,2,1,1,0,0,0,3,3,6,6 };

//bottom_right_skip
static const uint8_t brs[12] =
{  0x24,0x44,0x28,0x48,0x11,0x50,0x90,0xA0,0x21,0x41,0x42,0x44 };
//bottom_right_skip_start
static const uint8_t brss[12] =
{ 0,0,3,3,6,6,6,7,7,7,8,8  };

//top_right_skip
static const uint8_t trs[12] =
{  0x11,0x50,0x90,0xA0,0x21,0x42,0x44,0x82,0x84,0x09,0x05,0x41 };
//top_right_skip_start
static const uint8_t trss[12] =
{  6,6,6,7,7,8,8,5,5,2,2,8  };


static uint8_t skip_point(const uint8_t *pv, const uint8_t* pvs, uint8_t n, uint8_t v, uint8_t vs)
{
   for (int i=0; i<n; i++)
   {
	   if (*(pv+i) == v && *(pvs+i) == vs)
		   return 1;
   }
   return 0;
}

//only keep corner points, similar with CV_CHAIN_APPROX_SIMPLE
static uint16_t filter_contour_points(uint32_t n, float center_x, float center_y)
{
	uint16_t i, new_points_num = 0;
	uint16_t pre, next;
	R_Point *p_points = (R_Point *)param_findc_rect[n].addr;

	for (i=0; i<param_findc_rect[n].count; i++)
	{
		pre = (i == 0) ? (param_findc_rect[n].count - 1) : (i - 1);
		next = (i == param_findc_rect[n].count - 1) ? 0 : (i + 1);
		if ((p_points[i].x == p_points[pre].x && p_points[i].x == p_points[next].x) ||
			(p_points[i].y == p_points[pre].y && p_points[i].y == p_points[next].y))
		{
		}
		else
		{
			//save new points
			R_Point i0, i1;
			uint8_t dv, skip, s;
			i0.x = p_points[pre].x - p_points[i].x;
			i0.y = p_points[pre].y - p_points[i].y;
			i1.x = p_points[next].x - p_points[i].x;
			i1.y = p_points[next].y - p_points[i].y;
			//0       1       2
			//3               5
			//6       7       8
			s = (i0.y+1)*3+(i0.x+2)-1;
			dv = delta_value[s] + delta_value[(i1.y+1)*3+(i1.x+2)-1];
			if (p_points[i].x <= center_x && p_points[i].y <= center_y)
			{
				//top-left
				skip = skip_point(tls, tlss, sizeof(tls), dv, s);
			}
			else if (p_points[i].x <= center_x && p_points[i].y > center_y)
			{
				//bottom-left
				skip = skip_point(bls, blss, sizeof(bls), dv, s);
			}
			else if (p_points[i].x > center_x && p_points[i].y <= center_y)
			{
				//top-right
				skip = skip_point(trs, trss, sizeof(trs), dv, s);

			}
			else
			{
				//bottom-right
				skip = skip_point(brs, brss, sizeof(brs), dv, s);
			}
			if (!skip)
				p_points[new_points_num++] = p_points[i];
		}
	}
	return new_points_num;
}

static int get_contour_box(R_Box* p_box, uint32_t n, uint16_t width, uint16_t height)
{
	float minarea = width * height;
	R_Point *p_points = (R_Point *)param_findc_rect[n].addr;
	uint16_t i, k, new_points_num = 0;
	float orientation = 0;
	float base_a;
	float base_b = 0;
	int left = 0, bottom = 0, right = 0, top = 0;
	int left_x, right_x, top_y, bottom_y;
//	float *abuf = NULL;
//	float* inv_vect_length;
//	R_Point* vect;
	int seq[4] = { -1, -1, -1, -1 };
	float buffer[7];
	float out[6];
	int pindex;
	R_Point pt0;
	float center_x, center_y;

	center_x = (float)param_findc_rect[n].x_coordinate + (float)(param_findc_rect[n].width) / 2;
	center_y = (float)param_findc_rect[n].y_coordinate + (float)(param_findc_rect[n].height) /2;

//	memcpy(p_points, debug_contour, 12 * sizeof(R_Point));
//	param_findc_rect[n].count = 12;
//	center_x = 12.0; //12.0; 11.0;
//	center_y = 11.5; //5.0; //21.5;

	new_points_num = filter_contour_points(n, center_x, center_y);

//	for (i=0; i<new_points_num; i++)
//	{
//		p_points[i].y = height - 1 - p_points[i].y;
//	}

//	abuf = R_OS_Malloc(new_points_num * 2 * sizeof(float), R_MEMORY_REGION_DEFAULT);
//	if (abuf == NULL)
//		return -1;
//
//	inv_vect_length = abuf;
//	vect = (R_Point*)(inv_vect_length + new_points_num);

	pt0 = p_points[0];
	left_x = right_x = pt0.x;
	top_y = bottom_y = pt0.y;

	for( i = 0; i < new_points_num; i++ )
	{
		int16_t dx, dy;

		if( pt0.x < left_x )
		{
			left_x = pt0.x;
			left = i;
		}
		if( pt0.x > right_x )
		{
			right_x = pt0.x;
			right = i;
		}
		if( pt0.y > top_y )
		{
			top_y = pt0.y;
			top = i;
		}
		if( pt0.y < bottom_y )
		{
			bottom_y = pt0.y;
			bottom = i;
		}
		R_Point pt = p_points[(i+1) & (i+1 < new_points_num ? -1 : 0)];

		dx = pt.x - pt0.x;
		dy = pt.y - pt0.y;

		vect[i].x = dx;
		vect[i].y = dy;
		inv_vect_length[i] = (float)(1./sqrt((double)(dx*dx) + (double)dy*dy));

		pt0 = pt;
	}

	// find convex hull orientation
	{
		int ax = (int)vect[new_points_num-1].x;
		int ay = (int)vect[new_points_num-1].y;

		for( i = 0; i < new_points_num; i++ )
		{
			int bx = (int)vect[i].x;
			int by = (int)vect[i].y;

			int convexity = ax * by - ay * bx;

			if( convexity != 0 )
			{
				orientation = (convexity > 0) ? 1.f : (-1.f);
				break;
			}
			ax = bx;
			ay = by;
		}
		if( orientation == 0 )
		{
//			R_OS_Free((void*)&abuf);
			return -1;
		}
	}
	base_a = orientation;

	/*****************************************************************************************/
	/*                         init calipers position                                        */
	seq[0] = bottom;
	seq[1] = right;
	seq[2] = top;
	seq[3] = left;

	/*****************************************************************************************/
	/*                         Main loop - evaluate angles and rotate calipers               */

	/* all of edges will be checked while rotating calipers by 90 degrees */
	for( k = 0; k < new_points_num; k++ )
	{
		/* sinus of minimal angle */
		/*float sinus;*/
		/* compute cosine of angle between calipers side and polygon edge */
		/* dp - dot product */
		float dp[4] = {
			+base_a * (float)vect[seq[0]].x + base_b * (float)vect[seq[0]].y,
			-base_b * (float)vect[seq[1]].x + base_a * (float)vect[seq[1]].y,
			-base_a * (float)vect[seq[2]].x - base_b * (float)vect[seq[2]].y,
			+base_b * (float)vect[seq[3]].x - base_a * (float)vect[seq[3]].y,
		};

		float maxcos = dp[0] * inv_vect_length[seq[0]];

		/* number of calipers edges, that has minimal angle with edge */
		int main_element = 0;

		/* choose minimal angle */
		for ( i = 1; i < 4; ++i )
		{
			float cosalpha = dp[i] * inv_vect_length[seq[i]];
			if (cosalpha > maxcos)
			{
				main_element = i;
				maxcos = cosalpha;
			}
		}

		/*rotate calipers*/
		{
			//get next base
			pindex = seq[main_element];
			float lead_x = (float)(vect[pindex].x)*inv_vect_length[pindex];
			float lead_y = (float)(vect[pindex].y)*inv_vect_length[pindex];
//			float lead_x = (float)(vect[k].x)*inv_vect_length[k];
//			float lead_y = (float)(vect[k].y)*inv_vect_length[k];
			base_a = lead_x >= 0 ? lead_x : -lead_x;
			base_b = lead_y >= 0 ? lead_y : -lead_y;
			switch( main_element )
			{
			case 0:
				base_a = lead_x;
				base_b = lead_y;
				break;
			case 3:
				base_a = lead_y;
				base_b = -lead_x;
				break;
			case 2:
				base_a = -lead_x;
				base_b = -lead_y;
				break;
			case 1:
				base_a = -lead_y;
				base_b = lead_x;
				break;
			default:
				break;
			}
		}
		/* change base point of main edge */
		seq[main_element] += 1;
		seq[main_element] = (seq[main_element] == new_points_num) ? 0 : seq[main_element];

		/* find area of rectangle */
		{
		float h;
		float area;

		/* find vector left-right */
		float dx = p_points[seq[1]].x - p_points[seq[3]].x;
		float dy = p_points[seq[1]].y - p_points[seq[3]].y;

		/* dx * cos - dy * sin */
		float w = dx * base_a - dy * base_b;

		/* find vector top-bottom */
		dx = p_points[seq[2]].x - p_points[seq[0]].x;
		dy = p_points[seq[2]].y - p_points[seq[0]].y;

		/* dotproduct */
		h = -dx * base_b + dy * base_a;

		area = w * h;
		if( area <= minarea )
		{
			minarea = area;
			/* leftist point */
			buffer[0] = (float)seq[3];
			buffer[1] = base_a;
			buffer[2] = w;
			buffer[3] = base_b;
			buffer[4] = h;
			/* bottom point */
			buffer[5] = (float)seq[0];
			buffer[6] = area;
		}
		}
	}

	{
	float A1 = buffer[1];
	float B1 = buffer[3];

	float A2 = -buffer[3];
	float B2 = buffer[1];

	float C1 = A1 * p_points[(int)buffer[0]].x + p_points[(int)buffer[0]].y * B1;
	float C2 = A2 * p_points[(int)buffer[5]].x + p_points[(int)buffer[5]].y * B2;

	float idet = 1.f / (A1 * B2 - A2 * B1);

	float px = (C1 * B2 - C2 * B1) * idet;
	float py = (A1 * C2 - A2 * C1) * idet;

	out[0] = px;
	out[1] = py;

	out[2] = A1 * buffer[2];
	out[3] = B1 * buffer[2];

	out[4] = A2 * buffer[4];
	out[5] = B2 * buffer[4];
	}

	{
	float center_x, center_y, box_width, box_height;
	double angle;
	center_x = (float)(out[0] + (out[2] + out[4])*0.5f);
	center_y = (float)(out[1] + (out[3] + out[5])*0.5f);
	box_width = (float)sqrt((double)out[2]*out[2] + (double)out[3]*out[3]);
	box_height = (float)sqrt((double)out[4]*out[4] + (double)out[5]*out[5]);
	angle = (float)atan2( (double)out[3], (double)out[2] );

    float b = (float)cos(angle)*0.5f;
    float a = (float)sin(angle)*0.5f;

    p_box->box[0][0] = (int)(center_x - a*box_height - b*box_width);
    p_box->box[0][1] = (int)(center_y + b*box_height - a*box_width);
    p_box->box[1][0] = (int)(center_x + a*box_height - b*box_width);
    p_box->box[1][1] = (int)(center_y - b*box_height - a*box_width);
    p_box->box[2][0] = (int)(2*center_x - p_box->box[0][0]);
    p_box->box[2][1] = (int)(2*center_y - p_box->box[0][1]);
    p_box->box[3][0] = (int)(2*center_x - p_box->box[1][0]);
    p_box->box[3][1] = (int)(2*center_y - p_box->box[1][1]);
	}

//	for (i=0; i<4; i++)
//	{
//		p_box->box[i][1] = height - 1 - p_box->box[i][1];
//	}
//	R_OS_Free((void*)&abuf);
	return 0;
}
#endif

uint32_t R_BCD_DRP_Contours_2_Box(uint8_t frame_id, uint16_t width, uint16_t height)
{
	uint32_t contour_no, i;
	int barcode_square_min = (width/5) * (height/5);

	contour_no = i = 0;
	while(R_BCD_DrpCheckEmptyContours(i))
	{
		if (param_findc_rect[i].x_coordinate <= 2 ||
			param_findc_rect[i].y_coordinate <= 2 ||
			param_findc_rect[i].x_coordinate + param_findc_rect[i].width >= (width - 2) ||
			param_findc_rect[i].y_coordinate + param_findc_rect[i].height >= (height - 2))
		{
			//filter out the contour on image boundary
			i++;
			continue;
		}
		if (abs((int)param_findc_rect[i].width - (int)param_findc_rect[i].height) < (((int)param_findc_rect[i].width + (int)param_findc_rect[i].height) >> 3))
		{
			app_contours[frame_id].rect[contour_no].x = (int)param_findc_rect[i].x_coordinate;
			app_contours[frame_id].rect[contour_no].y = (int)param_findc_rect[i].y_coordinate;
			app_contours[frame_id].rect[contour_no].width = (int)param_findc_rect[i].width;
			app_contours[frame_id].rect[contour_no].height = (int)param_findc_rect[i].height;
			if (get_contour_box(&app_contours[frame_id].box[contour_no], i, width, height) == 0)
				contour_no++;
		}
		i++;
	}

	return contour_no;
}

static void get_box_orientation(R_Box r_box, int *p_ul, int *p_ur, int *p_ll, int *p_lr)
{
	int i, center_x, center_y;
	int upper[3], lower[3], u, l;
	int left_x, right_x;
	int left=-1, right=-1;

	center_x = center_y = 0;
	u = l = 0;
	for (i=0;i<4;i++)
	{
		center_x += r_box.box[i][0];
		center_y += r_box.box[i][1];
	}
	center_x >>= 2;
	center_y >>= 2;
	for (i=0;i<4;i++)
	{
		if (r_box.box[i][1] <= center_y)
		{
			upper[u++] = i;
		}
		else
		{
			lower[l++] = i;
		}
	}
	if (u == 3)
	{
		//45 degree, sort x
		left_x = right_x = r_box.box[upper[0]][0];
		left = right = 0;
		for (i=1;i<3;i++)
		{
			if (left_x > r_box.box[upper[i]][0])
			{
				left_x = r_box.box[upper[i]][0];			left = i;
			}
			if (right_x < r_box.box[upper[i]][0])
			{
				right_x = r_box.box[upper[i]][0];			right = i;
			}
		}
		*p_ul = upper[left];
		*p_ur = upper[3-left-right];
		*p_ll = lower[0];
		*p_lr = upper[right];
	}
	else if (u == 1)
	{
		//45 degree, sort x
		left_x = right_x = r_box.box[lower[0]][0];
		left = right = 0;
		for (i=1;i<3;i++)
		{
			if (left_x > r_box.box[lower[i]][0])
			{
				left_x = r_box.box[lower[i]][0];			left = i;
			}
			if (right_x < r_box.box[lower[i]][0])
			{
				right_x = r_box.box[lower[i]][0];			right = i;
			}
		}
		*p_ul = lower[left];
		*p_ur = upper[0];
		*p_ll = lower[3-left-right];
		*p_lr = lower[right];
	}
	else if (u == 2)
	{
		*p_ul = (r_box.box[upper[0]][0] < r_box.box[upper[1]][0]) ? upper[0] : upper[1];
		*p_ur = (r_box.box[upper[0]][0] < r_box.box[upper[1]][0]) ? upper[1] : upper[0];
		*p_ll = (r_box.box[lower[0]][0] < r_box.box[lower[1]][0]) ? lower[0] : lower[1];
		*p_lr = (r_box.box[lower[0]][0] < r_box.box[lower[1]][0]) ? lower[1] : lower[0];

	}
	else
	{
		DRP_DRV_ASSERT(1);
	}
}
#if 0
void R_BCD_DRP_Keystone_Correction(uint32_t input_address, uint32_t output_address, uint16_t* input_width, uint16_t* input_height, R_Box r_box)
{
    int32_t ret_val;
	int upper_left=-1, upper_right=-1, lower_left=-1, lower_right=-1;
	int new_width, new_height;
	int dx, dy;

	PerformSetStartTime(TIME_AFFINE_DL);
	/*********************************/
	/* Load DRP Library              */
	/*        +--------------------+ */
	/* tile 0 | Keystone           | */
	/*        +                    + */
	/* tile 1 |                    | */
	/*        +                    + */
	/* tile 2 |                    | */
	/*        +                    + */
	/* tile 3 |                    | */
	/*        +                    + */
	/* tile 4 |                    | */
	/*        +                    + */
	/* tile 5 |                    | */
	/*        +--------------------+ */
	/*********************************/
	ret_val = R_DK2_Load(&g_drp_lib_keystone_correction[0],
			R_DK2_TILE_0 ,
			R_DK2_TILE_PATTERN_3_1_1_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
	DRP_DRV_ASSERT(ret_val);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0] , 0);
	DRP_DRV_ASSERT(ret_val);

	/* Set end time of process(DL)*/
	PerformSetEndTime(TIME_AFFINE_DL);
	/* Set start time of process*/
	PerformSetStartTime(TIME_AFFINE_EXE);

	/***************************************/
	/* Set R_DK2_Start function parameters */
	/***************************************/
	get_box_orientation(r_box, &upper_left, &upper_right, &lower_left, &lower_right);

	if (upper_left == -1 || upper_right == -1 || lower_left == -1 || lower_right == -1)
	{
		DRP_DRV_ASSERT(1);
	}
#ifdef KEYSTONE_FIXED_SIZE_ENABLE
	new_width = KEYSTONE_FIXED_SIZE;
	new_height = KEYSTONE_FIXED_SIZE;
#else
	//Get width
	dx = r_box.box[upper_left][0] - r_box.box[upper_right][0];
	dy = r_box.box[upper_left][1] - r_box.box[upper_right][1];
	new_width = (int)sqrt((double)dx*dx + (double)dy*dy);
	new_width = new_width >= 64 ? new_width : 64;
	//As next step will be DRP findContour, so the width should be multiple of 8
	if (new_width % 8 != 0)
	{
		new_width = (new_width + 7) & ~7;
	}
	//Get height
	dx = r_box.box[upper_right][0] - r_box.box[lower_right][0];
	dy = r_box.box[upper_right][1] - r_box.box[lower_right][1];
	new_height = (int)sqrt((double)dx*dx + (double)dy*dy);
	new_height = new_height >= 32 ? new_height : 32;

	//limit dst size to reduce memory
	while(new_width > R_BCD_CAMERA_WIDTH/DRP_RESIZE || new_height > R_BCD_CAMERA_HEIGHT/DRP_RESIZE)
	{
		new_width -= 8;
		new_height -= 8;
	}
#endif
	/* Set the address of buffer to be read/write by DRP */
	param_keystone_correction.src = input_address;
	param_keystone_correction.width = *input_width;
	param_keystone_correction.height = *input_height;


	param_keystone_correction.src_upper_left_x = r_box.box[upper_left][0];
	param_keystone_correction.src_upper_left_y = r_box.box[upper_left][1];
	param_keystone_correction.src_upper_right_x = r_box.box[upper_right][0];
	param_keystone_correction.src_upper_right_y = r_box.box[upper_right][1];

	param_keystone_correction.src_lower_left_x = r_box.box[lower_left][0];
	param_keystone_correction.src_lower_left_y = r_box.box[lower_left][1];
	param_keystone_correction.src_lower_right_x = r_box.box[lower_right][0];
	param_keystone_correction.src_lower_right_y = r_box.box[lower_right][1];


	param_keystone_correction.dst = output_address;
	param_keystone_correction.dst_width = new_width;
	param_keystone_correction.dst_height = new_height;

	param_keystone_correction.dest_stride = new_width;

	/* Initialize variables to be used in termination judgment of the DRP application */
	drp_lib_status[TILE_0] = DRP_NOT_FINISH;

	/*********************/
	/* Start DRP Library */
	/*********************/
	ret_val = R_DK2_Start(drp_lib_id[TILE_0], (void *)&param_keystone_correction, sizeof(r_drp_keystone_correction_t));
	DRP_DRV_ASSERT(ret_val);


	/***************************************/
	/* Wait until DRP processing is finish */
	/***************************************/
	while (drp_lib_status[TILE_0] == DRP_NOT_FINISH);

	/* Set end time of process */
	PerformSetEndTime(TIME_AFFINE_EXE);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0] , &drp_lib_id[0]);
	DRP_DRV_ASSERT(ret_val);

	*input_width = new_width;
	*input_height = new_height;
}
#endif

void R_BCD_Keystone_Correction(uint32_t input_address, uint32_t output_address, uint16_t* input_width, uint16_t* input_height, R_Box r_box)
{
	int upper_left=-1, upper_right=-1, lower_left=-1, lower_right=-1;
	int new_width, new_height;
	int dx, dy;
	uint8_t *ptr_w, *ptr_r;
	uint16_t cx;
	uint16_t cy;
	uint32_t ccx;
	uint32_t ccy;
	int32_t cdx;
	int32_t cdy;

	uint32_t cpxl;
	uint32_t cpxr;
	uint32_t cpyl;
	uint32_t cpyr;
	int32_t cdxl;
	int32_t cdxr;
	int32_t cdyl;
	int32_t cdyr;

	get_box_orientation(r_box, &upper_left, &upper_right, &lower_left, &lower_right);

	if (upper_left == -1 || upper_right == -1 || lower_left == -1 || lower_right == -1)
	{
		DRP_DRV_ASSERT(1);
	}
#ifdef KEYSTONE_FIXED_SIZE_ENABLE
	new_width = KEYSTONE_FIXED_SIZE;
	new_height = KEYSTONE_FIXED_SIZE;
#else
	uint16_t h;

	//Get width
//	dx = r_box.box[upper_left][0] - r_box.box[upper_right][0];
//	dy = r_box.box[upper_left][1] - r_box.box[upper_right][1];
//	w = (int)sqrt((double)dx*dx + (double)dy*dy);

	//Get height
	dx = r_box.box[upper_right][0] - r_box.box[lower_right][0];
	dy = r_box.box[upper_right][1] - r_box.box[lower_right][1];
	h = (int)sqrt((double)dx*dx + (double)dy*dy);

	//limit dst size to reduce memory
	if (h <= 128)
	{
		new_width = 96;
		new_height = 96;
	}
	else if (h > 128 && h <= 256)
	{
		new_width = 128;
		new_height = 128;
	}
	else
	{
		new_width = 128;
		new_height = 128;
	}

#endif

	/* Set start coordinate */
	cpxl = 0x00008000 + (r_box.box[upper_left][0] <<16);
	cpxr = 0x00008000 + (r_box.box[upper_right][0] <<16);
	cdxl = (int32_t)((r_box.box[lower_left][0] - r_box.box[upper_left][0]) << 16)/new_height;
	cdxr = (int32_t)((r_box.box[lower_right][0] - r_box.box[upper_right][0]) << 16)/new_height;
	cpyl = 0x00008000 + (r_box.box[upper_left][1] <<16);
	cpyr = 0x00008000 + (r_box.box[upper_right][1] <<16);
	cdyl = (int32_t)((r_box.box[lower_left][1] - r_box.box[upper_left][1]) << 16)/new_height;
	cdyr = (int32_t)((r_box.box[lower_right][1] - r_box.box[upper_right][1]) << 16)/new_height;

	ptr_r = (uint8_t*)input_address;
	for(cy=0; cy<new_height; cy++){
		ccx = cpxl;
		ccy = cpyl;
		cdx = (int32_t)(cpxr - cpxl)/new_width;
		cdy = (int32_t)(cpyr - cpyl)/new_width;
		ptr_w = (uint8_t*)(output_address + cy * new_width);
		for(cx=0; cx<new_width; cx++){
			*(ptr_w++) = *(ptr_r + (ccy >> 16)*(uint32_t)(*input_width) + (ccx >> 16));
			ccx += cdx;
			ccy += cdy;
		}
		cpxl += cdxl;
		cpxr += cdxr;
		cpyl += cdyl;
		cpyr += cdyr;
	}

	*input_width = new_width;
	*input_height = new_height;
}


/**********************************************************************************************************************
* Function Name: R_BCD_MainBinarization2
* Description  : Function to perform binarization process called from ZXing process
* Arguments    : in_adr
*              :   Address of input image
*              : out_adr
*              :   Address of Output buffer
*              : width
*              :   Horizontal size of processed image
*              : height
*              :   Vertical size of processed image
* Return Value : -
**********************************************************************************************************************/
void R_BCD_MainBinarization2(uint32_t in_adr, uint32_t out_adr, uint32_t width, uint32_t height)
{
	int32_t ret_val;
	uint32_t tile_no;

    /******************************************/
	/* Load DRP Library                       */
	/*        +-----------------------------+ */
	/* tile 0 |                             | */
	/*        +                             + */
	/* tile 1 | Binarization Adaptive (bit) | */
	/*        +                             + */
	/* tile 2 |                             | */
	/*        +-----------------------------+ */
	/* tile 3 |                             | */
	/*        +-----------------------------+ */
	/* tile 4 |                             | */
	/*        +-----------------------------+ */
	/* tile 5 |                             | */
	/*        +-----------------------------+ */
	/******************************************/
	PerformSetStartTime(TIME_ZXING_BINARIZATION_DL);
    ret_val = R_DK2_Load(&g_drp_lib_binarization_adaptive_bit[0],
                         R_DK2_TILE_0,
						 R_DK2_TILE_PATTERN_3_2_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
    DRP_DRV_ASSERT(ret_val);
    PerformSetEndTime(TIME_ZXING_BINARIZATION_DL);

    PerformSetStartTime(TIME_ZXING_BINARIZATION_EXE);

	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_0], 0);
	DRP_DRV_ASSERT(ret_val);

    /***************************************/
    /* Set R_DK2_Start function parameters */
    /***************************************/
    /* Set the address of buffer to be read/write by DRP */
    param_bin.src = in_adr;
    param_bin.dst = out_adr;

    /* Set Image size(width) */
    param_bin.width = (uint16_t)width;

    /* Set Image size(hight) */
    param_bin.height = (uint16_t)height;

    /* Set additional information (currently no additional information) */
    param_bin.work = (uint32_t)&drp_binarization_work_ram[0];
    param_bin.range = 0x1C;

    /* Initialize variables to be used in termination judgment of the DRP library */
    drp_lib_status[TILE_0] = DRP_NOT_FINISH;

    /* Clean D cache */
    R_CACHE_L1DataCleanInvalidAll();

    /*****************/
    /* Start DRP lib */
    /*****************/
    ret_val = R_DK2_Start(drp_lib_id[TILE_0], (void *)&param_bin, sizeof(r_drp_binarization_adaptive_bit_t));

    /***************************************/
    /* Wait until DRP processing is finish */
    /***************************************/
    while (drp_lib_status[TILE_0] == DRP_NOT_FINISH)
    {
        /* DO NOTHING */
    }

    /* Invalidate and clean all D cache */
    R_CACHE_L1DataCleanInvalidLine((void *)out_adr, (width * height)/8);

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_0], &drp_lib_id[0]);
	DRP_DRV_ASSERT(ret_val);

	PerformSetEndTime(TIME_ZXING_BINARIZATION_EXE);
//	char s[10];
//	char *p = (char*)out_adr;
//	int i;
//	for (int y=0; y<height; y++)
//	{
//		for (int x=0; x<width/8; x++)
//		{
//			for(i = 0 ; i < 8 ; i ++)
//			    if(*p & (0x01 << i))
//			      s[i] = '1';
//			    else
//			      s[i] = '0';
//			s[i] = '\0';
//			printf("%s",s);
//			p++;
//		}
//		printf("\r\n");
//	}
    return;
}
/**********************************************************************************************************************
* End of function R_BCD_MainBinarization2
**********************************************************************************************************************/

/**********************************************************************************************************************
* Function Name: R_BCD_MainReedsolomon
* Description  : Function to perform reedsolomon process called from ZXing process
* Arguments    : codewordBytes
*              :   Address that stores the result of reedsolomon processing
*              : numCodewords
*              :   Read size
*              : numECCodewords
*              :   Correct size
* Return Value : -
**********************************************************************************************************************/
bool R_BCD_MainReedsolomon(int8_t * codewordBytes, uint8_t numCodewords, uint8_t numECCodewords)
{
    bool ret;
    int32_t ret_val = 0;
    uint32_t tile_no;

    /******************************************/
	/* Load DRP Library                       */
	/*        +-----------------------------+ */
	/* tile 0 |                             | */
	/*        +                             + */
	/* tile 1 |                             | */
	/*        +                             + */
	/* tile 2 |                             | */
	/*        +-----------------------------+ */
	/* tile 3 |                             | */
	/*        +                             + */
	/* tile 4 |                             | */
	/*        +-----------------------------+ */
	/* tile 5 | Reed Solomon                | */
	/*        +-----------------------------+ */
	/******************************************/
    PerformSetStartTime(TIME_ZXING_REEDSOLOMON_DL);
    ret_val |= R_DK2_Load(&g_drp_lib_reed_solomon[0],
                              R_DK2_TILE_5,
    						  R_DK2_TILE_PATTERN_3_2_1, NULL, &cb_drp_finish, &drp_lib_id[0]);
    DRP_DRV_ASSERT(ret_val);
    PerformSetEndTime(TIME_ZXING_REEDSOLOMON_DL);

    PerformSetStartTime(TIME_ZXING_REEDSOLOMON_EXE);
	/************************/
	/* Activate DRP Library */
	/************************/
	ret_val = R_DK2_Activate(drp_lib_id[TILE_5], 0);
	DRP_DRV_ASSERT(ret_val);

    /***************************************/
    /* Set R_DK2_Start function parameters */
    /***************************************/
    /* Set the source address of buffer to be read/write by DRP */
    param_rs.src = (uint32_t)codewordBytes;

    /* Set the destination address of buffer to be read/write by DRP */
    param_rs.dst = (uint32_t)&drp_work_uncache[0];
    R_MMU_VAtoPA(param_rs.dst, &param_rs.dst);

    /* Set additional information (currently no additional information) */
    param_rs.src_size = (uint16_t)numCodewords;

    /* Set check size */
    param_rs.check_size = (uint16_t)numECCodewords;

    /* Initialize variables to be used in termination judgment of the DRP library */
    drp_lib_status[TILE_5] = DRP_NOT_FINISH;

    /* Clean D cache */
    R_CACHE_L1DataCleanLine((void *)codewordBytes, (uint32_t)(numCodewords + numECCodewords));

    /*****************/
    /* Start DRP lib */
    /*****************/
    (void)R_DK2_Start(drp_lib_id[TILE_5], (void *)&param_rs, sizeof(r_drp_reed_solomon_t));

    /***************************************/
    /* Wait until DRP processing is finish */
    /***************************************/
    while (drp_lib_status[TILE_5] == DRP_NOT_FINISH)
    {
        /* DO NOTHING */
    }

    ret = (drp_work_uncache[numCodewords] == 0);
    if (ret != false)
    {
        /* copy codewords */
        memcpy((void *)codewordBytes, (void *)drp_work_uncache, numCodewords);
    }

	/***************************************/
	/* Unload DRP Library                  */
	/***************************************/
	ret_val = R_DK2_Unload(drp_lib_id[TILE_5], &drp_lib_id[0]);
	DRP_DRV_ASSERT(ret_val);

	PerformSetEndTime(TIME_ZXING_REEDSOLOMON_EXE);
    return ret;
}
/**********************************************************************************************************************
* End of function R_BCD_MainReedsolomon
**********************************************************************************************************************/

/**********************************************************************************************************************
* Function Name: R_BCD_MainSetReedsolomonTime
* Description  : Function to get processing time of Reedsolomon
* Arguments    : time
*              :   Processing time of Reedsolomon
* Return Value : -
**********************************************************************************************************************/
void R_BCD_MainSetReedsolomonTime(uint32_t time)
{
    ReedsolomonTime = time;

    return;
}
/**********************************************************************************************************************
* End of function R_BCD_MainSetReedsolomonTime
**********************************************************************************************************************/
