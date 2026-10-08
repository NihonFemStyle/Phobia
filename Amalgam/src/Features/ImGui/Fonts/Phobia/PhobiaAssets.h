#pragma once
#include <d3d9.h>
#include <cstddef>

// Phobia GUI assets + palette. bytearray.h (embedded TTF fonts + PNG textures) is
// only included from PhobiaAssets.cpp so the plain global arrays never collide
// across translation units.
namespace Phobia
{
	// embedded fonts (raw TTF bytes)
	extern const unsigned char* PoppinsFont;
	extern std::size_t PoppinsFontSize;
	extern const unsigned char* IconFont; // phobia material-style glyph font (0xE000 - 0xE226)
	extern std::size_t IconFontSize;

	// lazily-loaded D3D9 textures for the menu (background, logos, avatar)
	struct Textures
	{
		IDirect3DTexture9* bg = nullptr;        // bg_one  (1920x1080 backdrop)
		IDirect3DTexture9* logoWordmark = nullptr; // logo_one (120x25)
		IDirect3DTexture9* logoMark = nullptr;     // logo_two (85x98)
		IDirect3DTexture9* avatar = nullptr;       // user    (28x28)

		void load(IDirect3DDevice9* device);
		void release();
	};

	Textures& tex();
}