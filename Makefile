ROOT=D:\Projects\msvc-toolchain-portable\msvc^\
MSVC_VERSION=14.42.34433
MSVC_HOST=Hostx64
MSVC_ARCH=x64
SDK_VERSION=10.0.26100.0
SDK_ARCH=x64
MSVC_ROOT=$(ROOT)VC\Tools\MSVC\$(MSVC_VERSION)
SDK_INCLUDE=$(ROOT)WindowsKits\10\Include\$(SDK_VERSION)
SDK_LIBS=$(ROOT)WindowsKits\10\Lib\$(SDK_VERSION)
PATH=$(MSVC_ROOT)\bin\$(MSVC_HOST)\$(MSVC_ARCH);$(ROOT)WindowsKits\10\bin\$(SDK_VERSION)\$(SDK_ARCH);$(ROOT)WindowsKits\10\bin\$(SDK_VERSION)\$(SDK_ARCH)\ucrt;$(PATH)

INCLUDE= /I$(SDK_INCLUDE)\um /I$(MSVC_ROOT)\include /I$(SDK_INCLUDE)\ucrt /I$(SDK_INCLUDE)\shared /I$(SDK_INCLUDE)\winrt /I$(SDK_INCLUDE)\cppwinrt
LIB_PATH=/LIBPATH:$(MSVC_ROOT)\lib\$(MSVC_ARCH) /LIBPATH:$(SDK_LIBS)\ucrt\$(SDK_ARCH) /LIBPATH:$(SDK_LIBS)\ucrt_enclave\$(SDK_ARCH) /LIBPATH:$(SDK_LIBS)\um\$(SDK_ARCH)

COMMON_COMPILER_FLAGS=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4701 -wd4702 -wd4996 -wd4805 -DANTIQUA_SLOW=1 -DANTIQUA_INTERNAL=1 -FC -Z7

BUILD_DIR=..\build

POINT_VSHADER_NAME=d3d11_vshader_point
POINT_PSHADER_NAME=d3d11_pshader_point
LINE_VSHADER_NAME=d3d11_vshader_line
LINE_PSHADER_NAME=d3d11_pshader_line
RECT_WORLD_VSHADER_NAME=d3d11_vshader_rect_world
RECT_WORLD_PSHADER_NAME=d3d11_pshader_rect_world
RECT_SCREEN_VSHADER_NAME=d3d11_vshader_rect_screen
RECT_SCREEN_PSHADER_NAME=d3d11_pshader_rect_screen
TILE_VSHADER_NAME=d3d11_vshader_tile
TILE_PSHADER_NAME=d3d11_pshader_tile
TEXTURE_DEBUG_VSHADER_NAME=d3d11_vshader_texture_debug
TEXTURE_DEBUG_PSHADER_NAME=d3d11_pshader_texture_debug
TEXT_VSHADER_NAME=d3d11_vshader_text
TEXT_PSHADER_NAME=d3d11_pshader_text

build: clean
	pushd $(BUILD_DIR)
	fxc.exe /nologo /T vs_5_0 /E vsPoint /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(POINT_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(POINT_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(POINT_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E psPoint /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(POINT_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(POINT_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(POINT_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T vs_5_0 /E vs /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(LINE_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(LINE_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(LINE_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E ps /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(LINE_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(LINE_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(LINE_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T vs_5_0 /E vsRectWorld /Od /WX /Zi /Zpc /Ges /Fo $(BUILD_DIR)\$(RECT_WORLD_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(RECT_WORLD_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(RECT_WORLD_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E psRectWorld /Od /WX /Zi /Zpc /Ges /Fo $(BUILD_DIR)\$(RECT_WORLD_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(RECT_WORLD_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(RECT_WORLD_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T vs_5_0 /E vsRectScreen /Od /WX /Zi /Zpc /Ges /Fo $(BUILD_DIR)\$(RECT_SCREEN_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(RECT_SCREEN_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(RECT_SCREEN_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E psRectScreen /Od /WX /Zi /Zpc /Ges /Fo $(BUILD_DIR)\$(RECT_SCREEN_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(RECT_SCREEN_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(RECT_SCREEN_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T vs_5_0 /E vsTile /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(TILE_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(TILE_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(TILE_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E psTile /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(TILE_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(TILE_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(TILE_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T vs_5_0 /E vsTextureDebug /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(TEXTURE_DEBUG_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(TEXTURE_DEBUG_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(TEXTURE_DEBUG_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E psTextureDebug /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(TEXTURE_DEBUG_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(TEXTURE_DEBUG_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(TEXTURE_DEBUG_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T vs_5_0 /E vsText /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(TEXT_VSHADER_NAME).cso /Fd $(BUILD_DIR)\$(TEXT_VSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(TEXT_VSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	fxc.exe /nologo /T ps_5_0 /E psText /Od /WX /Zpc /Zi /Ges /Fo $(BUILD_DIR)\$(TEXT_PSHADER_NAME).cso /Fd $(BUILD_DIR)\$(TEXT_PSHADER_NAME).pdb /Fc $(BUILD_DIR)\$(TEXT_PSHADER_NAME).asm ..\antiqua\shaders.hlsl >NUL
	cl $(COMMON_COMPILER_FLAGS) $(INCLUDE) win32.cpp -Fmwin32_main.map -Fo:win32_main /link $(LIB_PATH) /NODEFAULTLIB:LIBCMT -incremental:no -opt:ref user32.lib d3d11.lib dxgi.lib dxguid.lib d3d12.lib -PDB:win32_main.pdb
	if not exist "$(BUILD_DIR)" mkdir $(BUILD_DIR)
	if exist "*.dll" move *.dll $(BUILD_DIR) >NUL
	if exist "*.lib" move *.lib $(BUILD_DIR) >NUL
	if exist "*.exp" move *.exp $(BUILD_DIR) >NUL
	if exist "*.map" move *.map $(BUILD_DIR) >NUL
	if exist "*.obj" move *.obj $(BUILD_DIR) >NUL
	if exist "*.pdb" move *.pdb $(BUILD_DIR) >NUL
	if exist "*.log" move *.log $(BUILD_DIR) >NUL
	if exist "*.ini" move *.ini $(BUILD_DIR) >NUL
	if exist "*.exe" move *.exe $(BUILD_DIR) >NUL
	if exist "*.pdb" move *.pdb $(BUILD_DIR) >NUL
	if exist "*.cso" move *.cso $(BUILD_DIR) >NUL
	if exist "*.asm" move *.asm $(BUILD_DIR) >NUL
	echo compile success
clean:
	if exist "$(BUILD_DIR)\*.dll" del $(BUILD_DIR)\*.dll >NUL
	if exist "$(BUILD_DIR)\*.lib" del $(BUILD_DIR)\*.lib >NUL
	if exist "$(BUILD_DIR)\*.exp" del $(BUILD_DIR)\*.exp >NUL
	if exist "$(BUILD_DIR)\*.map" del $(BUILD_DIR)\*.map >NUL
	if exist "$(BUILD_DIR)\*.obj" del $(BUILD_DIR)\*.obj >NUL
	if exist "$(BUILD_DIR)\*.pdb" del $(BUILD_DIR)\*.pdb >NUL
	if exist "$(BUILD_DIR)\*.log" del $(BUILD_DIR)\*.log >NUL
	if exist "$(BUILD_DIR)\*.ini" del $(BUILD_DIR)\*.ini >NUL
	if exist "$(BUILD_DIR)\*.exe" del $(BUILD_DIR)\*.exe >NUL
	if exist "$(BUILD_DIR)\*.cso" del $(BUILD_DIR)\*.cso >NUL
	if exist "$(BUILD_DIR)\*.asm" del $(BUILD_DIR)\*.asm >NUL
	echo clean success
