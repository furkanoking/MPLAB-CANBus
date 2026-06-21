include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(Deneme_default_library_list )

# Handle files with suffix s, for group default-XC32
if(Deneme_default_default_XC32_FILE_TYPE_assemble)
add_library(Deneme_default_default_XC32_assemble OBJECT ${Deneme_default_default_XC32_FILE_TYPE_assemble})
    Deneme_default_default_XC32_assemble_rule(Deneme_default_default_XC32_assemble)
    list(APPEND Deneme_default_library_list "$<TARGET_OBJECTS:Deneme_default_default_XC32_assemble>")

endif()

# Handle files with suffix S, for group default-XC32
if(Deneme_default_default_XC32_FILE_TYPE_assembleWithPreprocess)
add_library(Deneme_default_default_XC32_assembleWithPreprocess OBJECT ${Deneme_default_default_XC32_FILE_TYPE_assembleWithPreprocess})
    Deneme_default_default_XC32_assembleWithPreprocess_rule(Deneme_default_default_XC32_assembleWithPreprocess)
    list(APPEND Deneme_default_library_list "$<TARGET_OBJECTS:Deneme_default_default_XC32_assembleWithPreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC32
if(Deneme_default_default_XC32_FILE_TYPE_compile)
add_library(Deneme_default_default_XC32_compile OBJECT ${Deneme_default_default_XC32_FILE_TYPE_compile})
    Deneme_default_default_XC32_compile_rule(Deneme_default_default_XC32_compile)
    list(APPEND Deneme_default_library_list "$<TARGET_OBJECTS:Deneme_default_default_XC32_compile>")

endif()

# Handle files with suffix cpp, for group default-XC32
if(Deneme_default_default_XC32_FILE_TYPE_compile_cpp)
add_library(Deneme_default_default_XC32_compile_cpp OBJECT ${Deneme_default_default_XC32_FILE_TYPE_compile_cpp})
    Deneme_default_default_XC32_compile_cpp_rule(Deneme_default_default_XC32_compile_cpp)
    list(APPEND Deneme_default_library_list "$<TARGET_OBJECTS:Deneme_default_default_XC32_compile_cpp>")

endif()

# Handle files with suffix [cC], for group default-XC32
if(Deneme_default_default_XC32_FILE_TYPE_dependentObject)
add_library(Deneme_default_default_XC32_dependentObject OBJECT ${Deneme_default_default_XC32_FILE_TYPE_dependentObject})
    Deneme_default_default_XC32_dependentObject_rule(Deneme_default_default_XC32_dependentObject)
    list(APPEND Deneme_default_library_list "$<TARGET_OBJECTS:Deneme_default_default_XC32_dependentObject>")

endif()

# Handle files with suffix elf, for group default-XC32
if(Deneme_default_default_XC32_FILE_TYPE_bin2hex)
add_library(Deneme_default_default_XC32_bin2hex OBJECT ${Deneme_default_default_XC32_FILE_TYPE_bin2hex})
    Deneme_default_default_XC32_bin2hex_rule(Deneme_default_default_XC32_bin2hex)
    list(APPEND Deneme_default_library_list "$<TARGET_OBJECTS:Deneme_default_default_XC32_bin2hex>")

endif()


# Main target for this project
add_executable(Deneme_default_image_l4mS_0Xl ${Deneme_default_library_list})

if(NOT CMAKE_HOST_WIN32)
    set_target_properties(Deneme_default_image_l4mS_0Xl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${Deneme_default_output_dir}")
endif()
set_target_properties(Deneme_default_image_l4mS_0Xl PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf")
target_link_libraries(Deneme_default_image_l4mS_0Xl PRIVATE ${Deneme_default_default_XC32_FILE_TYPE_link})

# Add the link options from the rule file.
Deneme_default_link_rule( Deneme_default_image_l4mS_0Xl)

# Call bin2hex function from the rule file
Deneme_default_bin2hex_rule(Deneme_default_image_l4mS_0Xl)
if(CMAKE_HOST_WIN32)
    add_custom_command(
        TARGET Deneme_default_image_l4mS_0Xl
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${Deneme_default_output_dir}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Deneme_default_image_l4mS_0Xl> ${Deneme_default_output_dir}/${Deneme_default_original_image_name}
        BYPRODUCTS ${Deneme_default_output_dir}/${Deneme_default_original_image_name}
        COMMENT "Copying elf to out location")
    set_property(
        TARGET Deneme_default_image_l4mS_0Xl
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES
        ${Deneme_default_output_dir}/${Deneme_default_original_image_name})
endif()

