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
- Texture support

## Requirements
- Windows 10/11
- DirectX 12 compatible GPU
- CMake (v4.3.2+ recommended)
- Visual Studio 2022 recommended (can be changed from GenerateSolution.bat)
- Windows SDK 10.0+
- MSVC v143 build tools

## Generating and Building the Project
Ensure you have CMake and Visual Studio installed. Run the following commands from the project root directory using your terminal (Command Prompt, PowerShell, or VS Developer Command Prompt):

```bash
# 1. Configure the project and generate the build files
cmake -B build

# 2. Compile the engine (Release mode)
cmake --build build --config Release
```

*Note: To compile the project for debugging, change `--config Release` to `--config Debug`.*