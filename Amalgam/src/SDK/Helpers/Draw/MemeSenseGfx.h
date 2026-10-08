#pragma once
#include "../../Definitions/Types.h"
#include "Palette.h"

// Phobia palette as Color_t, for the ISurface-drawn HUD overlays (indicator
// panels, watermark, etc.) so they match the ImGui menu chrome exactly.
// The ImGui-side palette lives in MemeSense:: (Menu/Components.h) with the
// same values. Accent follows the user theme color.
namespace MemeSenseGfx
{
	inline Color_t Accent() { return TUI::Palette.Accent; }
	inline Color_t PageBg() { return Color_t(26, 26, 26, 255); }
	inline Color_t SidebarBg() { return Color_t(32, 32, 32, 255); }
	inline Color_t ActiveBg() { return Color_t(41, 41, 41, 255); }
	inline Color_t SidebarHover() { return Color_t(50, 50, 50, 255); }
	inline Color_t SidebarHeader() { return Color_t(105, 105, 105, 255); }
	inline Color_t Line() { return Color_t(50, 50, 50, 255); }
	inline Color_t WidgetBg() { return Color_t(41, 41, 41, 255); }
	inline Color_t WidgetHover() { return Color_t(55, 55, 55, 255); }
	inline Color_t CheckboxOff() { return Color_t(41, 41, 41, 255); }
	inline Color_t SliderFill() { return Color_t(105, 105, 105, 255); }
	inline Color_t TextDim() { return Color_t(105, 105, 105, 255); }
	inline Color_t White() { return Color_t(255, 255, 255, 255); }
	inline Color_t ComboText() { return Color_t(105, 105, 105, 255); }
	inline Color_t ComboTextHover() { return Color_t(200, 200, 200, 255); }
}