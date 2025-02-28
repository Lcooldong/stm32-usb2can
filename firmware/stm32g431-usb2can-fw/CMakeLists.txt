cmake_minimum_required(VERSION 3.5.0)

# Setup compiler settings
set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            ON)
set(PROJ_PATH                       ${CMAKE_CURRENT_SOURCE_DIR})
message("Build type: "              ${CMAKE_BUILD_TYPE})

#
# Core project settings
#
project(stm32g431-usb2can-fw VERSION 0.1.0)
enable_language(C CXX ASM)

#
# Core MCU flags, CPU, instruction set and FPU setup
# Needs to be set properly for your MCU
#
set(CPU_PARAMETERS
    -mthumb

    # This needs attention to properly set for used MCU
    -mcpu=cortex-m4
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# Set linker script
set(linker_script_SRC               ${PROJ_PATH}/src/bsp/ldscript/STM32G431CBUX_FLASH.ld)
set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

#
# List of source files to compile
#
file(GLOB SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.c
    src/ap/*.cpp
    src/ap/*.c
    src/bsp/*.c
    src/bsp/device/*.c
    
    
)

file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  src/ap/*.cpp
  src/ap/*.c
  src/bsp/*.s
  src/common/*.c
  src/hw/*.c
  src/lib/STM32G4xx_HAL_Driver/Src/*.c
  src/lib/STM32_USB_Device_Library/Core/Src/*.c
  src/bsp/FreeRTOS/Source/*.c
#   src/lib/littlefs/*.c
)

file(GLOB SRC_FILES_EXCLUDE CONFIGURE_DEPENDS
    src/lib/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_timebase_tim_template.c
    src/lib/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_msp_template.c
    src/bsp/FreeRTOS/Source/portable/MemMang/heap_1.c
    src/bsp/FreeRTOS/Source/portable/MemMang/heap_2.c
    src/bsp/FreeRTOS/Source/portable/MemMang/heap_3.c
    src/bsp/FreeRTOS/Source/portable/MemMang/heap_5.c
    # src/lib/STM32_USB_Device_LibraryCore/Src/usbd_conf_template.c
    # src/lib/STM32_USB_Device_LibraryCore/Src/usbd_desc_template.c
)

set(EXCLUDE_PATHS 
    ${SRC_FILES_EXCLUDE}
)

foreach(TMP_PATH ${SRC_FILES_RECURSE}) 
  foreach(EXCLUDE_PATH ${EXCLUDE_PATHS}) 
    string(FIND ${TMP_PATH} ${EXCLUDE_PATH} RESULT) 
    if(NOT ${RESULT} EQUAL -1) 
      list(REMOVE_ITEM SRC_FILES_RECURSE ${TMP_PATH}) 
    endif() 
  endforeach(EXCLUDE_PATH) 
endforeach(TMP_PATH)
#
# Include directories
#
set(include_path_DIRS
    ${PROJ_PATH}/src
    ${PROJ_PATH}/src/ap
    ${PROJ_PATH}/src/ap/mode
    # ${PROJ_PATH}/src/ap/thread
    ${PROJ_PATH}/src/bsp
    ${PROJ_PATH}/src/bsp/device
    ${PROJ_PATH}/src/bsp/FreeRTOS
    ${PROJ_PATH}/src/bsp/FreeRTOS/Source/CMSIS_RTOS
    ${PROJ_PATH}/src/bsp/FreeRTOS/Source/include
    ${PROJ_PATH}/src/bsp/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${PROJ_PATH}/src/common
    ${PROJ_PATH}/src/common/core
    ${PROJ_PATH}/src/common/hw/inc
    ${PROJ_PATH}/src/hw
    ${PROJ_PATH}/src/hw/driver
    ${PROJ_PATH}/src/hw/driver/usb
    ${PROJ_PATH}/src/hw/driver/usb/usb_cdc
    ${PROJ_PATH}/src/lib
    ${PROJ_PATH}/src/lib/CMSIS/Include
    ${PROJ_PATH}/src/lib/CMSIS/Device/ST/STM32G4xx/Include
    ${PROJ_PATH}/src/lib/STM32G4xx_HAL_Driver/Inc
    ${PROJ_PATH}/src/lib/STM32_USB_Device_Library/Core/Inc
    # Put here your include dirs, one in each line, relative to CMakeLists.txt file location
)

#
# Symbols definition
#
set(symbols_SYMB
    -DSTM32G431xx
    # STM32G431xx
    # Put here your symbols (preprocessor defines), one in each line
    # Encapsulate them with double quotes for safety purpose
)

# Executable files
add_executable(${EXECUTABLE} 
  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
)

# Include paths
target_include_directories(${EXECUTABLE} PRIVATE ${include_path_DIRS})

# Project symbols
target_compile_definitions(${EXECUTABLE} PRIVATE ${symbols_SYMB})

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    # Full debug configuration
    -Og -g3 -ggdb
)

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
    -T${linker_script_SRC}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -u _printf_float                # STDIO float formatting support
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage
)

# Execute post-build to print size
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>
)

# Convert output to hex and binary
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.hex
)

# Convert to bin file -> add conditional check?
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.bin
)

