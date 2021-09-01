# rza2m_barcode_type_detection_demo

Barcode Type Detection Demo with [RZ/A2M](https://www.renesas.com/eu/en/products/microcontrollers-microprocessors/rz-cortex-a-mpus/rza2m-high-speed-embedded-ai-based-image-processing-microprocessors-dynamically-reconfigurable-processor)  

<p align="center"> 
	<img src="https://github.com/renesas-rz/rza2m_barcode_type_detection_demo/blob/master/barcode_type_detection_demo.jpg" alt="">
	<img src="https://github.com/renesas-rz/rza2m_barcode_type_detection_demo/blob/master/barcode_type_detection_concept.jpg" alt="">
</p>

This demo can quickly identify 5 kinds of barcode encoding formats including QR, Micro QR, DataMatrix, AZTec and HanXin code in 8~10ms when data is received from a 1280x720 (1 million) camera 

This demo also integrated ZXing as backend of barcode decoding. When barcode type detection module successfully detected a barcode image, it will output barcode type and cropped barcode image to ZXing module. It will reduce ZXing decoding time. In this sample, it can support 3 kinds of barcode decoding: QR, DataMatrix and AZTec

Finally, the result is shown on a display (HDMI monitor).


### Supported platforms:

1. [RZ/A2M Evaluation Board Kit](https://www.renesas.com/eu/en/products/microcontrollers-microprocessors/rz-cortex-a-mpus/rza2m-evaluation-kit-rza2m-evaluation-kit)  
   [e²studio](https://www.renesas.com/eu/en/software-tool/e-studio) project name: “rza2m_barcode_type_freertos_gcc_evk”  
   ([e²studio User’s Manual: Getting Started Guide / RZ Family](https://www.renesas.com/eu/en/document/mat/e-studio-integrated-development-environment-users-manual-getting-started-renesas-mcu-rz-family?language=en&r=488826))

2. [Gadget Renesas board “GR-MANGO”](https://www.renesas.com/eu/en/products/gadget-renesas/boards/gr-mango)  
   [e²studio](https://www.renesas.com/eu/en/software-tool/e-studio) project name: “rza2m_barcode_type_freertos_gcc_grmango”  
   ([e²studio for GR-MANGO](https://github.com/renesas-rz/rza2m_barcode_type_detection_demo/blob/master/rza2m_barcode_type_freertos_gcc_grmango/doc/EPSD-IMB-20-0107-02_RZA2M_SoftwarePackage_for_GR-MANGO_Development_Environment_Construction.pdf); [GR-MANGO 'hands-on'](https://github.com/renesas-rz/rza2m_barcode_type_detection_demo/blob/master/rza2m_barcode_type_freertos_gcc_grmango/doc/EPSD-IMB-20-0106-01_RZA2M_SoftwarePackage_for_GR-MANGO_Hands_on_Training.pdf))


### Key features:

Barcode Type Detection format support: QR, MicroQR, AZTEC, DataMatrix, HanXin.

Barcode Decoding format support: QR, AZTEC, DataMatrix.

Barcode Type Detection performance: 8 to 12ms.

Total duration(detection+decoding): 12 to 16ms.

([DRP](https://www.renesas.com/eu/en/application/technologies/drp)).



### More:

Please check the 
[Application Note](https://github.com/renesas-rz/rza2m_barcode_type_detection_demo/blob/master/rza2m_barcode_type_freertos_gcc_evk/doc/RZA2M_Barcode_Type_Detection_demo_ApplicationNote_20210816.pdf)
for more details.

Please contact your local Renesas Sales representative in case you like to get more information about [RZ/A2M](https://www.renesas.com/eu/en/products/microcontrollers-microprocessors/rz-cortex-a-mpus/rza2m-high-speed-embedded-ai-based-image-processing-microprocessors-dynamically-reconfigurable-processor), [DRP](https://www.renesas.com/eu/en/application/technologies/drp)

## Q&A
Please contact us below if you have any questions.　
&nbsp;[Q&A Forum](https://renesasrulz.com/rz/rz-a2m-drp/f/rz-a2m-and-drp-forum)  

