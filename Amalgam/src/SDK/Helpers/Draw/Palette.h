#pragma once
#include "../../Definitions/Types.h"
#include "../../Vars.h"

// Shared GUI palette. Every surface - the ImGui menu, the classic ISurface overlays
// (watermark, indicators, chips) - is colored from this single source, which is
// derived entirely from Vars::Menu::Theme::{Accent, Background, Active, Inactive}.
namespace TUI
{
	struct Palette_t
	{
		Color_t Accent = Color_t(255, 255, 255, 255);
		Color_t Background0 = Color_t(26, 26, 26, 255);
		Color_t Background0p5 = {};
		Color_t Background1 = {};
		Color_t Background1p5 = {};
		Color_t Background2 = {};
		Color_t Active = Color_t(255, 255, 255, 255);
		Color_t Inactive = Color_t(105, 105, 105, 255);
		Color_t Text = Color_t(255, 255, 255, 255);
		Color_t TextDisabled = Color_t(130, 130, 146, 255);
		Color_t GroupBoxBg = {};
		Color_t FrameInactive = {};
		Color_t FrameActive = {};
		Color_t Button = {};
		Color_t ButtonHovered = {};
		Color_t ButtonActive = {};
		Color_t Border = Color_t(255, 255, 255, 10);
	};

	inline Palette_t Palette = {};

	// recompute every frame from the theme configuration
	inline void Update()
	{
		const Color_t tBackground = Vars::Menu::Theme::Background.Value;
		const Color_t tAccent = Vars::Menu::Theme::Accent.Value;
		const Color_t tActive = Vars::Menu::Theme::Active.Value;
		const Color_t tInactive = Vars::Menu::Theme::Inactive.Value;

		Palette.Accent = tAccent;
		Palette.Active = tActive;
		Palette.Inactive = tInactive;
		Palette.Text = tActive;
		Palette.TextDisabled = tActive.Lerp(tBackground, 0.45f, LerpEnum::NoAlpha);

		Palette.Background0 = tBackground;
		Palette.Background0p5 = tBackground.Lerp({ 127, 127, 127 }, 0.5f / 9, LerpEnum::NoAlpha);
		Palette.Background1 = tBackground.Lerp({ 127, 127, 127 }, 1.f / 9, LerpEnum::NoAlpha);
		Palette.Background1p5 = tBackground.Lerp({ 127, 127, 127 }, 1.5f / 9, LerpEnum::NoAlpha);
		Palette.Background2 = tBackground.Lerp({ 127, 127, 127 }, 2.f / 9, LerpEnum::NoAlpha);

		Palette.GroupBoxBg = Palette.Background0;
		Palette.FrameInactive = Palette.Background0p5;
		Palette.FrameActive = Palette.Background1;
		Palette.Button = Palette.Background0p5;
		Palette.ButtonHovered = Palette.Background1;
		Palette.ButtonActive = Palette.Background1p5;
		Palette.Border = Color_t(tActive.r, tActive.g, tActive.b, 10);
	}
}