cmake_minimum_required(VERSION 3.22.1)
project(newsoftphonea)

set(PJPROJECT_DIR "${CMAKE_SOURCE_DIR}/pjproject")

add_library(
    pjsip_jni
    SHARED
    pjsip_engine.cpp
    pjsip_jni.cpp
)

target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjsip/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjlib/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjmedia/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjnath/include")
target_include_directories(pjsip_jni PRIVATE "${PJPROJECT_DIR}/pjlib-util/include")

if (EXISTS "${PJPROJECT_DIR}/CMakeLists.txt")
    add_subdirectory("${PJPROJECT_DIR}" pjproject_build)
    target_link_libraries(pjsip_jni
        pjsip
        pjsua2
        pjsip-ua
        pjmedia
        pjmedia-audiodev
        pjmedia-codec
        pjmedia-videodev
        pjnath
        pjlib
        pjlib-util
    )
else()
    message(WARNING "pjproject not found in app/src/main/cpp/pjproject")
endif()

target_link_libraries(pjsip_jni
    android
    log
)
