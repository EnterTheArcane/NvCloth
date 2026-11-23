cmake_minimum_required(VERSION 4.3)

project(NvClothInstalledConsumer
    LANGUAGES CXX
)

find_package(NvCloth 1.1.6 EXACT CONFIG REQUIRED)

enable_testing()

foreach(order IN ITEMS nvcloth physx)
    set(consumer "nvcloth-consumer-${order}-first")
    add_executable(${consumer} main.cpp)
    target_link_libraries(${consumer}
        PRIVATE
            NvCloth::NvCloth
            PhysX::PhysX
    )

    if(order STREQUAL "physx")
        target_compile_definitions(${consumer}
            PRIVATE
                PHYSX_FIRST
        )
    endif()

    if(MSVC)
        target_compile_options(${consumer}
            PRIVATE
                /fp:precise
        )
    else()
        target_compile_options(${consumer}
            PRIVATE
                -fno-fast-math
        )
    endif()

    if(EMSCRIPTEN)
        target_link_options(${consumer}
            PRIVATE
                -pthread
                -sEXIT_RUNTIME=1
                -sENVIRONMENT=node
        )
    endif()

    add_test(
        NAME ${consumer}
        COMMAND ${consumer}
    )
endforeach()

get_target_property(nvcloth_type NvCloth::NvCloth TYPE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND nvcloth_type STREQUAL "SHARED_LIBRARY")
    add_executable(nvcloth-consumer-standalone standalone.cpp)
    target_link_libraries(nvcloth-consumer-standalone
        PRIVATE
            NvCloth::NvCloth
    )
    target_link_options(nvcloth-consumer-standalone
        PRIVATE
            "LINKER:--as-needed"
    )

    add_test(
        NAME nvcloth-consumer-standalone
        COMMAND nvcloth-consumer-standalone
    )
endif()
