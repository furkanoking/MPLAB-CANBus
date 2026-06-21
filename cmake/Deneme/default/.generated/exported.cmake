set(DEPENDENT_MP_BIN2HEXDeneme_default_l4mS_0Xl "/Applications/microchip/xc32/v4.60/bin/xc32-bin2hex")
set(DEPENDENT_DEPENDENT_TARGET_ELFDeneme_default_l4mS_0Xl ${CMAKE_CURRENT_LIST_DIR}/../../../../out/Deneme/default.elf)
set(DEPENDENT_TARGET_DIRDeneme_default_l4mS_0Xl ${CMAKE_CURRENT_LIST_DIR}/../../../../out/Deneme)
set(DEPENDENT_BYPRODUCTSDeneme_default_l4mS_0Xl ${DEPENDENT_TARGET_DIRDeneme_default_l4mS_0Xl}/${sourceFileNameDeneme_default_l4mS_0Xl}.c)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRDeneme_default_l4mS_0Xl}/${sourceFileNameDeneme_default_l4mS_0Xl}.c
    COMMAND ${DEPENDENT_MP_BIN2HEXDeneme_default_l4mS_0Xl} --image ${DEPENDENT_DEPENDENT_TARGET_ELFDeneme_default_l4mS_0Xl} --image-generated-c ${sourceFileNameDeneme_default_l4mS_0Xl}.c --image-generated-h ${sourceFileNameDeneme_default_l4mS_0Xl}.h --image-copy-mode ${modeDeneme_default_l4mS_0Xl} --image-offset ${addressDeneme_default_l4mS_0Xl} 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRDeneme_default_l4mS_0Xl}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFDeneme_default_l4mS_0Xl})
add_custom_target(
    dependent_produced_source_artifactDeneme_default_l4mS_0Xl 
    DEPENDS ${DEPENDENT_TARGET_DIRDeneme_default_l4mS_0Xl}/${sourceFileNameDeneme_default_l4mS_0Xl}.c
    )
