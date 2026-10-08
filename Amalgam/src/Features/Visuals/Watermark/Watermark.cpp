#include "Watermark.h"

#include "../../Players/PlayerUtils.h"
#include "../../../SDK/Helpers/Draw/MemeSenseGfx.h"
#include "../../../SDK/Helpers/Draw/PhobiaLogo.h"

#include <algorithm>

static const char* WatermarkBuildDate()
{
	static const std::string sResult = []()
	{
		std::string sDate = __DATE__;
		std::string sTime = __TIME__;
		return sDate + " " + sTime;
	}();
	return sResult.c_str();
}

void CWatermark::Draw(CTFPlayer* pLocal)
{
	if (!Vars::Menu::Watermark::Enabled.Value)
		return;

	// smooth the FPS reading over a short window
	if (auto pGlobals = I::GlobalVars; pGlobals && pGlobals->absoluteframetime > 0.f)
	{
		m_flFPSAccumulator += pGlobals->absoluteframetime;
		m_nFPSFrames++;
		if (m_nFPSFrames >= 30)
		{
			float flAvgFrameTime = m_flFPSAccumulator / m_nFPSFrames;
			m_flFPS = flAvgFrameTime > 0.f ? 1.f / flAvgFrameTime : 0.f;
			m_flFPSAccumulator = 0.f;
			m_nFPSFrames = 0;
		}
	}

	if (!(Vars::Menu::Watermark::Parts.Value))
		return;

	int iLocal = I::EngineClient->GetLocalPlayer();

	// Player name
	std::string sPlayerName = "";
	if (Vars::Menu::Watermark::Parts.Value & Vars::Menu::Watermark::PartsEnum::PlayerName)
	{
		player_info_t tInfo = {};
		if (I::EngineClient->GetPlayerInfo(iLocal, &tInfo) && tInfo.name[0])
			sPlayerName = tInfo.name;
		else if (pLocal)
			sPlayerName = F::PlayerUtils.GetPlayerName(iLocal);
	}

	// Ping
	int iPing = 0;
	if (Vars::Menu::Watermark::Parts.Value & Vars::Menu::Watermark::PartsEnum::Ping)
	{
		if (auto pNetChan = I::EngineClient->GetNetChannelInfo(); pNetChan && I::EngineClient->IsConnected())
			iPing = int(pNetChan->GetLatency(FLOW_INCOMING) * 1000.f);
	}

	// Assemble the string
	std::string sOutput;
	bool bFirst = true;
	auto fAppend = [&](const std::string& sPart)
	{
		if (sPart.empty())
			return;
		if (!bFirst)
			sOutput += " - ";
		sOutput += sPart;
		bFirst = false;
	};

	unsigned int uParts = Vars::Menu::Watermark::Parts.Value;
	if (uParts & Vars::Menu::Watermark::PartsEnum::CheatTitle)
		fAppend(Vars::Menu::CheatTitle.Value);
	if (uParts & Vars::Menu::Watermark::PartsEnum::PlayerName)
		fAppend(sPlayerName);
	if (uParts & Vars::Menu::Watermark::PartsEnum::FPS)
		fAppend(std::format("{} FPS", int(m_flFPS)));
	if (uParts & Vars::Menu::Watermark::PartsEnum::Ping)
		fAppend(std::format("{} ms", iPing));
	if (uParts & Vars::Menu::Watermark::PartsEnum::BuildDate)
		fAppend(WatermarkBuildDate());

	if (sOutput.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_WATERMARK);
	Vec2 vTextSize = H::Draw.GetTextSize(sOutput.c_str(), fFont);

	const bool bStyleRight = Vars::Menu::Watermark::Style.Value == Vars::Menu::Watermark::StyleEnum::TopRight || Vars::Menu::Watermark::Style.Value == Vars::Menu::Watermark::StyleEnum::BottomRight;
	const bool bStyleBottom = Vars::Menu::Watermark::Style.Value == Vars::Menu::Watermark::StyleEnum::BottomLeft || Vars::Menu::Watermark::Style.Value == Vars::Menu::Watermark::StyleEnum::BottomRight;
	const int nScreenW = H::Draw.m_nScreenW, nScreenH = H::Draw.m_nScreenH;

	// uniform Phobia card, flush against the corner: flat grey strip with the rose mark beside the text
	const int iMarkH = H::Draw.Scale(32, Scale_Round);
	const int iMarkW = int(iMarkH * 28.f / 33.f);
	const int iBoxH = H::Draw.Scale(40, Scale_Round);
	const int iTextW = int(vTextSize.x);
	const int iTextOff = H::Draw.Scale(14, Scale_Round);
	const int iMarkPad = H::Draw.Scale(4, Scale_Round);
	const int iFullW = iMarkPad + iMarkW + iTextW + iTextOff;
	const int X = bStyleRight ? nScreenW - iFullW : 0;
	const int Y = bStyleBottom ? nScreenH - iBoxH : 0;
	const int iMarkX = bStyleRight ? X + iFullW - iMarkW - iMarkPad : X + iMarkPad;
	const int iTextY = Y + iBoxH / 2;
	const int iRadius = H::Draw.Scale(5, Scale_Round);

	// solid Phobia card (41,41,41) + 1px border (50,50,50)
	H::Draw.FillRoundRect(X, Y, iFullW, iBoxH, iRadius, { 41, 41, 41, 255 });
	if (Vars::Menu::Overlay::Border.Value)
		H::Draw.LineRoundRect(X, Y, iFullW, iBoxH, iRadius, { 50, 50, 50, 255 });

	// rose Phobia mark, directly on the card, spanning its height with even breathing room
	Phobia::Logo(iMarkX, Y + (iBoxH - iMarkH) / 2, iMarkW, iMarkH, MemeSenseGfx::White());

	const Color_t tText = MemeSenseGfx::White();
	if (bStyleRight)
		H::Draw.StringOutlined(fFont, iMarkX - iTextOff, iTextY, tText, { 0, 0, 0, 255 }, ALIGN_RIGHT, sOutput.c_str());
	else
		H::Draw.StringOutlined(fFont, X + iMarkPad + iMarkW + iTextOff, iTextY, tText, { 0, 0, 0, 255 }, ALIGN_LEFT, sOutput.c_str());
}
