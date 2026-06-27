set(DEPENDENT_MP_BIN2HEXDeneme_default_JZPqOJBM "c:/Program Files/Microchip/xc32/v5.10/bin/xc32-bin2hex.exe")
set(DEPENDENT_DEPENDENT_TARGET_ELFDeneme_default_JZPqOJBM ${CMAKE_CURRENT_LIST_DIR}/../../../../out/Deneme/default.elf)
set(DEPENDENT_TARGET_DIRDeneme_default_JZPqOJBM ${CMAKE_CURRENT_LIST_DIR}/../../../../out/Deneme)
set(DEPENDENT_BYPRODUCTSDeneme_default_JZPqOJBM ${DEPENDENT_TARGET_DIRDeneme_default_JZPqOJBM}/${sourceFileNameDeneme_default_JZPqOJBM}.c)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRDeneme_default_JZPqOJBM}/${sourceFileNameDeneme_default_JZPqOJBM}.c
    COMMAND ${DEPENDENT_MP_BIN2HEXDeneme_default_JZPqOJBM} --image ${DEPENDENT_DEPENDENT_TARGET_ELFDeneme_default_JZPqOJBM} --image-generated-c ${sourceFileNameDeneme_default_JZPqOJBM}.c --image-generated-h ${sourceFileNameDeneme_default_JZPqOJBM}.h --image-copy-mode ${modeDeneme_default_JZPqOJBM} --image-offset ${addressDeneme_default_JZPqOJBM} 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRDeneme_default_JZPqOJBM}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFDeneme_default_JZPqOJBM})
add_custom_target(
    dependent_produced_source_artifactDeneme_default_JZPqOJBM 
    DEPENDS ${DEPENDENT_TARGET_DIRDeneme_default_JZPqOJBM}/${sourceFileNameDeneme_default_JZPqOJBM}.c
    )
