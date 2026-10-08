#pragma once
#include "Draw.h"
#include "MemeSenseGfx.h"
#include "SurfaceIcons.h"
#include "OverlayFx.h"
#include <algorithm>
#include <string>
#include <vector>

// Maps a 0..1 severity (0 = good, 1 = bad) onto the green -> yellow -> red indicator ramp.
inline Color_t SeverityColor(float flSeverity)
{
	flSeverity = std::clamp(flSeverity, 0.f, 1.f);
	const auto& tGood = Vars::Colors::IndicatorTextGood.Value;
	const auto& tMid = Vars::Colors::IndicatorTextMid.Value;
	const auto& tBad = Vars::Colors::IndicatorTextBad.Value;
	if (flSeverity < 0.5f)
		return tGood.Lerp(tMid, flSeverity * 2.f, LerpEnum::NoAlpha);
	return tMid.Lerp(tBad, (flSeverity - 0.5f) * 2.f, LerpEnum::NoAlpha);
}

// Themed overlay "chip" used by the classic ISurface HUD surfaces (indicators,
// watermark etc.). Auto-sizes to its text rows and draws the MemeSense-style
// rounded panel, matching the menu's chrome (dark fill, subtle border, accent title).
struct IndicatorPanel
{
private:
	const Font_t* m_pFont = nullptr;
	EAlign m_eAlign = ALIGN_TOP;
	Color_t m_tFill = {};
	Color_t m_tTitle = {};
	std::string m_sTitle;
	const char* m_sIcon = nullptr;
	struct Row_t { std::string m_sText; Color_t m_tColor; int m_nWidth; };
	std::vector<Row_t> m_vRows = {};

public:
	IndicatorPanel() = default;

	void Reset(const Font_t& fFont, const char* sTitle = nullptr, EAlign eAlign = ALIGN_TOP, const char* sIcon = nullptr)
	{
		m_pFont = &fFont;
		m_eAlign = eAlign;
		m_sTitle = sTitle ? sTitle : "";
		m_sIcon = sIcon;
		m_tFill = MemeSenseGfx::PageBg();
		m_tFill.a = 235;
		m_tTitle = MemeSenseGfx::Accent();
		m_vRows.clear();
		m_vRows.reserve(8);
	}

	void Row(const char* sText, const Color_t& tColor)
	{
		m_vRows.push_back({ sText, tColor, int(H::Draw.GetTextSize(sText, *m_pFont).x) });
	}

	bool Empty() const { return m_vRows.empty(); }
	int RowCount() const { return int(m_vRows.size()); }

	int PaddingX() const { return H::Draw.Scale(8, Scale_Round); }
	int PaddingY() const { return H::Draw.Scale(5, Scale_Round); }
	int RowHeight() const { return m_pFont->m_nTall + H::Draw.Scale(1, Scale_Round); }

	int LineCount() const
	{
		if (m_vRows.empty() && !m_sIcon)
			return 0;
		return m_vRows.empty() ? 1 : int(m_vRows.size());
	}

	int TextWidth() const
	{
		if (m_sIcon)
		{
			const int nIc = m_pFont->m_nTall;
			const int nGap = H::Draw.Scale(6, Scale_Round);
			int nMax = nIc;
			for (const auto& row : m_vRows)
				nMax = std::max(nMax, nIc + nGap + row.m_nWidth);
			return nMax;
		}
		int nMax = 0;
		for (const auto& row : m_vRows)
			nMax = std::max(nMax, row.m_nWidth);
		return nMax;
	}

	int Width() const { return TextWidth() + PaddingX() * 2; }
	int Height() const
	{
		const int nLines = LineCount();
		if (!nLines)
			return 0;
		return PaddingY() * 2 + RowHeight() * nLines - (m_vRows.empty() ? 0 : H::Draw.Scale(1, Scale_Round));
	}

	// pick a panel alignment so the chip stays fully on screen around the anchor (x, y)
	EAlign AutoAlign(int x) const
	{
		const int nW = Width();
		if (x <= nW / 2)
			return ALIGN_TOPLEFT;
		if (x >= int(H::Draw.m_nScreenW) - nW / 2)
			return ALIGN_TOPRIGHT;
		return ALIGN_TOP;
	}

	int OriginX(int x, EAlign a) const
	{
		const int nW = Width();
		switch (a)
		{
		case ALIGN_TOPLEFT: return x;
		case ALIGN_TOPRIGHT: return x - nW;
		default: return x - nW / 2;
		}
	}

	int OriginX(int x) const { return OriginX(x, AutoAlign(x)); }

	void Draw(int x, int y, EAlign align)
	{
		const int nW = Width();
		const int nH = Height();
		const int nX = OriginX(x, align);

		// eased reveal / staggered rows (Vars::Menu::Overlay). each panel fades and
		// rises in once on appearance, then rows follow with a small delay.
		const uint32_t uID = FNV1A::Hash32(m_sTitle.c_str());
		float flAlpha = 1.f, flSlide = 1.f;
		if (OverlayFx::Anim() && OverlayFx::Style() != OverlayFx::Style_Minimal)
			OverlayFx::Reveal(uID, 0.f, flAlpha, flSlide);
		if (flAlpha < 0.02f)
			return;

		const int nRise = int(H::Draw.Scale(6, Scale_Round));
		const int nY = y + int((1.f - flSlide) * nRise);

		// shared aero glass chrome: glow / gradient body / border / rail / ember
		const Color_t tAccent = MemeSenseGfx::Accent();
		OverlayFx::PanelSurface(nX, nY, nW, nH, tAccent, flAlpha);

		// fade helper for per-row staggering
		const auto RowFade = [&](int i) -> float
		{
			if (!OverlayFx::Anim() || OverlayFx::Style() == OverlayFx::Style_Minimal)
				return 1.f;
			float a = 1.f, s = 1.f;
			OverlayFx::Reveal(uID, 0.04f + 0.05f * i, a, s);
			return a;
		};

		const int nTextX = nX + PaddingX();
		int nCurY = nY + PaddingY();
		if (m_sIcon)
		{
			const int nIc = m_pFont->m_nTall;
			const float flIconA = RowFade(0);
			if (flIconA < 0.02f)
				return;
			SurfaceIcons::Draw(m_sIcon, nTextX, nCurY, nIc, m_tTitle.Alpha((unsigned char)(m_tTitle.a * flIconA)));
			if (!m_vRows.empty())
			{
				// first info line sits inline next to the icon (like the DT chip)
				const int nItemX = nTextX + nIc + H::Draw.Scale(6, Scale_Round);
				const float flFade = RowFade(1);
				H::Draw.StringOutlined(*m_pFont, nItemX, nCurY, m_vRows[0].m_tColor.Alpha((unsigned char)(m_vRows[0].m_tColor.a * flFade)), { 0, 0, 0, (unsigned char)(255.f * flFade) }, ALIGN_TOPLEFT, m_vRows[0].m_sText.c_str());
				nCurY += RowHeight();
				for (size_t i = 1; i < m_vRows.size(); ++i)
				{
					const float flA = RowFade(int(i + 1));
					H::Draw.StringOutlined(*m_pFont, nItemX, nCurY, m_vRows[i].m_tColor.Alpha((unsigned char)(m_vRows[i].m_tColor.a * flA)), { 0, 0, 0, (unsigned char)(255.f * flA) }, ALIGN_TOPLEFT, m_vRows[i].m_sText.c_str());
					nCurY += RowHeight();
				}
			}
			return;
		}
		for (size_t i = 0; i < m_vRows.size(); ++i)
		{
			const float flA = RowFade(int(i));
			H::Draw.StringOutlined(*m_pFont, nTextX, nCurY, m_vRows[i].m_tColor.Alpha((unsigned char)(m_vRows[i].m_tColor.a * flA)), { 0, 0, 0, (unsigned char)(255.f * flA) }, ALIGN_TOPLEFT, m_vRows[i].m_sText.c_str());
			nCurY += RowHeight();
		}
	}

	void Draw(int x, int y) { Draw(x, y, AutoAlign(x)); }
};