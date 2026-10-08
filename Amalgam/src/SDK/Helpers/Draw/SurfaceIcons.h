#pragma once
#include "Draw.h"
#include <unordered_map>
#include <cstdint>

// Rasterized Font Awesome glyphs drawn through ISurface (the classic HUD overlays).
// The glyph bitmaps are cut out of the ImGui font atlas once the menu fonts are
// built (see BuildSurfaceIcons in Render.cpp) and uploaded as ISurface textures,
// so the in-game chips can show the same icons as the menu chrome.
namespace SurfaceIcons
{
	inline std::unordered_map<uint32_t, int> s_mTextures = {};

	inline uint32_t Utf8Codepoint(const char* pStr)
	{
		const unsigned char* p = reinterpret_cast<const unsigned char*>(pStr);
		if (!p || !*p)
			return 0;
		if (p[0] < 0x80)
			return p[0];
		if ((p[0] & 0xE0) == 0xC0)
			return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
		if ((p[0] & 0xF0) == 0xE0)
			return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
		if ((p[0] & 0xF8) == 0xF0)
			return ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
		return 0;
	}

	// draw a registered icon glyph, tinted; returns false while unregistered
	inline bool Draw(const char* sIcon, int x, int y, int size, Color_t tColor)
	{
		auto it = s_mTextures.find(Utf8Codepoint(sIcon));
		if (it == s_mTextures.end())
			return false;
		I::MatSystemSurface->DrawSetColor(tColor);
		I::MatSystemSurface->DrawSetTexture(it->second);
		I::MatSystemSurface->DrawTexturedRect(x, y, x + size, y + size);
		return true;
	}
}