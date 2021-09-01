    @echo off
    echo LOAD BOOT LOADER INTO Single QSPI FLASH 
    echo ===================================================================
    echo ***    Remember to update the JLink.exe path specified          ***
    echo ***    in this batch file if using a different version than     ***
    echo ***    that installed by the RSK+RZA1H_V2 installer.            ***
    echo ***    This batch file was tested and works with Ver 6.12f      ***
    echo ***    of the JLink.exe                                         ***
    echo ***                                                             ***
    echo ***    Please refer to the RSK+RZA1H Tutorial Manual            *** 
    echo ***    for more information on generating the binary files.     ***
    echo ===================================================================
    echo ------------------------------------------------------------------------
    setlocal enableextensions 

    set programmer2=JLink.exe
    set progcmdfile=%1
    set programmer=.\JLink\

    "%programmer%%programmer2%" -speed 12000 -if JTAG -JTAGConf -1,-1 -device R7S921053VCBG -CommanderScript %progcmdfile%
    echo ------------------------------------------------------------------------


 