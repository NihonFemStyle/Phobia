#include "Render.h"

#include "../../Hooks/Direct3DDevice9.h"
#include "../../SDK/Helpers/Draw/SurfaceIcons.h"
#include <ImGui/imgui_impl_win32.h>
#include "Fonts/MaterialDesign/MaterialIcons.h"
#include "Fonts/MaterialDesign/IconDefinitions.h"
#include "Fonts/CascadiaMono/CascadiaMono.h"
#include "Fonts/Roboto/RobotoMedium.h"
#include "Fonts/Roboto/RobotoBlack.h"
#include "Fonts/Phobia/PhobiaAssets.h"
#include "../../SDK/Helpers/Fonts/Comfortaa/ComfortaaRegular.h"
#include "../../SDK/Helpers/Fonts/Comfortaa/ComfortaaLight.h"
#include "../../SDK/Helpers/Fonts/Comfortaa/ComfortaaBold.h"
#include "Menu/Menu.h"
#include "Menu/Components.h"
#include "Menu/neverlose/blur.hpp"
#include "Menu/neverlose/bytes.hpp"
#include "Menu/neverlose/hashes.hpp"
#include "MemeSense/ext/fonts/fonts.h"
#include "MemeSense/ext/fonts/iconsfontawesome/fa.h"

#include <vector>
#include <cstring>

void CRender::Render(IDirect3DDevice9* pDevice)
{
	static std::once_flag tFlag; std::call_once(tFlag, [&]
	{
		Initialize(pDevice);
	});

	LoadColors();
	{
		static float flStaticScale = Vars::Menu::Scale.Value;
		float flOldScale = flStaticScale;
		float flNewScale = flStaticScale = Vars::Menu::Scale.Value;
		if (flNewScale != flOldScale)
			Reload();
	}

	blur::device = pDevice;

	DWORD dwOldRGB; pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &dwOldRGB);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	F::Menu.Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, dwOldRGB);
}

void CRender::LoadColors()
{
	using namespace ImGui;

	TUI::Update();

	// loader-indigo slate palette (chrome backgrounds locked; Accent follows the user theme via TUI::Palette)
	Accent = MemeSense::Accent();
	Background0 = MemeSense::PageBg();
	Background0p5 = MemeSense::WidgetBg();
	Background1 = MemeSense::SidebarBg();
	Background1p5 = MemeSense::ActiveBg();
	Background1p5L = { Background1p5.Value.x * 1.1f, Background1p5.Value.y * 1.1f, Background1p5.Value.z * 1.1f, Background1p5.Value.w };
	Background2 = { 58.f / 255.f, 58.f / 255.f, 58.f / 255.f, 1.f }; // light grey (hover)
	Inactive = MemeSense::TextDim();
	Active = MemeSense::White();

	ImVec4* colors = GetStyle().Colors;
	colors[ImGuiCol_Border] = MemeSense::Line();
	colors[ImGuiCol_Button] = {};
	colors[ImGuiCol_ButtonHovered] = {};
	colors[ImGuiCol_ButtonActive] = {};
	colors[ImGuiCol_FrameBg] = Background1p5;
	colors[ImGuiCol_FrameBgHovered] = Background1p5L;
	colors[ImGuiCol_FrameBgActive] = Background1p5;
	colors[ImGuiCol_Header] = {};
	colors[ImGuiCol_HeaderHovered] = { Background1p5L.Value.x * 1.1f, Background1p5L.Value.y * 1.1f, Background1p5L.Value.z * 1.1f, Background1p5.Value.w }; // divd by 1.1
	colors[ImGuiCol_HeaderActive] = Background1p5;
	colors[ImGuiCol_ModalWindowDimBg] = { Background0.Value.x, Background0.Value.y, Background0.Value.z, 0.4f };
	colors[ImGuiCol_PopupBg] = MemeSense::WidgetBg();
	colors[ImGuiCol_ResizeGrip] = {};
	colors[ImGuiCol_ResizeGripActive] = {};
	colors[ImGuiCol_ResizeGripHovered] = {};
	colors[ImGuiCol_ScrollbarBg] = {};
	colors[ImGuiCol_Text] = Active;
	colors[ImGuiCol_WindowBg] = {};
}

// rasterize the FA glyphs the overlay chips need out of the ImGui icon font atlas
// into ISurface textures, so the in-game chips can show the same icons as the menu
static void BuildSurfaceIcons(ImFont* pFont)
{
	using namespace ImGui;

	if (!pFont || !pFont->OwnerAtlas)
		return;

	SurfaceIcons::s_mTextures.clear();

	unsigned char* pPixels = nullptr;
	int nW = 0, nH = 0;
	GetIO().Fonts->GetTexDataAsRGBA32(&pPixels, &nW, &nH, nullptr);
	if (!pPixels || nW <= 0 || nH <= 0)
		return;

	ImFontBaked* pBaked = pFont->GetFontBaked(pFont->LegacySize);
	if (!pBaked)
		return;

	const char* aIcons[] = { MS_ICON_FA_BOLT, MS_ICON_FA_BULLSEYE, MS_ICON_FA_EYE, MS_ICON_FA_SIGNAL, MS_ICON_FA_HEART_PULSE, MS_ICON_FA_DICE_SIX, MS_ICON_FA_GEAR, MS_ICON_FA_ANGLES_RIGHT, MS_ICON_FA_KEYBOARD, MS_ICON_FA_GAMEPAD, MS_ICON_FA_SEEDLING, MS_ICON_FA_HEART, MS_ICON_FA_WIFI };
	for (const char* sIcon : aIcons)
	{
		const uint32_t uCodepoint = SurfaceIcons::Utf8Codepoint(sIcon);
		const ImFontGlyph* pGlyph = pBaked->FindGlyphNoFallback(ImWchar(uCodepoint));
		if (!pGlyph)
			continue;

		const int gx0 = int(pGlyph->U0 * nW), gy0 = int(pGlyph->V0 * nH);
		const int gx1 = int(pGlyph->U1 * nW), gy1 = int(pGlyph->V1 * nH);
		const int gw = gx1 - gx0, gh = gy1 - gy0;
		if (gw <= 0 || gh <= 0)
			continue;

		std::vector<unsigned char> vPixels(size_t(gw) * gh * 4);
		for (int row = 0; row < gh; row++)
			memcpy(&vPixels[row * gw * 4], &pPixels[(gy0 + row) * nW * 4 + gx0 * 4], gw * 4);

		SurfaceIcons::s_mTextures[uCodepoint] = H::Draw.CreateTextureFromArray(vPixels.data(), gw, gh);
	}
}

void CRender::LoadFonts()
{
	using namespace ImGui;

	auto& io = GetIO();

	if (static bool bLoaded = false; !bLoaded)
		bLoaded = true;
	else
		io.Fonts->Clear();

	ImFontConfig tFontConfig;
	tFontConfig.OversampleH = 2;

	// Comfortaa: the rounded UI face (regular for text, light for large labels, bold for titles)
	FontMono = io.Fonts->AddFontFromMemoryCompressedTTF(CascadiaMono_compressed_data, CascadiaMono_compressed_size, H::Draw.Scale(15), &tFontConfig);
	FontSmall = io.Fonts->AddFontFromMemoryTTF(ComfortaaRegular, sizeof ComfortaaRegular, H::Draw.Scale(12), &tFontConfig);
	FontRegular = io.Fonts->AddFontFromMemoryTTF(ComfortaaRegular, sizeof ComfortaaRegular, H::Draw.Scale(13), &tFontConfig);

	// merge font awesome into the regular font so icons render inline with text
	ImFontConfig tMergedConfig;
	tMergedConfig.MergeMode = true;
	tMergedConfig.PixelSnapH = true;
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, H::Draw.Scale(13), &tMergedConfig, icon_ranges);

	// merge font awesome 6 into the regular font as well (MemeSense chrome icons, e.g. the gun/glyphs that FA5 lacks)
	static const ImWchar ms_icon_ranges[] = { 0xE180, 0xE39F, 0xF000, 0xF9A0, 0 };
	io.Fonts->AddFontFromMemoryTTF(freesolid900, sizeof freesolid900, H::Draw.Scale(13), &tMergedConfig, ms_icon_ranges);

	FontBold = io.Fonts->AddFontFromMemoryTTF(ComfortaaBold, sizeof ComfortaaBold, H::Draw.Scale(13), &tFontConfig);
	// merge the icon fonts into bold/large too so icons render in section titles, buttons, etc.
	io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, H::Draw.Scale(13), &tMergedConfig, icon_ranges);
	io.Fonts->AddFontFromMemoryTTF(freesolid900, sizeof freesolid900, H::Draw.Scale(13), &tMergedConfig, ms_icon_ranges);
	FontLarge = io.Fonts->AddFontFromMemoryTTF(ComfortaaLight, sizeof ComfortaaLight, H::Draw.Scale(15), &tFontConfig);
	io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, H::Draw.Scale(15), &tMergedConfig, icon_ranges);
	io.Fonts->AddFontFromMemoryTTF(freesolid900, sizeof freesolid900, H::Draw.Scale(15), &tMergedConfig, ms_icon_ranges);
	FontTitle = io.Fonts->AddFontFromMemoryTTF(ComfortaaBold, sizeof ComfortaaBold, H::Draw.Scale(28), &tFontConfig);
	// splash wordmark: bold 54px Comfortaa so the boot logo reads strong fullscreen
	FontSplash = io.Fonts->AddFontFromMemoryTTF(ComfortaaBold, sizeof ComfortaaBold, H::Draw.Scale(54), &tFontConfig);

	ImFontConfig tIconConfig;
	tIconConfig.PixelSnapH = true;
	IconFont = io.Fonts->AddFontFromMemoryCompressedTTF(MaterialIcons_compressed_data, MaterialIcons_compressed_size, H::Draw.Scale(16), &tIconConfig);

	// merge font awesome into the icon font so FA glyphs render (playerlist ban icons)
	ImFontConfig tIconMergedConfig;
	tIconMergedConfig.MergeMode = true;
	tIconMergedConfig.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, H::Draw.Scale(16), &tIconMergedConfig, icon_ranges);
	io.Fonts->AddFontFromMemoryTTF(freesolid900, sizeof freesolid900, H::Draw.Scale(16), &tIconMergedConfig, ms_icon_ranges);

	// Phobia icon glyphs for the sidebar tabs (0xE000 - 0xE226, kept in their own font
	// so they can't collide with the MaterialIcons glyphs the pages already use)
	PhobiaIcon = io.Fonts->AddFontFromMemoryTTF((void*)Phobia::IconFont, (int)Phobia::IconFontSize, H::Draw.Scale(18), &tIconConfig);
	io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, H::Draw.Scale(18), &tIconMergedConfig, icon_ranges);
	io.Fonts->AddFontFromMemoryTTF(freesolid900, sizeof freesolid900, H::Draw.Scale(18), &tIconMergedConfig, ms_icon_ranges);

	io.FontDefault = FontRegular;
	io.Fonts->Build();

	// rasterize the overlay chip icons (bolts, targets, ...) into ISurface textures
	BuildSurfaceIcons(IconFont);

	io.ConfigDebugHighlightIdConflicts = false;
}

void CRender::LoadStyle()
{
	using namespace ImGui;

	auto& style = GetStyle();
	style.ButtonTextAlign = { 0.5f, 0.5f };
	style.CellPadding = { H::Draw.Scale(4), 0 };
	style.ChildBorderSize = 0.f;
	style.ChildRounding = H::Draw.Scale(5);
	style.FrameBorderSize = 0.f;
	style.FramePadding = { 0, 0 };
	style.FrameRounding = H::Draw.Scale(6);
	style.ItemInnerSpacing = { 0, 0 };
	style.ItemSpacing = { H::Draw.Scale(8), H::Draw.Scale(8) };
	style.PopupBorderSize = 0.f;
	style.PopupRounding = H::Draw.Scale(8);
	style.ScrollbarSize = 0.f; // invisible scrollbars (wheel still scrolls; takes no width)
	style.ScrollbarRounding = 0.f;
	style.WindowBorderSize = 0.f;
	style.WindowPadding = { 0, 0 };
	style.WindowRounding = H::Draw.Scale(10);
}

void CRender::Initialize(IDirect3DDevice9* pDevice)
{
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(WndProc::hwWindow);
	ImGui_ImplDX9_Init(pDevice);

	auto& io = ImGui::GetIO();
	//io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	LoadFonts();
	LoadStyle();

	m_bLoaded = true;
}

void CRender::Reload()
{
	m_bLoaded = false;

	LoadFonts();
	LoadStyle();

	m_bLoaded = true;
}