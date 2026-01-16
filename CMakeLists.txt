cmake_minimum_required(VERSION 3.20)
project(softphone C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT PJSIP_DIR)
    if(DEFINED ENV{PJSIP_DIR})
        set(PJSIP_DIR $ENV{PJSIP_DIR})
    endif()
endif()

if(NOT PJSIP_DIR)
    message(FATAL_ERROR "Set PJSIP_DIR to the PJSIP install root")
endif()

set(PJSIP_INCLUDE_DIR "${PJSIP_DIR}/include")
set(PJSIP_LIB_DIR "${PJSIP_DIR}/lib")

add_executable(softphone
    src/main.c
    src/softphone.c
    src/config.c
)

target_include_directories(softphone PRIVATE ${PJSIP_INCLUDE_DIR} include)
target_link_directories(softphone PRIVATE ${PJSIP_LIB_DIR})

if(MSVC)
    target_compile_definitions(softphone PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(softphone
    pjsua-lib
    pjsua
    pjsip-ua
    pjsip-simple
    pjsip
    pjmedia-codec
    pjmedia
    pjmedia-audiodev
    pjmedia-videodev
    pjnath
    pjlib-util
    pjlib
)

if(WIN32)
    target_link_libraries(softphone
        ws2_32
        winmm
        ole32
        shell32
        user32
        advapi32
        uuid
    )
endif()
