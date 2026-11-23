cmake_minimum_required(VERSION 4.3)

get_property(nvcloth_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(nvcloth_multi_config
    AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
    AND NOT DEFINED ENV{CMAKE_CONFIGURATION_TYPES}
)
    set(CMAKE_CONFIGURATION_TYPES Debug Checked Profile Release CACHE STRING "Build configurations")
endif()

project(NvClothSamples
    LANGUAGES CXX
)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "NvCloth samples require Windows with MSVC and Direct3D 11")
endif()

if(PROJECT_IS_TOP_LEVEL AND NOT nvcloth_multi_config AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()

option(STATIC_WINCRT "Use the static MSVC runtime" OFF)

if(STATIC_WINCRT)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(sample_crt staticcrt)
else()
    set(sample_crt dynamiccrt)
endif()

string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" sample_arch)
if(sample_arch STREQUAL "x64")
    set(prebuilt_arch x64)
    set(graphics_arch win64)
elseif(sample_arch STREQUAL "x86")
    set(prebuilt_arch Win32)
    set(graphics_arch win32)
else()
    message(FATAL_ERROR "The bundled sample graphics libraries support x86 and x64")
endif()

if(NOT TARGET NvCloth::NvCloth)
    add_subdirectory(.. NvCloth)
endif()

set(DXUTSDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/DXUT/9.15.2016.1" CACHE PATH "DXUT SDK directory")
set(DIRECTXTEXSDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/DirectXTex/10.5.2016.2" CACHE PATH "DirectXTex SDK directory")
set(IMGUISDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/1.49" CACHE PATH "ImGui source directory")

# Use the bundled v140 libraries with the matching architecture and CRT.
set(prebuilt_dir "${sample_crt}/vs2015/${prebuilt_arch}/$<IF:$<CONFIG:Debug>,Debug,Release>")
set(shadow "${CMAKE_CURRENT_SOURCE_DIR}/external/shadow_lib/GFSDK_ShadowLib_DX11.${graphics_arch}")
set(hbao "${CMAKE_CURRENT_SOURCE_DIR}/external/hbao/GFSDK_SSAO_D3D11.${graphics_arch}")

add_executable(base WIN32)

target_sources(base
    PRIVATE
        base/Main.cpp
        base/Sample.h
        base/core/Application.cpp
        base/core/Application.h
        base/core/DeviceManager.cpp
        base/core/DeviceManager.h
        base/core/SampleController.cpp
        base/core/SampleController.h
        base/core/SampleManager.cpp
        base/core/SampleManager.h
        base/renderer/ClothRenderMesh.cpp
        base/renderer/ClothRenderMesh.h
        base/renderer/ConvexRenderMesh.cpp
        base/renderer/ConvexRenderMesh.h
        base/renderer/CustomRenderMesh.cpp
        base/renderer/CustomRenderMesh.h
        base/renderer/DebugRenderBuffer.h
        base/renderer/Mesh.cpp
        base/renderer/Mesh.h
        base/renderer/PrimitiveRenderMesh.cpp
        base/renderer/PrimitiveRenderMesh.h
        base/renderer/PxRenderBuffer.h
        base/renderer/Renderable.cpp
        base/renderer/Renderable.h
        base/renderer/Renderer.cpp
        base/renderer/Renderer.h
        base/renderer/RendererHBAO.cpp
        base/renderer/RendererHBAO.h
        base/renderer/RendererShadow.cpp
        base/renderer/RendererShadow.h
        base/renderer/RenderMaterial.cpp
        base/renderer/RenderMaterial.h
        base/renderer/RenderUtils.h
        base/renderer/ResourceManager.cpp
        base/renderer/ResourceManager.h
        base/renderer/ShaderUtils.h
        base/renderer/SkinnedRenderMesh.cpp
        base/renderer/SkinnedRenderMesh.h
        base/scene/Scene.cpp
        base/scene/Scene.h
        base/scene/SceneController.cpp
        base/scene/SceneController.h
        base/scene/scenes/CapsuleScene.cpp
        base/scene/scenes/CapsuleScene.h
        base/scene/scenes/CCDScene.cpp
        base/scene/scenes/CCDScene.h
        base/scene/scenes/CCDScene2.cpp
        base/scene/scenes/CCDScene2.h
        base/scene/scenes/ConvexCollisionScene.cpp
        base/scene/scenes/ConvexCollisionScene.h
        base/scene/scenes/DistanceConstraintScene.cpp
        base/scene/scenes/DistanceConstraintScene.h
        base/scene/scenes/FreeFallScene.cpp
        base/scene/scenes/FreeFallScene.h
        base/scene/scenes/FrictionScene.cpp
        base/scene/scenes/FrictionScene.h
        base/scene/scenes/GeodesicScene.cpp
        base/scene/scenes/GeodesicScene.h
        base/scene/scenes/InterCollisionScene.cpp
        base/scene/scenes/InterCollisionScene.h
        base/scene/scenes/LocalGlobalScene.cpp
        base/scene/scenes/LocalGlobalScene.h
        base/scene/scenes/MultiSolverScene.cpp
        base/scene/scenes/MultiSolverScene.h
        base/scene/scenes/PlaneCollisionScene.cpp
        base/scene/scenes/PlaneCollisionScene.h
        base/scene/scenes/ScaledScene.cpp
        base/scene/scenes/ScaledScene.h
        base/scene/scenes/SelfCollisionScene.cpp
        base/scene/scenes/SelfCollisionScene.h
        base/scene/scenes/SimpleScene.cpp
        base/scene/scenes/SimpleScene.h
        base/scene/scenes/SphereScene.cpp
        base/scene/scenes/SphereScene.h
        base/scene/scenes/StiffnessPerConstraintScene.cpp
        base/scene/scenes/StiffnessPerConstraintScene.h
        base/scene/scenes/TeleportScene.cpp
        base/scene/scenes/TeleportScene.h
        base/scene/scenes/TetherScene.cpp
        base/scene/scenes/TetherScene.h
        base/scene/scenes/TimeStepScene.cpp
        base/scene/scenes/TimeStepScene.h
        base/scene/scenes/TriangleScene.cpp
        base/scene/scenes/TriangleScene.h
        base/scene/scenes/VirtualParticleScene.cpp
        base/scene/scenes/VirtualParticleScene.h
        base/scene/scenes/WindScene.cpp
        base/scene/scenes/WindScene.h
        base/task/PxCpuDispatcher.h
        base/task/PxGpuDispatcher.h
        base/task/PxGpuTask.h
        base/task/PxTask.h
        base/task/PxTaskDefine.h
        base/task/PxTaskManager.h
        base/ui/CommonUIController.cpp
        base/ui/CommonUIController.h
        base/ui/imgui_impl_dx11.cpp
        base/ui/imgui_impl_dx11.h
        base/utils/AnimatedModelUtilities.cpp
        base/utils/AnimatedModelUtilities.h
        base/utils/CallbackImplementations.cpp
        base/utils/CallbackImplementations.h
        base/utils/ClothMeshGenerator.cpp
        base/utils/ClothMeshGenerator.h
        base/utils/DataStream.cpp
        base/utils/DataStream.h
        base/utils/DebugLineRenderBuffer.cpp
        base/utils/DebugLineRenderBuffer.h
        base/utils/JobManager.cpp
        base/utils/JobManager.h
        base/utils/MeshGenerator.cpp
        base/utils/MeshGenerator.h
        base/utils/SampleProfiler.cpp
        base/utils/SampleProfiler.h
        base/utils/SampleTime.h
        base/utils/UIHelpers.h
        base/utils/Utils.cpp
        base/utils/Utils.h
)

get_target_property(sample_sources base SOURCES)
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/base"
    PREFIX "Source"
    FILES
        ${sample_sources}
)

set(imgui_sources
    "${IMGUISDK_PATH}/imconfig.h"
    "${IMGUISDK_PATH}/imgui_demo.cpp"
    "${IMGUISDK_PATH}/imgui_draw.cpp"
    "${IMGUISDK_PATH}/imgui_internal.h"
    "${IMGUISDK_PATH}/imgui.cpp"
    "${IMGUISDK_PATH}/imgui.h"
    "${IMGUISDK_PATH}/stb_rect_pack.h"
    "${IMGUISDK_PATH}/stb_textedit.h"
    "${IMGUISDK_PATH}/stb_truetype.h"
)
target_sources(base
    PRIVATE
        ${imgui_sources}
)
source_group("Source\\imgui"
    FILES
        ${imgui_sources}
)

target_include_directories(base
    PRIVATE
        base
        base/core
        base/renderer
        base/scene
        base/ui
        base/utils
        external/hbao
        external/shadow_lib
        "${DIRECTXTEXSDK_PATH}/include"
        "${DXUTSDK_PATH}/Core"
        "${DXUTSDK_PATH}/Optional"
        "${IMGUISDK_PATH}"
)

target_compile_definitions(base
    PRIVATE
        UNICODE _UNICODE WIN32
        $<$<STREQUAL:${sample_arch},x64>:WIN64>
        _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE
        _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH _ALLOW_RUNTIME_LIBRARY_MISMATCH
        __GFSDK_DX11__
        $<$<CONFIG:Debug>:NV_DEBUG=1>
        $<$<CONFIG:Checked>:NV_CHECKED=1>
        $<$<CONFIG:Profile>:NV_PROFILE=1>
        NVCLOTH_SAMPLE_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
        USE_CUDA=$<BOOL:${NV_CLOTH_ENABLE_CUDA}>
)
target_compile_options(base
    PRIVATE
        /GR-
        /MP
        /fp:fast
        /W3 /WX
        /wd4005 /wd4244
        "$<$<CONFIG:Checked,Profile>:/O2;/Z7>"
)
set_property(
    TARGET base
    PROPERTY MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:Embedded>"
)

target_link_options(base
    PRIVATE
        "$<$<CONFIG:Checked,Profile>:/INCREMENTAL:NO>"
)
target_link_libraries(base
    PRIVATE
        NvCloth::NvCloth
        d3dcompiler
        d3d11
        dxgi
        comctl32
        "${DIRECTXTEXSDK_PATH}/bin/${prebuilt_dir}/DirectXTex.lib"
        "${DXUTSDK_PATH}/Core/Bin/${prebuilt_dir}/DXUT.lib"
        "${DXUTSDK_PATH}/Optional/Bin/${prebuilt_dir}/DXUTOpt.lib"
        "${hbao}.lib"
        "${shadow}.lib"
)

if(NV_CLOTH_ENABLE_CUDA)
    find_package(CUDAToolkit 10 REQUIRED)
    target_link_libraries(base
        PRIVATE
            CUDA::cuda_driver
    )
    if(NOT STATIC_WINCRT)
        # Keep the sample's DLL CRT when cuda.lib requests the static CRT.
        target_link_options(base
            PRIVATE
                /NODEFAULTLIB:LIBCMT
        )
    endif()
endif()

add_custom_command(
    TARGET base
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${shadow}.dll"
        "${hbao}.dll"
        "$<TARGET_RUNTIME_DLLS:base>"
        "$<TARGET_FILE_DIR:base>"
    COMMAND_EXPAND_LISTS
    VERBATIM
)
