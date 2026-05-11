# Latest version of CMake when this file was created.
cmake_minimum_required(VERSION 4.3.2)

# Define the library project framework name and specify C++ language support.
project(DirectXTex LANGUAGES CXX)

# C++ standard configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC-specific compile settings.
if(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# Include directory containing DirectXTex headers.
set(DIRECTXTEX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex)

# Shaders directory definition
set(DIRECTXTEX_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/Shaders)

# Header files (Public API)
set(HEADERS_DIRECTXTEX
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTex.h
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTex.inl
)

set(HEADERS_DDS
    ${CMAKE_CURRENT_SOURCE_DIR}/DDSTextureLoader/DDSTextureLoader12.h
)

set(HEADERS_WIC
    ${CMAKE_CURRENT_SOURCE_DIR}/WICTextureLoader/WICTextureLoader12.h
)

set(DIRECTXTEX_PUBLIC_HEADERS
    ${HEADERS_DIRECTXTEX}
    ${HEADERS_DDS}
    ${HEADERS_WIC}
)

# Implementation-only internal headers (Private API)
set(DIRECTXTEX_PRIVATE_HEADERS
    ${DIRECTXTEX_INCLUDE_DIR}/BC.h
    ${DIRECTXTEX_INCLUDE_DIR}/BCDirectCompute.h
    ${DIRECTXTEX_INCLUDE_DIR}/DDS.h
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexP.h
    ${DIRECTXTEX_INCLUDE_DIR}/filters.h
    ${DIRECTXTEX_INCLUDE_DIR}/scoped.h
)

# Shader and build script files.
set(DIRECTXTEX_SHADERS
    ${DIRECTXTEX_SHADERS_DIR}/BC6HEncode.hlsl
    ${DIRECTXTEX_SHADERS_DIR}/BC7Encode.hlsl
    ${DIRECTXTEX_SHADERS_DIR}/CompileShaders.cmd
)

# Source files compiled into the library.
set(SOURCES_DIRECTXTEX
    # Block Compression & DirectCompute.
    ${DIRECTXTEX_INCLUDE_DIR}/BC.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/BC4BC5.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/BC6HBC7.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/BCDirectCompute.cpp

    # Core Library Operations.
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexCompress.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexCompressGPU.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexConvert.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexD3D12.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexDDS.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexFlipRotate.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexHDR.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexImage.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexMipmaps.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexMisc.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexNormalMaps.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexPMAlpha.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexResize.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexTGA.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexUtil.cpp
    ${DIRECTXTEX_INCLUDE_DIR}/DirectXTexWIC.cpp
)

set(SOURCES_DDS
    ${CMAKE_CURRENT_SOURCE_DIR}/DDSTextureLoader/DDSTextureLoader12.cpp
)

set(SOURCES_WIC
    ${CMAKE_CURRENT_SOURCE_DIR}/WICTextureLoader/WICTextureLoader12.cpp
)

set(DIRECTXTEX_SOURCES
    ${SOURCES_DIRECTXTEX}
    ${SOURCES_DDS}
    ${SOURCES_WIC}
)

# --- EXPLICITLY DISABLE MSBUILD HLSL GENERATOR FOR THESE FILES ---
# This prevents compilation conflicts in Visual Studio because the batch file handles it.
set_source_files_properties(${DIRECTXTEX_SHADERS_DIR}/BC6HEncode.hlsl PROPERTIES VS_TOOL_OVERRIDE "None")
set_source_files_properties(${DIRECTXTEX_SHADERS_DIR}/BC7Encode.hlsl PROPERTIES VS_TOOL_OVERRIDE "None")

# Build DirectXTex as a static library (.lib).
add_library(DirectXTexDX12 STATIC 
	${DIRECTXTEX_SOURCES}
	${DIRECTXTEX_PUBLIC_HEADERS}
	${DIRECTXTEX_PRIVATE_HEADERS}
	${DIRECTXTEX_SHADERS})

# --- AUTOMATED SHADER COMPILATION CUSTOM COMMAND ---
# This invokes the .cmd script inside its directory before compiling the project binaries.
add_custom_command(
    TARGET DirectXTexDX12 PRE_BUILD
    COMMAND call CompileShaders.cmd
    WORKING_DIRECTORY ${DIRECTXTEX_SHADERS_DIR}
    COMMENT "Compiling DirectXTex DirectCompute Shaders via CompileShaders.cmd..."
    VERBATIM
)

# Visual Studio Organization.
source_group("inc\\DirectXTex" FILES ${HEADERS_DIRECTXTEX})
source_group("inc\\DDS" FILES ${HEADERS_DDS})
source_group("inc\\WIC" FILES ${HEADERS_WIC})

source_group("src\\DirectXTex" FILES ${SOURCES_DIRECTXTEX} ${DIRECTXTEX_PRIVATE_HEADERS})
source_group("src\\DDS" FILES ${SOURCES_DDS})
source_group("src\\WIC" FILES ${SOURCES_WIC})

source_group("src\\DirectXTex\\shaders" FILES ${DIRECTXTEX_SHADERS})

# Expose include path to anything linking this library.
target_include_directories(DirectXTexDX12
    PUBLIC
        ${DIRECTXTEX_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/DDSTextureLoader
        ${CMAKE_CURRENT_SOURCE_DIR}/WICTextureLoader
        ${CMAKE_CURRENT_SOURCE_DIR}/../DirectXHeaders
	PRIVATE
	    ${DIRECTXTEX_SHADERS_DIR}/Compiled
)

# Required Windows compile definitions.
target_compile_definitions(DirectXTexDX12
	PUBLIC
		USING_DIRECTX_HEADERS
    PRIVATE
        _UNICODE
        UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

# Required system libraries for DirectXTex + DirectX 12.
target_link_libraries(DirectXTexDX12
    PUBLIC
        d3d12
        dxgi
        dxguid
        windowscodecs
		DirectX::Headers
    PRIVATE
        shlwapi
        ole32
)