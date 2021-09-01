r
h

// Download loader into QSPI flash
loadbin .\Renesas\rza2m_sflash_boot_loader_gcc.bin,0x20000000
r
h

// Download application into QSPI flash
loadbin .\Renesas\rza2m_barcode_type_freertos_gcc_evk.bin,0x20010000
r
g
q
exit
