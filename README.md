# DirectX12-Render-Engine

This is a DirectX 12 rendering engine built in C++ for learning and experimentation with low-level graphics programming.

## Features: 
- Win32 window creation and message loop
- DirectX 12 initialization
- Basic rendering pipeline setup
- Shader support (Vertex and Pixel shaders)
- Command list abstraction
- Descriptor heap management
- Resource management and state tracking
- Upload buffer system
- Dear ImGui integration for real-time debugging and tooling
- Mesh class implementation for geometry handling and rendering
- Texture loading support:
	- DDS via DDSTextureLoader12
	- PNG / JPG / BMP and other WIC-supported formats via WICTextureLoader12
	- HDR textures and advanced processing via DirectXTex

## Requirements
- Windows 10/11
- DirectX 12 compatible GPU
- CMake (v4.3.2+ recommended)
- Visual Studio 2022 recommended (can be changed from GenerateSolution.bat)
- Windows SDK 10.0+
- MSVC v143 build tools

## Generating the Project
Run the provided batch script from the project root: GenerateSolution.bat