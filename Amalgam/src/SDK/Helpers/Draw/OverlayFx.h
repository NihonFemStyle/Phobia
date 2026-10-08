#pragma once
#include "Draw.h"
#include "MemeSenseGfx.h"
#include "../../SDK.h"
#include <cmath>
#include <cstdint>
#include <unordered_map>

// Flat Phobia chrome for the HUD overlay panels (indicator chips, the DT panel
// and the ImGui binds window): a solid dark-grey card with a 1px border, rounded
// corners. No glass body, no glow, rail or ember accents, and no per-row reveal
// animation (all off by default). An optional eased reveal can still be re-enabled
// via Vars::Menu::Overlay; it just fades the whole panel, never slides rows.
namespace OverlayFx
{
	enum OverlayStyle_t { Style_Glass = 0, Style_Solid = 1, Style_Minimal = 2 };

	// module-load clock, valid on both the vgui paint thread and the ImGui frame
	inline float Clock()
	{
		return float(SDK::PlatFloatTime());
	}

	inline float Clamp01(float t)
	{
		return t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
	}

	inline float EaseOutCubic(float t)
	{
		t = Clamp01(t);
		return 1.f - std::pow(1.f - t, 3.f);
	}

	inline float EaseOutQuint(float t)
	{
		t = Clamp01(t);
		return 1.f - std::pow(1.f - t, 5.f);
	}

	inline int Style() { return Vars::Menu::Overlay::Style.Value; }
	inline bool Anim() { return Vars::Menu::Overlay::Animated.Value; }

	// eased reveal for composite uID. on first sight (or after 2.5s unseen) the
	// overlay is hidden and fades in; flAlphaOut [0..1] and flSlideOut [0..1].
	inline void Reveal(uint32_t uID, float flDelay, float& flAlphaOut, float& flSlideOut)
	{
		static std::unordered_map<uint32_t, float> m_flBorn = {};
		auto& flBorn = m_flBorn[uID];
		const float flNow = Clock();
		if (flBorn <= 0.f || fabsf(flNow - flBorn) > 2.5f)
			flBorn = flNow;
		const float flT = Clamp01((flNow - flBorn - flDelay) / 0.32f);
		flAlphaOut = EaseOutCubic(flT);
		flSlideOut = EaseOutQuint(flT);
	}

	// corner radius (0 when rounded corners are disabled)
	inline int Radius()
	{
		return Vars::Menu::Overlay::Rounded.Value ? int(H::Draw.Scale(8, Scale_Round)) : 0;
	}

	// one-stop shop used by every ISurface overlay panel: solid Phobia card + 1px border
	inline void PanelSurface(int x, int y, int w, int h, const Color_t& tAccent, float flAlpha)
	{
		const int nRadius = Radius() > 0 ? int(H::Draw.Scale(5, Scale_Round)) : 0;
		const unsigned char nA = (unsigned char)(255.f * Clamp01(flAlpha));
		if (nRadius > 0)
		{
			H::Draw.FillRoundRect(x, y, w, h, nRadius, { 41, 41, 41, nA });
			if (Vars::Menu::Overlay::Border.Value)
				H::Draw.LineRoundRect(x, y, w, h, nRadius, { 50, 50, 50, nA });
		}
		else
		{
			H::Draw.FillRect(x, y, w, h, { 41, 41, 41, nA });
			if (Vars::Menu::Overlay::Border.Value)
				H::Draw.LineRect(x, y, w, h, { 50, 50, 50, nA });
		}
	}
}