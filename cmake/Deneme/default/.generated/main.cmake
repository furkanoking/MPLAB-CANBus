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


# Main target for this project
add_executable(Deneme_default_image_JZPqOJBM ${Deneme_default_library_list})

set_target_properties(Deneme_default_image_JZPqOJBM PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${Deneme_default_output_dir}")
target_link_libraries(Deneme_default_image_JZPqOJBM PRIVATE ${Deneme_default_default_XC32_FILE_TYPE_link})

# Add the link options from the rule file.
Deneme_default_link_rule( Deneme_default_image_JZPqOJBM)

# Add bin2hex target for converting built file to a .hex file.
string(REGEX REPLACE [.]elf$ .hex Deneme_default_image_name_hex ${Deneme_default_image_name})
add_custom_target(Deneme_default_Bin2Hex ALL
    COMMAND ${MP_BIN2HEX} \"${Deneme_default_output_dir}/${Deneme_default_image_name}\"
    BYPRODUCTS ${Deneme_default_output_dir}/${Deneme_default_image_name_hex}
    COMMENT "Convert built file to .hex")
add_dependencies(Deneme_default_Bin2Hex Deneme_default_image_JZPqOJBM)



