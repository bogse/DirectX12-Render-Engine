# Define the hardware module library name and specify C++ language support.
project(DirectXHeaders VERSION 1.0 LANGUAGES CXX)

# Static Library Target.
add_library(DirectXHeaders INTERFACE)
add_library(DirectX::Headers ALIAS DirectXHeaders)

# Include Directories.
target_include_directories(DirectXHeaders BEFORE INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/directx>
)

# Populate a local string list containing the absolute system paths of the vendor headers.
set(DX_HEADER_FILES
    directx/d3d12.h
    directx/d3d12compatibility.h
    directx/d3d12compiler.h
    directx/d3d12sdklayers.h
    directx/d3d12shader.h
    directx/D3D12TokenizedProgramFormat.hpp
    directx/d3d12video.h
    directx/d3dcommon.h
    directx/d3dshadercacheregistration.h
    directx/d3dx12.h
    directx/d3dx12_barriers.h
    directx/d3dx12_check_feature_support.h
    directx/d3dx12_core.h
    directx/d3dx12_default.h
    directx/d3dx12_pipeline_state_stream.h
    directx/d3dx12_property_format_table.h
    directx/d3dx12_render_pass.h
    directx/d3dx12_resource_helpers.h
    directx/d3dx12_root_signature.h
    directx/d3dx12_state_object.h
    directx/DirectML.h
    directx/dxcore.h
    directx/dxcore_interface.h
    directx/dxgicommon.h
    directx/dxgiformat.h
)

set (DX_GUIDS_HEADER_FILES
	dxguids/dxguids.h
)

# Visual Studio Organization.
source_group("directx" FILES ${DX_HEADER_FILES})
source_group("dxguids" FILES ${DX_GUIDS_HEADER_FILES})

# Visual Studio Visibility.
set_property(TARGET DirectXHeaders PROPERTY
    SOURCES 
		${DX_HEADER_FILES}
		${DX_GUIDS_HEADER_FILES}
)