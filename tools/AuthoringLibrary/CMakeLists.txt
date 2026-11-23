cmake_minimum_required(VERSION 4.3)

get_property(nvcloth_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(nvcloth_multi_config
    AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
    AND NOT DEFINED ENV{CMAKE_CONFIGURATION_TYPES}
)
    set(CMAKE_CONFIGURATION_TYPES Debug Checked Profile Release CACHE STRING "Build configurations")
endif()

project(NvClothAuthoringLibrary
    LANGUAGES CXX
)

if(PROJECT_IS_TOP_LEVEL AND NOT nvcloth_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()

if(MSVC AND STATIC_WINCRT)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(NOT TARGET NvCloth::NvCloth)
    add_subdirectory(../.. NvCloth)
endif()

add_library(NvClothAuthoringLibrary STATIC)

target_sources(NvClothAuthoringLibrary
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            include
        FILES
            include/NvClothAuthoringLibrary/CollisionVisualization.h
            include/NvClothAuthoringLibrary/DebugVisualization.h
            include/NvClothAuthoringLibrary/Parameters.h

    PRIVATE
        src/CollisionVisualization.cpp
        src/DebugVisualization.cpp
        src/Parameters.cpp
)
target_link_libraries(NvClothAuthoringLibrary
    PUBLIC
        NvCloth::NvCloth
)

if(MSVC)
    target_compile_definitions(NvClothAuthoringLibrary
        PRIVATE
            _CRT_SECURE_NO_DEPRECATE
    )
    target_compile_options(NvClothAuthoringLibrary
        PRIVATE
            /GR-
            /MP
            /fp:fast
            /W3 /WX
            /wd4005 /wd4244
            "$<$<CONFIG:Checked,Profile>:/O2;/Z7>"
    )
endif()
