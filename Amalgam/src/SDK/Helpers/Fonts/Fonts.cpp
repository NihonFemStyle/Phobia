#include "Fonts.h"

#include "Comfortaa/ComfortaaRegular.h"

#include "../../Definitions/Interfaces/IMatSystemSurface.h"
#include <ranges>

#pragma comment(lib, "Gdi32.lib")

void CFonts::Reload(float flDPI, bool bOutline)
{
	int iFlags = !bOutline ? FONTFLAG_ANTIALIAS : FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW;

	// register Comfortaa so GDI can resolve the family for the surface fonts
	static bool bRegistered = false;
	if (!bRegistered)
	{
		DWORD dwNum = 0;
		AddFontMemResourceEx((void*)ComfortaaRegular, DWORD(sizeof ComfortaaRegular), nullptr, &dwNum);
		bRegistered = true;
	}

	m_mFonts[FONT_ESP] = { "Comfortaa", int(12.f * flDPI), iFlags, 300 };
	m_mFonts[FONT_INDICATORS] = { "Comfortaa", int(13.f * flDPI), iFlags, 700 };
	m_mFonts[FONT_WATERMARK] = { Vars::Menu::Watermark::Font.Value.c_str(), int(float(Vars::Menu::Watermark::FontSize.Value) * flDPI), iFlags, 400 };

	for (auto& fFont : m_mFonts | std::views::values)
	{
		if (fFont.m_dwFont = I::MatSystemSurface->CreateFont())
			I::MatSystemSurface->SetFontGlyphSet(fFont.m_dwFont, fFont.m_szName, fFont.m_nTall, fFont.m_nWeight, 0, 0, fFont.m_nFlags);
	}
}

const Font_t& CFonts::GetFont(EFonts eFont)
{
	return m_mFonts[eFont];
}