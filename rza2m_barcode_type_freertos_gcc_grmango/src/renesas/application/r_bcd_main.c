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
* Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
**********************************************************************************************************************/

/**********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
**********************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "r_typedefs.h"
#include "iodefine.h"
#include "r_cache_lld_rza2m.h"

#include "r_bcd_main.h"
#include "r_bcd_camera.h"
#include "r_bcd_lcd.h"
#include "draw.h"
#include "perform.h"
#include "r_mmu_lld_rza2m.h"
#include "fontdata.h"
#include "r_bcd_ae.h"
#include "r_dk2_if.h"


#include "main.h"
#include "r_bcd_drp_application.h"
#include "r_opencv.h"
#include "r_bcd_contour.h"
#include "r_zxing.h"

/*******************************************************************************
Macro definitions
*******************************************************************************/
#define ZXING_RESULT_BUF_SIZE (256)

typedef enum
{
	DM_GRAY,	//Only show 1/4 thinning gray image
	DM_BIN,		//Only show 1/4 thinning binarized image
	DM_BIN_BB,	//binarized barcode overlay on 1/4 binarized image
}R_DISPLAY_MODE;

typedef struct
{
	uint16_t peak_start;
	uint16_t peak_end;
	uint8_t	 peak_valid;
}SEGMENT_WAVE_ST;

typedef struct
{
	int32_t			barcode_contour_detected; //a barcode was foud by barcode type detection module
	barcode_type_t	type;	//barcode type is output from barcode type detection module
	uint32_t		cropped_barcode_grayscale_buf; //output from barcode type detection module
	uint16_t		cropped_barcode_buf_width;
	uint16_t		cropped_barcode_buf_height;
	uint32_t		camera_bayer_buf;
	uint8_t 		result_buf[ZXING_RESULT_BUF_SIZE];
}BARCODE_DECODE_ST;

/* Key status */
#define KEY_STAUS_INIT  (0x00000000U)
#define KEY_CHECK_BIT   (0x00000003U)
#define KEY_JUST_ON     (0x00000002U)
#define KEY_ON_KEEP     (0x00000000U)

/*******************************************************************************
Imported global variables and functions (from other files)
*******************************************************************************/

static R_Rect barcode_rect;
static R_Rect g_crop_rect;
static barcode_type_t g_barcode_type;
static bool_t g_data_matrix;
static int32_t g_contour_id;
static R_DISPLAY_MODE g_display_mode = DM_GRAY;
static BARCODE_DECODE_ST g_barcode_decode;

static const unsigned char* barcode_name[BARCODE_UNKNOWN+1] = {
"QR",
"MICRO QR",
"AZTEC",
"HANXIN",
"DATA MATRIX",
"PDF417",
"MICRO_PDF417",
"MAXICODE",
"CODABLOK_F",
"DOTCODE",
"UNKNOWN"
};


/*******************************************************************************
Private global variables and functions
*******************************************************************************/
static uint8_t  work_buf[16][(R_BCD_CAMERA_WIDTH/DRP_RESIZE) * (R_BCD_CAMERA_HEIGHT/DRP_RESIZE)] __attribute__ ((section("Video_RAM")));
static uint32_t histogram_h[R_BCD_CAMERA_HEIGHT/DRP_RESIZE] ;
static uint32_t histogram_v[R_BCD_CAMERA_WIDTH/DRP_RESIZE] ;
static r_bcd_ae_setting_t ae_setting;
static uint32_t g_execute_time[TIME_INVALID];

uint32_t 		ave_result[6] __attribute__ ((section("UNCACHED_BSS")));
extern R_Contour    app_contours[2];
extern uint32_t g_camera_vsync_us;


void R_BCD_BARCODE_TYPE_DETECT(void);
void R_BCD_DrawProcessingTime_SVGA(void);
/* key status */
static uint32_t key3_status;
static uint32_t key4_status;

static void draw_contour_box(uint32_t frameId)
{
	uint16_t x_offset=0, y_offset=0;
	uint8_t color;
	if (frameId == 1)
	{
		x_offset = barcode_rect.x;
		y_offset = barcode_rect.y;
	}
	for (int i=0; i< app_contours[frameId].contours_no; i++)
	{
		if (i == g_contour_id)
			color = RED;
		else
			color = BLUE;
		DrawLine( VDC_LAYER_ID_3_RD, app_contours[frameId].box[i].box[0][0]+x_offset, app_contours[frameId].box[i].box[0][1]+y_offset, app_contours[frameId].box[i].box[1][0]+x_offset, app_contours[frameId].box[i].box[1][1]+y_offset, color );
		DrawLine( VDC_LAYER_ID_3_RD, app_contours[frameId].box[i].box[1][0]+x_offset, app_contours[frameId].box[i].box[1][1]+y_offset, app_contours[frameId].box[i].box[2][0]+x_offset, app_contours[frameId].box[i].box[2][1]+y_offset, color );
		DrawLine( VDC_LAYER_ID_3_RD, app_contours[frameId].box[i].box[2][0]+x_offset, app_contours[frameId].box[i].box[2][1]+y_offset, app_contours[frameId].box[i].box[3][0]+x_offset, app_contours[frameId].box[i].box[3][1]+y_offset, color );
		DrawLine( VDC_LAYER_ID_3_RD, app_contours[frameId].box[i].box[3][0]+x_offset, app_contours[frameId].box[i].box[3][1]+y_offset, app_contours[frameId].box[i].box[0][0]+x_offset, app_contours[frameId].box[i].box[0][1]+y_offset, color );
	}
}

static void draw_contour_rect(uint32_t frameId)
{
	uint16_t x_offset=0, y_offset=0;
	uint8_t color;

//	if (frameId == 1)
//	{
//		x_offset = barcode_rect.x;
//		y_offset = barcode_rect.y;
//	}
	for (int i=0; i< app_contours[frameId].contours_no; i++)
	{
		if (i == g_contour_id)
			color = RED;
		else
			color = BLUE;

		DrawLine( VDC_LAYER_ID_3_RD,
				app_contours[frameId].rect[i].x + x_offset,
				app_contours[frameId].rect[i].y + y_offset,
				app_contours[frameId].rect[i].x + x_offset + app_contours[frameId].rect[i].width,
				app_contours[frameId].rect[i].y + y_offset, color );
		DrawLine( VDC_LAYER_ID_3_RD,
				app_contours[frameId].rect[i].x + x_offset,
				app_contours[frameId].rect[i].y + y_offset,
				app_contours[frameId].rect[i].x + x_offset,
				app_contours[frameId].rect[i].y + y_offset + app_contours[frameId].rect[i].height, color );
		DrawLine( VDC_LAYER_ID_3_RD,
				app_contours[frameId].rect[i].x + x_offset + app_contours[frameId].rect[i].width,
				app_contours[frameId].rect[i].y + y_offset,
				app_contours[frameId].rect[i].x + x_offset + app_contours[frameId].rect[i].width,
				app_contours[frameId].rect[i].y + y_offset + app_contours[frameId].rect[i].height, color );
		DrawLine( VDC_LAYER_ID_3_RD,
				app_contours[frameId].rect[i].x + x_offset,
				app_contours[frameId].rect[i].y + y_offset + app_contours[frameId].rect[i].height,
				app_contours[frameId].rect[i].x + x_offset + app_contours[frameId].rect[i].width,
				app_contours[frameId].rect[i].y + y_offset + app_contours[frameId].rect[i].height, color );
	}
}

//Get maximal rect from contour box and rect
static void get_cropping_rect(uint8_t frameId, uint32_t contour_no, R_Rect* p_rect)
{
	int i;
	uint8_t mod;

	memcpy(p_rect, &app_contours[frameId].rect[contour_no], sizeof(R_Rect) );

	//clip the max area from contour rect and contour box
	for (i=0; i<4; i++)
	{
		//adjust top_left
		if (app_contours[frameId].box[contour_no].box[i][0] < p_rect->x)
			p_rect->x = app_contours[frameId].box[contour_no].box[i][0];
		if (app_contours[frameId].box[contour_no].box[i][1] < p_rect->y)
			p_rect->y = app_contours[frameId].box[contour_no].box[i][1];
	}
	for (i=0; i<4; i++)
	{
		//adjust width/height
		if (app_contours[frameId].box[contour_no].box[i][0] > p_rect->x + p_rect->width)
			p_rect->width = app_contours[frameId].box[contour_no].box[i][0] - p_rect->x;
		if (app_contours[frameId].box[contour_no].box[i][1] > p_rect->y + p_rect->height)
			p_rect->height = app_contours[frameId].box[contour_no].box[i][1] - p_rect->y;
	}

	//get original size on bayer image
	p_rect->x *= DRP_RESIZE;
	p_rect->y *= DRP_RESIZE;
	p_rect->width *= DRP_RESIZE;
	p_rect->height *= DRP_RESIZE;

	//DRP cropping requires width is 8~1280 and multiple of 8, height is 8~960 and multiple of 8
	//DRP BinarizationAdaptive require width is 64~1280 and multiple of 32, height is 40~960 and multiple of 8
	//So we set cropping size based on DRP BinarizationAdaptive
	p_rect->width = p_rect->width >= 64 ? p_rect->width : 64;
	p_rect->height = p_rect->height >= 40 ? p_rect->height : 40;
	mod = p_rect->width % 32;
	if (mod != 0)
	{
		mod = 32 - mod;
		if (p_rect->x > (mod/2))
		{
			p_rect->x -= (mod/2);
		}
		p_rect->width += mod;
		if (p_rect->x + p_rect->width > R_BCD_CAMERA_WIDTH)
			p_rect->width -= 32;
	}
	mod = p_rect->height % 48;
	if (mod != 0)
	{
		//we use 6 tile crop after this process, make sure every tile's height is multiple of 8
		mod = 48 - mod;
		if (p_rect->y > (mod/2))
		{
			p_rect->y -= (mod/2);
		}
		p_rect->height += mod;

		if (p_rect->y + p_rect->height > R_BCD_CAMERA_HEIGHT)
			p_rect->height -= 48;
	}
}

static barcode_type_t check_data_matrix(uint16_t width, uint16_t height)
{
	uint16_t i;
	uint32_t h_max, v_max;
	uint16_t h_max_pos=0, v_max_pos=0;
	barcode_type_t ret;
	uint8_t left=0, right=0, top=0, bottom=0;

	h_max = histogram_h[0];
	for (i=1; i<height; i++)
	{
		if (histogram_h[i] > h_max)
		{
			h_max_pos = i;
			h_max = histogram_h[i];
		}
	}

	v_max = histogram_v[0];
	for (i=1; i<width; i++)
	{
		if (histogram_v[i] > v_max)
		{
			v_max_pos = i;
			v_max = histogram_v[i];
		}
	}

	if (h_max_pos <= (height >> 3))
		top = 1;
	if (h_max_pos >= height - (height >> 3))
		bottom = 1;
	if (v_max_pos <= (width >> 3))
		left = 1;
	if (v_max_pos >= width - (width >> 3))
		right = 1;

	if ((top & left) || (top & right) || (right & bottom) || (bottom & left))
		ret = BARCODE_DATA_MATRIX;
	else
		ret = BARCODE_UNKNOWN;

	return ret;
}

static void check_data_matrix2(uint8_t* pbuf, uint16_t width, uint16_t height)
{
	//only check boundary region
	//start from 0%, end at 20%, whether there is one line all of the data is 0xFF
	uint16_t loop_h = height / 5;
	uint16_t loop_w = width / 5;
	uint8_t left=1, right=1, top=1, bottom=1;
	uint16_t i,j;
	uint8_t *ptr;

	//check top boundary
	for (i=0; i<loop_h; i++)
	{
		ptr = pbuf + i*width + loop_w;
		top = 1;
		for (j=loop_w; j<width-loop_w; j++)
		{
			if (*ptr++ == 0)
			{
				top = 0;
				break;
			}
		}
		if (top == 1)
			break;
	}

	//check bottom boundary
	for (i=height-loop_h; i<height; i++)
	{
		ptr = pbuf + i*width + loop_w;
		bottom = 1;
		for (j=loop_w; j<width-loop_w; j++)
		{
			if (*ptr++ == 0)
			{
				bottom = 0;
				break;
			}
		}
		if (bottom == 1)
			break;
	}

	//check left boundary
	for (i=0; i<loop_w; i++)
	{
		left = 1;
		for (j=loop_h; j<height-loop_h; j++)
		{
			if (*(pbuf+j*width+i) == 0)
			{
				left = 0;
				break;
			}
		}
		if (left == 1)
			break;
	}

	//check right boundary
	for (i=width-loop_w; i<width; i++)
	{
		right = 1;
		for (j=loop_h; j<height-loop_h; j++)
		{
			if (*(pbuf+j*width+i) == 0)
			{
				right = 0;
				break;
			}
		}
		if (right == 1)
			break;
	}

	if ((top & left) || (top & right) || (right & bottom) || (bottom & left))
		g_data_matrix = true;
	else
		g_data_matrix = false;

}

static uint32_t select_contour(void)
{
	int ret = -1;

	for (int i=0; i<app_contours[0].contours_no; i++)
	{
		//select the rectangle closed to square
		//2 line margin on top
		if (abs(app_contours[0].rect[i].width - app_contours[0].rect[i].height) > ((app_contours[0].rect[i].width + app_contours[0].rect[i].height) >> 3) ||
				app_contours[0].rect[i].y < 2)
			continue;
		if (ret == -1)
		{
			ret = i; //the first possible rect
		}
		else
		{
			//TODO
			if (app_contours[0].rect[i].x > app_contours[0].rect[ret].x &&
			(app_contours[0].rect[i].x + app_contours[0].rect[i].width) < (app_contours[0].rect[ret].x + app_contours[0].rect[ret].width) &&
			app_contours[0].rect[i].y > app_contours[0].rect[ret].y &&
			(app_contours[0].rect[i].y + app_contours[0].rect[i].height) < (app_contours[0].rect[ret].y + app_contours[0].rect[ret].height))
			{
				ret = i;
			}
		}
	}
	return ret;
}

void R_BCD_Histogram_HV(uint8_t* pbuf, uint16_t width, uint16_t height)
{
	uint16_t i, j;

	//first line
	histogram_h[0] = 0;
	for (j=0; j<width; j++)
	{
		histogram_h[0] += *pbuf;
		histogram_v[j] = *pbuf;
		pbuf++;
	}
	for (i=1; i<height; i++)
	{
		histogram_h[i] = *pbuf;
		histogram_v[0] += *pbuf;
		pbuf++;
		for (j=1; j<width; j++)
		{
			histogram_h[i] += *pbuf;
			histogram_v[j] += *pbuf;
			pbuf++;
		}
	}
}

static void decode_barcode(BARCODE_DECODE_ST *decode_param)
{
	if (decode_param->barcode_contour_detected != -1)
	{
		zxing_decode_image((uint8_t*)(decode_param->cropped_barcode_grayscale_buf),\
							decode_param->cropped_barcode_buf_width, decode_param->cropped_barcode_buf_height, \
							(char *)decode_param->result_buf, sizeof(decode_param->result_buf)-1,\
							decode_param->type);
	}
	else
	{
		//barcode type detection module couldn't find any barcode in current frame
		//if we fully depend on barcode detection result, just do nothing here;
		//otherwise we will convert camera frame to grayscale and
		//pass the whole buffer to zxing module and detect it again, it will take ~3xms for 1280x720 input

	}
}

uint32_t debug_findc = 0;
/*******************************************************************************
* Function Name: sample_main
* Description  : First function called after initialization is completed
* Arguments    : -
* Return Value : -
*******************************************************************************/
void sample_main(void)
{

	/* Initialization of VIN and MIPI driver */
	R_BCD_CameraInit();

	/* Initialization of LCD driver */
	R_BCD_LcdInit();

    /* Set SW4 readable */
    PORT5.PMR.BIT.PMR6 = 0;
    PORT5.PDR.BIT.PDR6 = 2;

	/* Capture Start */
	R_BCD_CameraCaptureStart();

	/* Initialize of Performance counter */
	PerformInit();

	/********************************/
	/* Initialization of DRP driver */
	/********************************/
	R_DK2_Initialize();

	R_BCD_AeInit(&ae_setting);

	R_BCD_AeStart(&ae_setting);
	ae_setting.brightness = BRIGHTNESS_SET; //GammaTable is not used


	//first time to draw video on 2nd buffer as r_bcd_lcd use 1st buffer as initialization
	R_BCD_LcdSwapVideoBuffer();
	while(R_BCD_ChkLcdSwapVideoBufferReq());

	/* Clean D cache */
	R_CACHE_L1DataCleanInvalidAll();


    /* Key initialize */
    key3_status = KEY_STAUS_INIT;
    key4_status = KEY_STAUS_INIT;

    /* main loop */
    while (1)
    {

    	/* Get User Switch0 (SW2) key */
		key3_status = (key3_status << 1) | PORTD.PIDR.BIT.PIDR6;
		if ( ( key3_status & KEY_CHECK_BIT) == KEY_JUST_ON )
		{
	       	g_display_mode = (g_display_mode == DM_BIN_BB) ? DM_GRAY : (g_display_mode+1);
        }

        R_BCD_BARCODE_TYPE_DETECT();


    }

    return;
}
/*******************************************************************************
* End of function sample_main
*******************************************************************************/

void R_BCD_BARCODE_TYPE_DETECT(void)
{
	int32_t frame_buf_id;
	uint32_t input_bufadr, input_bufadr_va;
	uint32_t output_bufadr, output_bufadr_va;
	uint32_t work_bufadr1, work_bufadr2, work_bufadr16, work_bufadr_tmp;
	R_Box box;

	while ((frame_buf_id = R_BCD_CameraGetCaptureStatus()) == R_BCD_CAMERA_NOT_CAPTURED )
	{
		/* DO NOTHING */
	}

	g_barcode_type = BARCODE_UNKNOWN;
	app_contours[0].contours_no = 0;
	app_contours[1].contours_no = 0;
	g_contour_id = -1;
	g_data_matrix = false;
	memset(&g_barcode_decode.result_buf, 0, ZXING_RESULT_BUF_SIZE);

	/* Set Start of Total Processing Time */
	PerformSetStartTime(TIME_TOTAL);
	//frame_buf_id = (frame_buf_id == 0) ? 1 : 0;
	input_bufadr_va        = (uint32_t)R_BCD_CameraGetFrameAddress(frame_buf_id);
	output_bufadr_va       = (uint32_t)R_BCD_LcdGetVramAddress(); //1/16 of input buffer size

	/* Convert Virtual Address(VA:Logical) to Physical address(PA) */
	R_MMU_VAtoPA((uint32_t)&work_buf[0], &work_bufadr1 );
	R_MMU_VAtoPA((uint32_t)&work_buf[1], &work_bufadr2 );
	R_MMU_VAtoPA((uint32_t)&work_buf[15], &work_bufadr16 );
	R_MMU_VAtoPA(input_bufadr_va, &input_bufadr);
	R_MMU_VAtoPA(output_bufadr_va, &output_bufadr);

	/*************************************/
	/*  bayer -> gray (1/4 thinning)      */
	/*************************************/
	PerformSetStartTime(TIME_B2G_TOTAL);
	//Use work_buf1[0]/work_buf1[1]/work_buf1[2] to save RGB image
	R_BCD_DRP_Bayer2Gray_thinning(input_bufadr, output_bufadr, R_BCD_CAMERA_WIDTH, R_BCD_CAMERA_HEIGHT);
	R_BCD_AeRunAutoExpousure(&ae_setting, (uint16_t)(ave_result[0] / ((R_BCD_CAMERA_WIDTH/DRP_RESIZE) * (R_BCD_CAMERA_HEIGHT/DRP_RESIZE))));
	PerformSetEndTime(TIME_B2G_TOTAL);

	/*************************************/
	/*  gray scale gaussian blur         */
	/*************************************/
	PerformSetStartTime(TIME_GAUSSIAN_TOTAL);
	//R_BCD_DRP_GaussianBlur(output_bufadr,output_bufadr,R_BCD_CAMERA_WIDTH/DRP_RESIZE,R_BCD_CAMERA_HEIGHT/DRP_RESIZE,GAUSSIAN_TIME);
	PerformSetEndTime(TIME_GAUSSIAN_TOTAL);


	/*************************************/
	/*     binarization                  */
	/*************************************/
	PerformSetStartTime(TIME_BINARIZATION_TOTAL);
	work_bufadr_tmp = (g_display_mode == DM_GRAY) ? work_bufadr1 : output_bufadr;
	//R_BCD_DRP_Binarization_fixed(output_bufadr,output_bufadr,(R_BCD_CAMERA_WIDTH/DRP_RESIZE), (R_BCD_CAMERA_HEIGHT/DRP_RESIZE),DRP_R_BINARY_THRESHOLD);
	R_BCD_DRP_Binarization(output_bufadr,work_bufadr_tmp,work_bufadr2,(R_BCD_CAMERA_WIDTH/DRP_RESIZE), (R_BCD_CAMERA_HEIGHT/DRP_RESIZE),DRP_R_BINARY_RANGE);
	PerformSetEndTime(TIME_BINARIZATION_TOTAL);

	/*************************************/
	/*     Dilate			            */
	/*************************************/
	PerformSetStartTime(TIME_DILATE_TOTAL);
	//when dilate time=1, output buffer is 2nd parameter
	//When dilate time=2, output buffer is 3rd parameter
	//When dilate time=3, output buffer is 2nd parameter
	//... ... output buffer toggle between the 2nd / 3rd parameter
	R_BCD_DRP_Dilate(work_bufadr_tmp, work_bufadr_tmp, work_bufadr_tmp, (R_BCD_CAMERA_WIDTH/DRP_RESIZE), (R_BCD_CAMERA_HEIGHT/DRP_RESIZE), 1);
	PerformSetEndTime(TIME_DILATE_TOTAL);

	/*************************************/
	/*     Find Contours                 */
	/*************************************/
	PerformSetStartTime(TIME_FINDC_TOTAL);
#if 0 //def FINDCONTOUR_DO_DRP
	/* Initialize result buffer            */
	R_BCD_DrpCleanContours();
	/* Run find contours on DRP            */
	R_BCD_DRP_FindContours(work_bufadr1, work_bufadr2, (R_BCD_CAMERA_WIDTH/DRP_RESIZE), (R_BCD_CAMERA_HEIGHT/DRP_RESIZE), NULL, 0);
	app_contours[0].contours_no = R_BCD_DRP_Contours_2_Box(0, (R_BCD_CAMERA_WIDTH/DRP_RESIZE), (R_BCD_CAMERA_HEIGHT/DRP_RESIZE));
#else
	R_CACHE_L1DataInvalidLine((uint8_t*)work_bufadr_tmp,(R_BCD_CAMERA_WIDTH/DRP_RESIZE)*(R_BCD_CAMERA_HEIGHT/DRP_RESIZE));
	app_contours[0].contours_no = cv_findcontours_boundingrect((uint8_t*)work_bufadr_tmp, (R_BCD_CAMERA_WIDTH/DRP_RESIZE), (R_BCD_CAMERA_HEIGHT/DRP_RESIZE), 0);
#endif
	PerformSetEndTime(TIME_FINDC_TOTAL);

	/*************************************/
	/*     Cropping Image                */
	/*************************************/
	PerformSetStartTime(TIME_CROPPING_TOTAL);
	//only process the 1st contour(TBD)
	if (app_contours[0].contours_no > 0)
	{
		//select contour
		g_contour_id = select_contour();
		if (g_contour_id != -1)
		{
			//barcode_rect is the position info on original image
			get_cropping_rect(0,g_contour_id, (R_Rect*)&barcode_rect);
			g_crop_rect = barcode_rect;
			R_BCD_DRP_Bayer2Gray_Cropping(input_bufadr, work_bufadr1, R_BCD_CAMERA_WIDTH, R_BCD_CAMERA_HEIGHT, barcode_rect.x, barcode_rect.y, barcode_rect.width, barcode_rect.height);
		}
	}
	PerformSetEndTime(TIME_CROPPING_TOTAL);

	/*************************************/
	/*     CV Affine or DRP Keystone     */
	/*************************************/
	PerformSetStartTime(TIME_AFFINE_TOTAL);
	if (g_contour_id != -1)
	{
		R_Box r_box = app_contours[0].box[g_contour_id];
		for (int i=0; i<4; i++)
		{
			r_box.box[i][0] = r_box.box[i][0] * DRP_RESIZE - barcode_rect.x;
			r_box.box[i][1] = r_box.box[i][1] * DRP_RESIZE - barcode_rect.y;
		}
		//option-1: use opencv wrapaffine
		//cv_affine_transform((uint8_t*)input_bufadr, (uint8_t*)work_bufadr3, (uint16_t*)&barcode_rect.width, (uint16_t*)&barcode_rect.height, r_box);
		//R_CACHE_L1DataCleanLine((void *)work_bufadr3, barcode_rect.width*barcode_rect.height);

		//option-2: use DRP keystone correction
		//R_BCD_DRP_Keystone_Correction(input_bufadr, work_bufadr1, (uint16_t*)&barcode_rect.width, (uint16_t*)&barcode_rect.height, r_box);

		//option3: use CPU keystone correction
		//As memory size limitation, couldn't allocate another 1/4 thinning buffer to save the keystone result,
		//Just save the result to the work_buf[15], so check whether work_buf[15] is already occupied by cropping frame (work_buffer1)
		if (barcode_rect.width * barcode_rect.height > (sizeof(work_buf[0]) * 15))
		{
			//overwrite might be happen, just skip this frame
			g_contour_id = -1;
		}
		else
		{
			R_CACHE_L1DataInvalidLine((uint8_t*)work_bufadr1,barcode_rect.width*barcode_rect.height);
			R_BCD_Keystone_Correction(work_bufadr1, work_bufadr16, (uint16_t*)&barcode_rect.width, (uint16_t*)&barcode_rect.height, r_box);
			R_CACHE_L1DataCleanLine((void *)work_bufadr16, barcode_rect.width*barcode_rect.height);
		}
	}
	PerformSetEndTime(TIME_AFFINE_TOTAL);

	/*************************************/
	/*     GaussianBlur 2                */
	/*************************************/
	PerformSetStartTime(TIME_GAUSSIAN2_TOTAL);
	if (g_contour_id != -1)
	{
		//performance consideration, skip gaussian for big image
		//if (barcode_rect.height < 96)
			//R_BCD_DRP_GaussianBlur(work_bufadr16,work_bufadr16,barcode_rect.width,barcode_rect.height,GAUSSIAN_TIME);
	}
	PerformSetEndTime(TIME_GAUSSIAN2_TOTAL);

	/*************************************/
	/*     Binarization                  */
	/*************************************/
	PerformSetStartTime(TIME_BINARIZATION2_TOTAL);
	if (g_contour_id != -1)
	{
		R_BCD_DRP_Binarization(work_bufadr16,work_bufadr1,work_bufadr2,barcode_rect.width,barcode_rect.height,DRP_R_BINARY_RANGE);
	}
	PerformSetEndTime(TIME_BINARIZATION2_TOTAL);

	/*************************************/
	/*     Find contour on affine img    */
	/*************************************/
	PerformSetStartTime(TIME_FINDC2_TOTAL);
	if (g_contour_id != -1)
	{
		/* Initialize result buffer            */
		R_BCD_DrpCleanContours();
		/* Run find contours on DRP            */
		//TODO: workbuf can not be input_bufadr, drp will not be terminated, bus conflict? MIPI write+DRP read+DRP write)
		//potential issue here, workbufadr1 might be overflow
		//TODO: As we use input_bufadr (work_bufadr3), when total process time > camera frame duration, will display garbage on output_bufadr, because, workbuf3 and new mipi input will write same buffer
		//R_BCD_DRP_FindContours(work_bufadr3, work_bufadr1, barcode_rect.width, barcode_rect.height, R_BCD_Histogram_HV, 1);
		R_BCD_DRP_FindContours(work_bufadr1, work_bufadr2, barcode_rect.width, barcode_rect.height, check_data_matrix2, 1);
		//app_contours[1].contours_no will be updated in R_BCD_DRP_FindContours()
	}
	PerformSetEndTime(TIME_FINDC2_TOTAL);

	/*************************************/
	/*     Find QR position symbol       */
	/*************************************/
	PerformSetStartTime(TIME_FIND_POSITION_SYMBOL);
	if (app_contours[1].contours_no != 0)
	{
		//filter out small rect
		app_contours[1].contours_no = R_BCD_SortContours(&app_contours[1].rect[0], app_contours[1].contours_no);
		if (app_contours[1].contours_no != 0)
		{
			g_barcode_type = R_BCD_FindPositionSymbo(&app_contours[1].rect[0], app_contours[1].contours_no, barcode_rect.width, barcode_rect.height);
		}
	}
	if (g_barcode_type == BARCODE_UNKNOWN && app_contours[0].contours_no > 0 && g_data_matrix != false)
	{
//		R_BCD_Histogram_HV((uint8_t*)work_bufadr16, barcode_rect.width, barcode_rect.height);
//		g_barcode_type = check_data_matrix(barcode_rect.width, barcode_rect.height);
		g_barcode_type = BARCODE_DATA_MATRIX;
	}
	PerformSetEndTime(TIME_FIND_POSITION_SYMBOL);

	PerformSetStartTime(TIME_ZXING_TOTAL);
	g_barcode_decode.barcode_contour_detected = g_contour_id;
	g_barcode_decode.type = g_barcode_type;
	g_barcode_decode.camera_bayer_buf = input_bufadr;
	g_barcode_decode.cropped_barcode_grayscale_buf = work_bufadr16;
	g_barcode_decode.cropped_barcode_buf_height = barcode_rect.height;
	g_barcode_decode.cropped_barcode_buf_width = barcode_rect.width;
	decode_barcode(&g_barcode_decode);
	PerformSetEndTime(TIME_ZXING_TOTAL);

	PerformSetEndTime(TIME_TOTAL);

DETECTION_END:

	/******************************************/
	/*  Clear Status & working graphic Buffer */
	/******************************************/
	/* Clear the current capture state and enable the detection of the next capture completion */
	R_BCD_CameraClearCaptureStatus();
	/* Display time taken for image processing */
	R_BCD_LcdClearGraphicsBuffer();
	R_BCD_LcdClearGraphicsBuffer2();

	/*********************************************/
	/*     Draw on working Graphic Buffer        */
	/*********************************************/
	//Draw Processing Time to display
	//R_BCD_DrawProcessingTime();
	PerformSetStartTime(TIME_DEBUG_DRAW);
	R_BCD_DrawProcessingTime_SVGA();

	if (app_contours[0].contours_no != 0 && g_display_mode != DM_BIN_BB)
	{
		draw_contour_box(0);
		draw_contour_rect(0);
	}


	if (g_display_mode == DM_BIN_BB)
	{
		if (g_contour_id != -1)
		{
			draw_contour_rect(1);
			for (int i=0; i<barcode_rect.height; i++)
			{
				memcpy((uint8_t*)(output_bufadr+i*R_BCD_CAMERA_WIDTH/DRP_RESIZE), (uint8_t*)(work_bufadr16+i*barcode_rect.width), barcode_rect.width);
			}
			R_CACHE_L1DataCleanLine((void *)output_bufadr, R_BCD_CAMERA_WIDTH/DRP_RESIZE*barcode_rect.height);
		}
	}

	PerformSetEndTime(TIME_DEBUG_DRAW);



	/* Write the data(buf) on the cache to physical memory */
	//PerformSetStartTime(TIME_GRAPH_WRITE);
	R_CACHE_L1DataCleanLine(R_BCD_LcdGetOLVramAddress(), ((R_BCD_LCD_WIDTH2 * R_BCD_LCD_HEIGHT2) / 2));
	R_CACHE_L1DataCleanLine(R_BCD_LcdGetOLVram2Address(), ((R_BCD_LCD_WIDTH3 * R_BCD_LCD_HEIGHT3) / 2));
	//PerformSetEndTime(TIME_GRAPH_WRITE);

	/***************************************************/
	/* from displaying buffer switch to working buffer */
	/***************************************************/
	/* Display image processing result */
	R_BCD_LcdSwapVideoBuffer();
	/* Display overlay buffer written processing time */
	R_BCD_LcdSwapGraphicsBuffer();
	R_BCD_LcdSwapGraphicsBuffer2();

	//wait for buffer swap complete
	//PerformSetStartTime(TIME_DISPLAY_WAIT);
	while(R_BCD_ChkLcdSwapVideoBufferReq());
	while(R_BCD_ChkLcdSwapGraphicsBufferReq());
	while(R_BCD_ChkLcdSwapGraphicsBuffer2Req());
	//PerformSetEndTime(TIME_DISPLAY_WAIT);


}

/******************************************************************************
* Function Name: R_BCD_DrawProcessingTime SVGA
* Description  : Draw Processing Time to display for SVGA LCD Display
* Arguments    : mode
*                switch of running mode(motion detection or object detection)
* Return Value : -
******************************************************************************/
void R_BCD_DrawProcessingTime_SVGA(void)
{
    uint8_t buf[300];
    uint32_t us_total, us_dl, us_fps;
    uint32_t us_total2, us_dl2;
    uint32_t us_total3, us_total4, us_total5, us_total6;
    uint32_t frame_rate;

    for (int i=0; i<TIME_INVALID; i++)
    {
    	g_execute_time[i] = PerformGetElapsedTime_us(i);
    }

	sprintf((void *)&buf[0],"(1)B2G    :%d.%03dms(DL:%d.%03dms) (2)BINARY :%d.%03dms(DL:%d.%03dms)",
			(int)(g_execute_time[TIME_B2G_TOTAL] / 1000), (int)(g_execute_time[TIME_B2G_TOTAL] - ((g_execute_time[TIME_B2G_TOTAL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_B2G_DL]    / 1000), (int)(g_execute_time[TIME_B2G_DL] -    ((g_execute_time[TIME_B2G_DL]    / 1000) * 1000)),
			(int)(g_execute_time[TIME_BINARIZATION_TOTAL] / 1000), (int)(g_execute_time[TIME_BINARIZATION_TOTAL] - ((g_execute_time[TIME_BINARIZATION_TOTAL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_BINARIZATION_DL]    / 1000), (int)(g_execute_time[TIME_BINARIZATION_DL] -    ((g_execute_time[TIME_BINARIZATION_DL]    / 1000) * 1000))  );
	R_BCD_LcdWriteString(&buf[0], 2, 0, GREEN);

	sprintf((void *)&buf[0],"(3)DILATE :%d.%03dms(DL:%d.%03dms) (4)FINDC1 :%d.%03dms(CPU       )",
			(int)(g_execute_time[TIME_DILATE_TOTAL] / 1000), (int)(g_execute_time[TIME_DILATE_TOTAL] - ((g_execute_time[TIME_DILATE_TOTAL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_DILATE_DL]    / 1000), (int)(g_execute_time[TIME_DILATE_DL] -    ((g_execute_time[TIME_DILATE_DL]    / 1000) * 1000)),
			(int)(g_execute_time[TIME_FINDC_TOTAL] / 1000), (int)(g_execute_time[TIME_FINDC_TOTAL] - ((g_execute_time[TIME_FINDC_TOTAL] / 1000) * 1000)));
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT, GREEN);

	sprintf((void *)&buf[0],"(5)CROP   :%d.%03dms(DL:%d.%03dms) (6)KeyStone:%d.%03dms(CPU      )",
			(int)(g_execute_time[TIME_CROPPING_TOTAL] / 1000), (int)(g_execute_time[TIME_CROPPING_TOTAL] - ((g_execute_time[TIME_CROPPING_TOTAL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_CROPPING_DL]    / 1000), (int)(g_execute_time[TIME_CROPPING_DL] -    ((g_execute_time[TIME_CROPPING_DL]    / 1000) * 1000)),
			(int)(g_execute_time[TIME_AFFINE_TOTAL] / 1000), (int)(g_execute_time[TIME_AFFINE_TOTAL] - ((g_execute_time[TIME_AFFINE_TOTAL] / 1000) * 1000)));
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*2, GREEN);

	sprintf((void *)&buf[0],"(7)BINARY2:%d.%03dms(DL:%d.%03dms) (8)FINDC2 :%d.%03dms(DL:%d.%03dms)",
			(int)(g_execute_time[TIME_BINARIZATION2_TOTAL] / 1000), (int)(g_execute_time[TIME_BINARIZATION2_TOTAL] - ((g_execute_time[TIME_BINARIZATION2_TOTAL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_BINARIZATION_DL]    / 1000), (int)(g_execute_time[TIME_BINARIZATION_DL] -    ((g_execute_time[TIME_BINARIZATION_DL]    / 1000) * 1000)),
			(int)(g_execute_time[TIME_FINDC2_TOTAL] / 1000), (int)(g_execute_time[TIME_FINDC2_TOTAL] - ((g_execute_time[TIME_FINDC2_TOTAL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_FINDC_DL]    / 1000), (int)(g_execute_time[TIME_FINDC_DL] -    ((g_execute_time[TIME_FINDC_DL]    / 1000) * 1000)));
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*3, GREEN);

	sprintf((void *)&buf[0],"(11)ZXING :%d.%03dms(CPU       ) (12)TOTAL :%d.%03dms",
//			(int)(g_execute_time[TIME_FIND_POSITION_SYMBOL] / 1000), (int)(g_execute_time[TIME_FIND_POSITION_SYMBOL] - ((g_execute_time[TIME_FIND_POSITION_SYMBOL] / 1000) * 1000)),
			(int)(g_execute_time[TIME_ZXING_TOTAL] / 1000), (int)(g_execute_time[TIME_ZXING_TOTAL] - ((g_execute_time[TIME_ZXING_TOTAL] / 1000) * 1000)),
			//(int)(us_dl    / 1000), (int)(us_dl - 	 ((us_dl 	/ 1000) * 1000)),
			(int)(g_execute_time[TIME_TOTAL]  / 1000), (int)(g_execute_time[TIME_TOTAL] - ((g_execute_time[TIME_TOTAL] / 1000) * 1000)));
			//(int)(us_dl2    / 1000), (int)(us_dl2 -    ((us_dl2    / 1000) * 1000)));
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*4, GREEN);

	sprintf((void *)&buf[0],"BarCode Type: %s", barcode_name[g_barcode_type]);
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*5, GREEN);


	if (app_contours[0].contours_no != 0)
	{
		sprintf((void *)&buf[0],"BarCode Result: %s", \
				g_barcode_decode.result_buf);
	}
	else
		sprintf((void *)&buf[0],"BarCode Result:");
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*6, GREEN);

	frame_rate = 1000000 / g_camera_vsync_us;
	sprintf((void *)&buf[0],"Camera frame rate: %d fps (%d ms)", \
			frame_rate, (int)(g_camera_vsync_us/1000));
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*7, GREEN);

	sprintf((void *)&buf[0],"Version: 2.1.0");
	R_BCD_LcdWriteString(&buf[0], 2, R_BCD_FONTDATA_HEIGHT*8, GREEN);
}


#ifdef SAVE_IMAGE_TO_SD
/******************************************************************************
* Function Name: sd_save_bmp_func
* Description  : Convert raw image data to BMP file format,
*                and save the data to MicroSD.
* Arguments    : -
* Return Value : -
******************************************************************************/
static void sd_save_bmp_func ( uint8_t* p_buf)
{
    int_t retval;
    int_t counter_x;
    int_t counter_y;
    uint8_t *ispout_p = (uint8_t *)(p_buf);
    retval = R_BCD_SD_Init();    /* SD Mount */
    uint32_t temp;

    /* Create an empty file */
    if( retval == 0 )
    {
        retval = R_BCD_SD_CreateFile(&sd_filename[0],&sd_savenum_bmp,1);

        if( retval != 0)
        {
            R_BCD_SD_Uninit();    /* SD UnMount */
        }
    }

    if( retval == 0 )
    {
		/* Save the header of bmp file format */
		retval = R_BCD_SD_Savedata(bmpheader,54);

		/* 32*40 */
		/* Save the BMP format from line 39 */
		ispout_p = ispout_p + 32 * 39;
		for( counter_y = 0; counter_y < 40; counter_y++)
		{
			for( counter_x = 0; counter_x < 32; counter_x++)
			{
				/* Convert 1-byte grayscale image data to 3-byte RGB format image data */
				CnvBmp_buffer[(counter_y * 32 + counter_x)*3] = *ispout_p;
				CnvBmp_buffer[(counter_y * 32 + counter_x)*3+1] = *ispout_p;
				CnvBmp_buffer[(counter_y * 32 + counter_x)*3+2] = *ispout_p;
				ispout_p++;
			}
			ispout_p = ispout_p - 32 * 2;
		}
		retval = R_BCD_SD_Savedata(CnvBmp_buffer, 1280*3);
    }

    /* File close */
    R_BCD_SD_WriteEnd();
}
#endif


/* End of File */
