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
* Copyright (C) 2021 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* System Name : [RZ/A2M] barcode type detection Sample Project
* File Name   : readme-e.txt
*******************************************************************************/
/*******************************************************************************
*
* History     : May. 15,2020 Rev.1.00.00    First edition issued
*             : Dec. 15,2020 Rev.1.01.00    Updated to e2 studio 2020-07
*             : Jan. 12,2021 Rev.2.00.00    Add QR/AZTEC/DataMatrix Decoding
*             : Aug, 03,2021 Rev.2.01.00	Update device driver to the latest version
*******************************************************************************/

1. Before Use

  This sample code has been tested using the GR-MANGO Rev.B board
  with the RZ/A2M group R7S921058. 
  Use this readme file, the application note, and the sample code as a reference 
  for your software development.


  ****************************** CAUTION ******************************
   This sample code are all reference, and no one to guarantee the 
   operation. Please use this sample code for the technical 
   reference when customers develop software.
  ****************************** CAUTION ******************************


2. System Verification

  This sample code was developed and tested with the following components:

    CPU                                 : RZ/A2M
    Board                               : GR-MANGO Rev.B board
    Compiler                            : GNU ARM Embedded 6.3.1.20170620
    Integrated development environment  : e2 studio 2021-04
    Camera                              : Raspberry Pi Camera V2
    Monitor                             : Monitor compatible with Full-WXGA(1366*768) resolution

3. About Sample Code

    This sample program uses RZ/A2M's DRP to detects 5 different barcode encoding formats and decode QR/AZTEC/DataMatrix format.
    Barcode type detection support: QR, MicroQR, AZTEC, DataMatrix, HanXin
	Barcode type decoding support: QR, AZTEC, DataMatrix

    The application program is executed from the loader program after the Octa memory controller register 
    setting processing is performed by the loader program.
    The loader program is stored in the following folder.
      ["Project name"\generate\gr_mango_boot\mbed_sf_boot.c]

   Refer to application notes for the details about the sample code. 


4. Operation Confirmation Conditions

  (1) Boot mode
    - Boot mode 6
      Boot from OctaFlash connected to OctaFlash space.
      * The program can not be operated if the boot mode except the above is specified.

  (2) Operating frequency
      The RZ/A2M clock pulse oscillator module is set to ensure that the RZ/A2M clocks on GR-MANGO
      have the following frequencies:
      (The frequencies indicate the values in the state that the clock with 24MHz
      is input to the EXTAL pin in RZ/A2M clock mode 1.)
      - CPU clock (I clock)                 : 528MHz
      - Image processing clock (G clock)    : 264MHz
      - Internal bus clock (B clock)        : 132MHz
      - Peripheral clock1 (P1 clock)        :  66MHz
      - Peripheral clock0 (P0 clock)        :  33MHz
      - OM_SCLK                             : 132MHz
      - CKIO                                : 132MHz

  (3) OctaFlash (connected to OctaFlash space) used
    - Manufacturer  : Macronix Inc.
    - Product No.   : MX25UW12845GXDIO0

  (4) Setting for cache
      Initial setting for the L1 and L2 caches is executed by the MMU. 
      Refer to the "RZ/A2M group Example of Initialization" application note about "Setting for MMU" for 
      the valid/invalid area of L1 and L2 caches.


5. Operational Procedure

  Use the following procedure to execute this sample code.

  Refer Quick Start Guide(R01QS0042) for building, downloading, and running this application.

/* End of File */
