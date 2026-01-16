cmake_minimum_required(VERSION 3.22.1)
project(newsoftphonea)

set(PJPROJECT_DIR "${CMAKE_SOURCE_DIR}/pjproject")
set(PJSIP_LINK_LIBS "")

option(PJSIP_USE_PJPROJECT_CMAKE "Build pjproject via its CMake" OFF)
if (NOT ANDROID)
    set(PJSIP_USE_PJPROJECT_CMAKE ON CACHE BOOL "Build pjproject via its CMake" FORCE)
endif()

add_library(
    pjsip_jni
    SHARED
    pjsip_engine.cpp
    pjsip_jni.cpp
)

target_include_directories(pjsip_jni PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjsip/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjlib/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjmedia/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjnath/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjlib-util/include")

if (PJSIP_USE_PJPROJECT_CMAKE
    AND EXISTS "${PJPROJECT_DIR}/CMakeLists.txt"
    AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
    add_subdirectory("${PJPROJECT_DIR}" pjproject_build)
    list(APPEND PJSIP_LINK_LIBS
        pjlib
        pjlib-util
        pjnath
        pjmedia
        pjmedia-audiodev
        pjmedia-codec
        pjmedia-videodev
        pjsip
        pjsip-simple
        pjsip-ua
        pjsua-lib
        pjsua2
    )
else()
    set(PJSIP_PREBUILT_ROOT "${PJPROJECT_DIR}/android/${ANDROID_ABI}"
        CACHE PATH "Prebuilt pjproject output root for this ABI")
    set(PJSIP_LIB_DIR "${PJSIP_PREBUILT_ROOT}/lib")

    if (NOT EXISTS "${PJSIP_LIB_DIR}")
        message(FATAL_ERROR
            "PJSIP prebuilt libs not found at ${PJSIP_LIB_DIR}. "
            "Build with configure-android and copy libs into that path.")
    endif()

    set(PJSIP_BASE_LIBS
        pj
        pjlib-util
        pjnath
        pjmedia
        pjmedia-audiodev
        pjmedia-codec
        pjmedia-videodev
        pjsip
        pjsip-simple
        pjsip-ua
        pjsua-lib
        pjsua2
    )

    foreach(lib IN LISTS PJSIP_BASE_LIBS)
        add_library(${lib} STATIC IMPORTED)
        set(lib_file "${lib}")
        if (lib STREQUAL "pjsua-lib")
            set(lib_file "pjsua")
        endif()
        set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION "${PJSIP_LIB_DIR}/lib${lib_file}.a"
        )
        list(APPEND PJSIP_LINK_LIBS ${lib})
    endforeach()

    set(PJSIP_OPTIONAL_LIBS
        srtp
        gsmcodec
        speex
        speexdsp
        ilbccodec
        resample
        yuv
        webrtc
        webrtc-aec3
        g7221codec
        opus
    )

    foreach(lib IN LISTS PJSIP_OPTIONAL_LIBS)
        if (EXISTS "${PJSIP_LIB_DIR}/lib${lib}.a")
            add_library(${lib} STATIC IMPORTED)
            set_target_properties(${lib} PROPERTIES
                IMPORTED_LOCATION "${PJSIP_LIB_DIR}/lib${lib}.a"
            )
            list(APPEND PJSIP_LINK_LIBS ${lib})
        endif()
    endforeach()
endif()

target_link_libraries(pjsip_jni
    ${PJSIP_LINK_LIBS}
    android
    log
)
