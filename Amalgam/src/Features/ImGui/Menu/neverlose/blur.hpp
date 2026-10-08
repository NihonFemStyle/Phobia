#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <math.h>

#include <d3d9.h>

#pragma comment ( lib, "d3d9.lib" )

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

namespace blur {

	inline IDirect3DDevice9* device;
}

extern void draw_blur( ImDrawList* drawList );