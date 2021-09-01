/*******************************************************************************
* File Name   : r_drp_opening_closing.h
* Description : IF header of morphologyEx opening and closing.
*               This is a 3 tiles DRP librry
******************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
/******************************************************************************
Typedef definitions
******************************************************************************/
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;

typedef struct
{
    uint32_t  image_addr;	/* image address 8BPP	*/
	uint32_t  out_addr;		/* output address 8BPP	*/

    uint16_t  image_w;		/* multiple of 2, imge_w * kernel_h < 4096	*/
    uint16_t  image_h;		/* multiple of 2					 		*/
	uint8_t   kernel_w;		/* kernel width	odd number and <=15	 		*/
    uint8_t   kernel_h;		/* kernel height odd number and <=15 		*/
    uint8_t   top_bot;		/* bit0 - top frame bit1 - bot frame 		*/
	uint8_t   mode;			/* 0-opening  1-closing 			 		*/
} r_drp_opening_closing_t;

