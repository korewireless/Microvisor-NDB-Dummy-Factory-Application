cmake_minimum_required(VERSION 3.12)

# Set application data
set(APP "Microvisor FreeRTOS Demo")
set(VERSION_NUMBER "1.0.0")
set(BUILD_NUMBER "0")

message("Building ${APP} ${VERSION_NUMBER} build ${BUILD_NUMBER}")

# Make app data available to the compiler
add_compile_definitions(APP_NAME="${APP}")
add_compile_definitions(APP_VERSION="${VERSION_NUMBER}")
add_compile_definitions(BUILD_NUM=${BUILD_NUMBER})

# Compile app source code file(s)
add_executable(${PROJECT_NAME}
    main.cpp
    led_thread.cpp
    gpio.cpp
    system.c
    stm32u5xx_hal_timebase_tim.c
)

# Link built libraries
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC
    Microvisor-HAL-STM32U5
    FreeRTOS
    ST_Code
)

# Optional informational and additional format generation
add_custom_command(OUTPUT EXTRA_FILES DEPENDS ${PROJECT_NAME}
    COMMAND mv "${PROJECT_NAME}" "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_SIZE} --format=berkeley "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_OBJDUMP} -h -S "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.list"
    COMMAND ${CMAKE_OBJCOPY} -O binary "${PROJECT_NAME}.elf" "${PROJECT_NAME}.bin"
)

# Prepare the additional files
add_custom_target(extras ALL DEPENDS EXTRA_FILES)
