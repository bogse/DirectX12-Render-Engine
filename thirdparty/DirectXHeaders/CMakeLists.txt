# Latest version of CMake when this file was created.
cmake_minimum_required(VERSION 4.3.2)

# Define the hardware module library name and specify C++ language support.
project(DirectXHeaders VERSION 1.0 LANGUAGES CXX)

# Static Library Target.
add_library(DirectXHeaders INTERFACE)
add_library(DirectX::Headers ALIAS DirectXHeaders)

# Include Directories.
target_include_directories(DirectXHeaders BEFORE INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

# Populate a local string list containing the absolute system paths of the vendor headers.
set(DX_HEADER_FILES
    inc/d3d12.h
    inc/d3d12compatibility.h
    inc/d3d12compiler.h
    inc/d3d12sdklayers.h
    inc/d3d12shader.h
    inc/D3D12TokenizedProgramFormat.hpp
    inc/d3d12video.h
    inc/d3dcommon.h
    inc/d3dshadercacheregistration.h
    inc/d3dx12.h
    inc/d3dx12_barriers.h
    inc/d3dx12_check_feature_support.h
    inc/d3dx12_core.h
    inc/d3dx12_default.h
    inc/d3dx12_pipeline_state_stream.h
    inc/d3dx12_property_format_table.h
    inc/d3dx12_render_pass.h
    inc/d3dx12_resource_helpers.h
    inc/d3dx12_root_signature.h
    inc/d3dx12_state_object.h
    inc/DirectML.h
    inc/dxcore.h
    inc/dxcore_interface.h
    inc/dxgicommon.h
    inc/dxgiformat.h
)

# Visual Studio Organization.
source_group("inc" FILES ${DX_HEADER_FILES})

# Visual Studio Visibility.
set_property(TARGET DirectXHeaders PROPERTY
    SOURCES ${DX_HEADER_FILES}
)