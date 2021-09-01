/*******************************************************************************
* File Name   : r_drp_bitwiseand.h
* Description : IF header of median filter.
******************************************************************************/
/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
/******************************************************************************
Typedef definitions
******************************************************************************/
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;

typedef struct
{
    uint32_t  iaddr1;    /* Input image data address 1     */
    uint32_t  iaddr2;    /* Input image data address 2     */
    uint32_t  oaddr;    /* Output image data address       */
    uint16_t  width;    /* Image width in pixels           */
    uint16_t  height;   /* Image height in pixels          */
}r_drp_bitwiseand_t;

