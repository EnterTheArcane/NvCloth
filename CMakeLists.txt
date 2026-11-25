cmake_minimum_required(VERSION 4.3)

get_property(nvcloth_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(nvcloth_multi_config
    AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
    AND NOT DEFINED ENV{CMAKE_CONFIGURATION_TYPES}
)
    set(CMAKE_CONFIGURATION_TYPES Debug Checked Profile Release CACHE STRING "Build configurations")
endif()

project(NvCloth
    VERSION 1.1.6
    LANGUAGES CXX
)

include(GNUInstallDirs)

if(PROJECT_IS_TOP_LEVEL AND NOT nvcloth_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()

option(BUILD_SHARED_LIBS "Build NvCloth as a shared library" OFF)
option(NV_CLOTH_ENABLE_CUDA "Build the CUDA backend on supported platforms" OFF)
option(NV_CLOTH_ENABLE_DX11 "Build the Direct3D 11 backend on Windows" OFF)

find_package(PhysX 5...<6 CONFIG REQUIRED)

get_target_property(physx_foundation_type PhysX::PhysXFoundation TYPE)
if(BUILD_SHARED_LIBS AND physx_foundation_type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR "Shared NvCloth requires shared PhysXFoundation so both SDKs use one foundation")
endif()

add_library(NvCloth)
add_library(NvCloth::NvCloth ALIAS NvCloth)

target_sources(NvCloth
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS
            include
        FILES
            include/NvCloth/Allocator.h
            include/NvCloth/Callbacks.h
            include/NvCloth/Cloth.h
            include/NvCloth/DxContextManagerCallback.h
            include/NvCloth/Fabric.h
            include/NvCloth/Factory.h
            include/NvCloth/PhaseConfig.h
            include/NvCloth/ps/Ps.h
            include/NvCloth/ps/PsAlignedMalloc.h
            include/NvCloth/ps/PsAllocator.h
            include/NvCloth/ps/PsArray.h
            include/NvCloth/ps/PsAtomic.h
            include/NvCloth/ps/PsBasicTemplates.h
            include/NvCloth/ps/PsBitUtils.h
            include/NvCloth/ps/PsHash.h
            include/NvCloth/ps/PsHashInternals.h
            include/NvCloth/ps/PsHashMap.h
            include/NvCloth/ps/PsIntrinsics.h
            include/NvCloth/ps/PsMathUtils.h
            include/NvCloth/ps/PsUserAllocated.h
            include/NvCloth/ps/unix/PsUnixIntrinsics.h
            include/NvCloth/ps/windows/PsWindowsIntrinsics.h
            include/NvCloth/Range.h
            include/NvCloth/Solver.h
            include/NvClothExt/ClothFabricCooker.h
            include/NvClothExt/ClothMeshDesc.h
            include/NvClothExt/ClothMeshQuadifier.h
            include/NvClothExt/ClothTetherCooker.h

    PRIVATE
        src/Allocator.cpp
        src/Callbacks.cpp
        src/ext/ClothFabricCooker.cpp
        src/ext/ClothGeodesicTetherCooker.cpp
        src/ext/ClothMeshQuadifier.cpp
        src/ext/ClothSimpleTetherCooker.cpp
        src/Factory.cpp
        src/PhaseConfig.cpp
        src/ps/PsAllocator.cpp
        src/SwCloth.cpp
        src/SwClothData.cpp
        src/SwCollision.cpp
        src/SwFabric.cpp
        src/SwFactory.cpp
        src/SwInterCollision.cpp
        src/SwSelfCollision.cpp
        src/SwSolver.cpp
        src/SwSolverKernel.cpp
        src/TripletScheduler.cpp
)

target_compile_features(NvCloth
    PUBLIC
        cxx_std_14
)
target_include_directories(NvCloth
    PUBLIC
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        src
)
target_compile_definitions(NvCloth
    PRIVATE
        NV_CLOTH_ENABLE_CUDA=$<BOOL:${NV_CLOTH_ENABLE_CUDA}>
        NV_CLOTH_ENABLE_DX11=$<BOOL:${NV_CLOTH_ENABLE_DX11}>
    PUBLIC
        "$<$<CONFIG:Debug>:_DEBUG;PX_DEBUG=1>"
        $<$<CONFIG:Debug,Checked>:PX_CHECKED=1>
        $<$<CONFIG:Checked,Profile,Release>:NDEBUG>
        $<$<CONFIG:Profile>:PX_PROFILE=1>
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(NvCloth
        PRIVATE
            NV_CLOTH_EXPORTS
    )
    if(EMSCRIPTEN)
        target_link_options(NvCloth PRIVATE -pthread -sSHARED_MEMORY=1)
    elseif(NOT DEFINED CMAKE_INSTALL_RPATH)
        if(APPLE)
            set_property(TARGET NvCloth PROPERTY INSTALL_RPATH "@loader_path")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
            set_property(TARGET NvCloth PROPERTY INSTALL_RPATH "$ORIGIN")
        endif()
    endif()
else()
    target_compile_definitions(NvCloth
        PUBLIC
            NV_CLOTH_STATIC
    )
endif()

set_target_properties(NvCloth
    PROPERTIES
        CXX_MODULE_STD OFF
        CXX_SCAN_FOR_MODULES OFF
        POSITION_INDEPENDENT_CODE ON
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND BUILD_SHARED_LIBS)
    # Keep the consumer's direct dependency so its RUNPATH can locate PhysX.
    target_link_libraries(NvCloth
        PUBLIC
            "-Wl,--push-state,--no-as-needed"
            PhysX::PhysXFoundation
            "-Wl,--pop-state"
    )
else()
    target_link_libraries(NvCloth
        PUBLIC
            PhysX::PhysXFoundation
    )
endif()

# Platform-specific CPU sources and options.
if(WIN32)
    target_sources(NvCloth
        PRIVATE
            src/ps/windows/PsWindowsAtomic.cpp
    )

    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
        set(nvcloth_target_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
    elseif(CMAKE_GENERATOR_PLATFORM)
        set(nvcloth_target_arch "${CMAKE_GENERATOR_PLATFORM}")
    else()
        set(nvcloth_target_arch "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    string(TOLOWER "${nvcloth_target_arch}" nvcloth_target_arch)

    if(NOT nvcloth_target_arch MATCHES "^(arm64|aarch64)(,|$)")
        target_sources(NvCloth
            PRIVATE
                src/avx/SwSolveConstraints.cpp
        )
        set_source_files_properties(src/avx/SwSolveConstraints.cpp
            PROPERTIES
                COMPILE_OPTIONS "/arch:AVX"
        )
    endif()

    if(STATIC_WINCRT)
        set_property(
            TARGET NvCloth
            PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
    endif()

    target_compile_definitions(NvCloth
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
    )
    # Embed debug information so static SDK consumers do not need the compiler PDB.
    set_property(
        TARGET NvCloth
        PROPERTY MSVC_DEBUG_INFORMATION_FORMAT ""
    )
    target_compile_options(NvCloth
        PRIVATE
            /GR-
            /fp:fast
            /Z7
            "$<$<CONFIG:Checked,Profile>:/O2;/Zo>"
    )

    if(BUILD_SHARED_LIBS)
        target_link_options(NvCloth
            PRIVATE
                /DEBUG
        )
    endif()
else()
    target_sources(NvCloth
        PRIVATE
            src/ps/unix/PsUnixAtomic.cpp
    )
    target_compile_options(NvCloth
        PRIVATE
            -fno-rtti
            -fno-exceptions
            "$<$<CONFIG:Checked,Profile>:-O3;-g>"
    )

    if(APPLE OR (ANDROID AND CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a"))
        target_compile_options(NvCloth
            PRIVATE
                -ffast-math
        )
    endif()
endif()

# Optional GPU backends.
if(NV_CLOTH_ENABLE_DX11)
    if(NOT WIN32)
        message(FATAL_ERROR "NvCloth's Direct3D 11 backend requires Windows")
    endif()
    target_sources(NvCloth
        PRIVATE
            src/dx/DxCloth.cpp
            src/dx/DxClothClone.cpp
            src/dx/DxClothData.cpp
            src/dx/DxContextLock.cpp
            src/dx/DxFabric.cpp
            src/dx/DxFactory.cpp
            src/dx/DxSolver.cpp
    )
endif()

set(NVCLOTH_EXPORT_CUDA_DEPENDENCY OFF)
if(NV_CLOTH_ENABLE_CUDA)
    if(NOT WIN32 AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR EMSCRIPTEN))
        message(FATAL_ERROR "NvCloth's CUDA backend requires Windows or native Linux")
    endif()

    set(NVCLOTH_CUDA_ARCHITECTURE "75" CACHE STRING "CUDA virtual architecture for the NvCloth kernel")
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES AND "$ENV{CUDAARCHS}" STREQUAL "")
        set(CMAKE_CUDA_ARCHITECTURES "${NVCLOTH_CUDA_ARCHITECTURE}-virtual")
    endif()

    enable_language(CUDA)
    find_package(CUDAToolkit 11 REQUIRED)

    add_library(NvClothCudaKernel OBJECT src/cuda/CuSolverKernel.cu)
    set_target_properties(NvClothCudaKernel
        PROPERTIES
            CUDA_STANDARD 14
            CUDA_STANDARD_REQUIRED ON
            CUDA_VISIBILITY_PRESET hidden
            POSITION_INDEPENDENT_CODE ON
    )
    target_include_directories(NvClothCudaKernel
        PRIVATE
            include
            include/NvCloth
            "$<TARGET_PROPERTY:PhysX::PhysXFoundation,INTERFACE_INCLUDE_DIRECTORIES>"
    )
    target_compile_definitions(NvClothCudaKernel
        PRIVATE
            NDEBUG
    )
    target_compile_options(NvClothCudaKernel
        PRIVATE
            --use_fast_math
    )
    if(MSVC AND STATIC_WINCRT)
        set_property(
            TARGET NvClothCudaKernel
            PROPERTY MSVC_RUNTIME_LIBRARY "$<TARGET_PROPERTY:NvCloth,MSVC_RUNTIME_LIBRARY>"
        )
    endif()

    target_sources(NvCloth
        PRIVATE
            "$<TARGET_OBJECTS:NvClothCudaKernel>"
            src/cuda/CuCloth.cpp
            src/cuda/CuClothClone.cpp
            src/cuda/CuClothData.cpp
            src/cuda/CuContextLock.cpp
            src/cuda/CuFabric.cpp
            src/cuda/CuFactory.cpp
            src/cuda/CuSolver.cpp
    )
    set_target_properties(NvCloth
        PROPERTIES
            CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}"
            CUDA_RUNTIME_LIBRARY None
    )
    target_link_libraries(NvCloth
        PRIVATE
            CUDA::cuda_driver
            CUDA::cudart_static
    )

    if(WIN32 AND BUILD_SHARED_LIBS)
        target_link_libraries(NvCloth
            PRIVATE
                delayimp
        )
        target_link_options(NvCloth
            PRIVATE
                /DELAYLOAD:nvcuda.dll
        )
    endif()

    if(NOT BUILD_SHARED_LIBS)
        set(NVCLOTH_EXPORT_CUDA_DEPENDENCY ON)
    endif()
endif()

# SDK installation and package exports.
include(CMakePackageConfigHelpers)

set(nvcloth_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/NvCloth")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NvClothConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NvClothConfig.cmake"
    INSTALL_DESTINATION "${nvcloth_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NvClothConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

set(nvcloth_install_config_dir "$<$<BOOL:${nvcloth_multi_config}>:$<LOWER_CASE:$<CONFIG>>>")

install(
    TARGETS NvCloth
    EXPORT NvClothTargets
    RUNTIME
        DESTINATION "${CMAKE_INSTALL_BINDIR}/${nvcloth_install_config_dir}"
    LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/${nvcloth_install_config_dir}"
    ARCHIVE
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/${nvcloth_install_config_dir}"
    FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
if(WIN32 AND BUILD_SHARED_LIBS)
    install(
        FILES "$<TARGET_PDB_FILE:NvCloth>"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/${nvcloth_install_config_dir}"
        OPTIONAL
    )
endif()

install(
    FILES LICENSE.txt
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/NvCloth"
)

install(
    EXPORT NvClothTargets
    NAMESPACE NvCloth::
    DESTINATION "${nvcloth_package_dir}"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/NvClothConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/NvClothConfigVersion.cmake"
    DESTINATION "${nvcloth_package_dir}"
)
