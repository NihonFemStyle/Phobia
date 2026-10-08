#include "Notifications.h"

#include "../Easings/Easings.h"
#include "../Menu/Components.h"
#include <ImGui/imgui_internal.h>

#include <algorithm>

void CNotifications::Add(const std::string& sText, const char* sIcon, Color_t tColor, float flLifeTime, float flPanTime)
{
	float flTime = SDK::PlatFloatTime();
	m_vNotifications.emplace_back(sText, sIcon, flTime, flLifeTime, flPanTime, tColor);
	if (m_vNotifications.size() > Vars::Logging::MaxNotifications.Value)
	{
		for (int i = 0; i < m_vNotifications.size() - Vars::Logging::MaxNotifications.Value; i++)
		{
			auto& tNotification = m_vNotifications[i];
			if (!tNotification.m_flLifeTime)
				continue;

			tNotification.m_flCreateTime = flTime - tNotification.m_flPanTime;
			tNotification.m_flLifeTime = 0.f;
		}
		//m_vNotifications.pop_front();
	}
}

void CNotifications::Add(const std::string& sText, Color_t tColor, float flLifeTime, float flPanTime)
{
	Add(sText, nullptr, tColor, flLifeTime, flPanTime);
}

static inline bool ShouldReverseX()
{
	switch (Vars::Logging::NotificationPosition.Value)
	{
	case Vars::Logging::NotificationPositionEnum::TopLeft:
	case Vars::Logging::NotificationPositionEnum::BottomLeft:
		return false;
	case Vars::Logging::NotificationPositionEnum::TopRight:
	case Vars::Logging::NotificationPositionEnum::BottomRight:
		return true;
	}
	return false;
}
static inline bool ShouldReverseY()
{
	switch (Vars::Logging::NotificationPosition.Value)
	{
	case Vars::Logging::NotificationPositionEnum::TopLeft:
	case Vars::Logging::NotificationPositionEnum::TopRight:
		return false;
	case Vars::Logging::NotificationPositionEnum::BottomLeft:
	case Vars::Logging::NotificationPositionEnum::BottomRight:
		return true;
	}
	return false;
}

void CNotifications::Draw()
{
	using namespace ImGui;

	const float flNow = SDK::PlatFloatTime();
	for (auto it = m_vNotifications.begin(); it != m_vNotifications.end();)
	{
		if (it->m_flCreateTime + it->m_flPanTime * 2 + it->m_flLifeTime < flNow)
			it = m_vNotifications.erase(it);
		else
			++it;
	}
	if (m_vNotifications.empty())
		return;

	ImDrawList* pDrawList = GetForegroundDrawList();
	const float flScreenW = GetIO().DisplaySize.x;
	const float flScreenH = GetIO().DisplaySize.y;

	const bool bRight = ShouldReverseX();
	const bool bBottom = ShouldReverseY();

	// watermark proportions (scaled identically)
	const float fiBoxW = H::Draw.Scale(41, Scale_Round);
	const float fiBoxH = H::Draw.Scale(39, Scale_Round);
	const float fiStripW = H::Draw.Scale(2, Scale_Round);
	const float fiTextPadX = H::Draw.Scale(20, Scale_Round);
	const float fiTextOff = H::Draw.Scale(9, Scale_Round);
	const float fiBarH = H::Draw.Scale(2, Scale_Round);

	const auto MakeCol = [](byte r, byte g, byte b, float flA) -> ImU32
	{
		return IM_COL32(r, g, b, int(std::clamp(flA, 0.f, 255.f)));
	};

	int i = 0;
	for (auto& tNotification : m_vNotifications)
	{
		const float flPan = tNotification.m_flPanTime;
		const float flLife = tNotification.m_flLifeTime;

		// enter: the fade stretches into the size of the notification
		float flEnter = flPan > 0.f ? std::clamp((flNow - tNotification.m_flCreateTime) / flPan, 0.f, 1.f) : 1.f;
		flEnter = Ease::OutCubic(flEnter);
		// exit: slide off the screen (up for top corners, down for bottom corners)
		float flExit = flPan > 0.f ? std::clamp((flNow - (tNotification.m_flCreateTime + flPan + flLife)) / flPan, 0.f, 1.f) : 0.f;
		flExit = Ease::InCubic(flExit);

		const char* sIcon = tNotification.m_sIcon ? tNotification.m_sIcon : ICON_MD_INFO;
		const float flTextW = CalcTextSize(tNotification.m_sText.c_str()).x;
		const float flW = fiBoxW + flTextW + fiTextPadX;
		const float flAreaW = (flTextW + fiTextPadX) * flEnter;	// stretched fade/text region
		const float flAlpha = std::min(1.f, flEnter * 1.6f);

		// flush against the corner, stacked against it with no gaps
		const float x = bRight ? flScreenW - flW : 0.f;
		const float fiBoxX = bRight ? flScreenW - fiBoxW : x;
		const float y = (bBottom ? flScreenH - fiBoxH - i * fiBoxH : i * fiBoxH) + (bBottom ? 1.f : -1.f) * flExit * fiBoxH;

		const Color_t tAccent = tNotification.m_tColor;

		// 1. black logo box (keep the notification's own icon, no watermark logo)
		pDrawList->AddRectFilled({ fiBoxX, y }, { fiBoxX + fiBoxW, y + fiBoxH }, MakeCol(0, 0, 0, 255.f * flAlpha));

		// 2. accent seam on the inner edge of the box (top -> bottom, like the watermark)
		{
			const float fiSeamX = bRight ? fiBoxX - fiStripW : fiBoxX + fiBoxW;
			pDrawList->AddRectFilledMultiColor({ fiSeamX, y }, { fiSeamX + fiStripW, y + fiBoxH },
				MakeCol(tAccent.r, tAccent.g, tAccent.b, 255.f * flAlpha),
				MakeCol(tAccent.r, tAccent.g, tAccent.b, 255.f * flAlpha),
				MakeCol(tAccent.r, tAccent.g, tAccent.b, 150.f * flAlpha),
				MakeCol(tAccent.r, tAccent.g, tAccent.b, 150.f * flAlpha));
		}

		// 3. fading black text backdrop, bright near the seam fading outwards (stretches on enter)
		{
			const float fiFadeX = bRight ? fiBoxX - flAreaW : fiBoxX + fiBoxW;
			const ImU32 uIn = MakeCol(0, 0, 0, 255.f * flAlpha);
			const ImU32 uOut = MakeCol(0, 0, 0, 8.f * flAlpha);
			const ImU32 uTL = bRight ? uOut : uIn, uTR = bRight ? uIn : uOut;
			pDrawList->AddRectFilledMultiColor({ fiFadeX, y }, { fiFadeX + flAreaW, y + fiBoxH }, uTL, uTR, uTR, uTL);
		}

		// 4. optional remaining-time bar along the bottom (fades like the main notification)
		if (Vars::Logging::NotificationProgress.Value && flLife > 0.f)
		{
			const float flRemain = std::clamp((tNotification.m_flCreateTime + flPan + flLife - flNow) / flLife, 0.f, 1.f);
			const ImU32 uTrackIn = MakeCol(0, 0, 0, 150.f * flAlpha);
			const ImU32 uTrackOut = MakeCol(0, 0, 0, 8.f * flAlpha);
			const ImU32 uTrackL = bRight ? uTrackOut : uTrackIn, uTrackR = bRight ? uTrackIn : uTrackOut;
			pDrawList->AddRectFilledMultiColor({ x, y + fiBoxH - fiBarH }, { x + flW, y + fiBoxH }, uTrackL, uTrackR, uTrackR, uTrackL);
			if (flRemain > 0.f)
			{
				const float fiFillW = flW * flRemain;
				const float fiFillX = bRight ? x + flW - fiFillW : x;
				const ImU32 uFillIn = MakeCol(tAccent.r, tAccent.g, tAccent.b, 255.f * flAlpha);
				const ImU32 uFillOut = MakeCol(tAccent.r, tAccent.g, tAccent.b, 8.f * flAlpha);
				const ImU32 uFillL = bRight ? uFillOut : uFillIn, uFillR = bRight ? uFillIn : uFillOut;
				pDrawList->PushClipRect({ fiFillX, y + fiBoxH - fiBarH }, { fiFillX + fiFillW, y + fiBoxH }, true);
				pDrawList->AddRectFilledMultiColor({ x, y + fiBoxH - fiBarH }, { x + flW, y + fiBoxH }, uFillL, uFillR, uFillR, uFillL);
				pDrawList->PopClipRect();
			}
		}

		// 5. icon centered in the box
		{
			const float fiIconSize = F::Render.IconFont->LegacySize;
			const ImVec2 vIconPos = { fiBoxX + (fiBoxW - fiIconSize) / 2.f, y + (fiBoxH - fiIconSize) / 2.f };
			pDrawList->AddText(F::Render.IconFont, fiIconSize, vIconPos, MakeCol(tAccent.r, tAccent.g, tAccent.b, 255.f * flAlpha), sIcon);
		}

		// 6. text attaches to the seam, vertically centered, clipped by the stretched fade region
		{
			const ImVec2 vTextSize = CalcTextSize(tNotification.m_sText.c_str());
			const float fiTextX = bRight ? fiBoxX - fiTextOff - flTextW : fiBoxX + fiBoxW + fiTextOff;
			const float fiTextY = y + (fiBoxH - vTextSize.y) / 2.f;
			const float fiClipL = bRight ? fiBoxX - flAreaW : fiBoxX + fiBoxW;
			const float fiClipR = bRight ? fiBoxX : fiBoxX + fiBoxW + flAreaW;
			pDrawList->PushClipRect({ fiClipL, y - 1.f }, { fiClipR, y + fiBoxH + 1.f }, true);
			pDrawList->AddText({ fiTextX, fiTextY }, MakeCol(255, 255, 255, 255.f * flAlpha), tNotification.m_sText.c_str());
			pDrawList->PopClipRect();
		}

		i++;
	}
}