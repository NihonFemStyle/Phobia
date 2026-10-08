#include "Ticks.h"

#include "../../SDK/Helpers/Draw/IndicatorPanel.h"
#include "../../SDK/Helpers/Draw/OverlayFx.h"
#include "../../SDK/Helpers/Draw/MemeSenseGfx.h"
#include "../ImGui/Menu/FA6Icons.h"
#include "../ImGui/Menu/Menu.h"
#include "../PacketManip/AntiAim/AntiAim.h"
#include "../EnginePrediction/EnginePrediction.h"
#include "../Aimbot/AutoRocketJump/AutoRocketJump.h"
#include "../Backtrack/Backtrack.h"
#include "../AntiCheatCompatibility/AntiCheatCompatibility.h"

void CTicks::Reset()
{
	m_bSpeedhack = m_bDoubletap = m_bRecharge = m_bWarp = false;
	m_iShiftedTicks = m_iShiftedGoal = 0;
}

void CTicks::Recharge(CTFPlayer* pLocal)
{
	if (!m_bGoalReached)
		return;

	bool bPassive = m_bRecharge = false;

	static float flPassiveTime = 0.f;
	flPassiveTime = std::max(flPassiveTime - TICK_INTERVAL, -TICK_INTERVAL);
	if (Vars::Doubletap::PassiveRecharge.Value && 0.f >= flPassiveTime)
	{
		bPassive = true;
		flPassiveTime += 1.f / Vars::Doubletap::PassiveRecharge.Value;
	}

	if (m_iDeficit)
	{
		bPassive = true;
		m_iDeficit--, m_iShiftedTicks--;
	}

	if (!Vars::Doubletap::RechargeTicks.Value && !bPassive
		|| m_bDoubletap || m_bWarp || m_iShiftedTicks == m_iMaxShift || m_bSpeedhack)
		return;

	m_bRecharge = true;
	m_iShiftedGoal = m_iShiftedTicks + 1;
}

void CTicks::Warp()
{
	if (!m_bGoalReached)
		return;

	m_bWarp = false;
	if (!Vars::Doubletap::Warp.Value
		|| !m_iShiftedTicks || m_bDoubletap || m_bRecharge || m_bSpeedhack)
		return;

	m_bWarp = true;
	m_iShiftedGoal = std::max(m_iShiftedTicks - Vars::Doubletap::WarpRate.Value + 1, 0);
}

void CTicks::Doubletap(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!m_bGoalReached)
		return;

	if (!Vars::Doubletap::Doubletap.Value
		|| m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack)
		return;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	auto pWeapon = H::Entities.GetWeapon();
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		return;

	bool bAttacking = G::PrimaryWeaponType == EWeaponType::MELEE ? pCmd->buttons & IN_ATTACK : G::Attacking;
	if (!G::CanPrimaryAttack && !G::Reloading || !bAttacking && !m_bDoubletap || F::AutoRocketJump.IsRunning())
		return;

	m_bDoubletap = true;
	m_iShiftedGoal = std::max(m_iShiftedTicks - Vars::Doubletap::TickLimit.Value + 1, 0);
	if (Vars::Doubletap::AntiWarp.Value)
		m_bAntiWarp = pLocal->m_hGroundEntity();
}

void CTicks::Speedhack()
{
	m_bSpeedhack = Vars::Speedhack::Scale.Value != 1;
	if (!m_bSpeedhack)
		return;

	m_bDoubletap = m_bWarp = m_bRecharge = false;
}

static Vec3 s_vVelocity = {};
static int s_iMaxTicks = 0;
void CTicks::AntiWarp(CTFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove, int iTicks)
{
	s_iMaxTicks = std::max(iTicks + 1, s_iMaxTicks);

	Vec3 vAngles; Math::VectorAngles(s_vVelocity, vAngles);
	vAngles.y = flYaw - vAngles.y;
	Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
	vForward *= s_vVelocity.Length2D();

	if (iTicks > std::max(s_iMaxTicks - 8, 3))
		flForwardMove = -vForward.x, flSideMove = -vForward.y;
	else if (iTicks > 3)
		flForwardMove = flSideMove = 0.f;
	else
		flForwardMove = vForward.x, flSideMove = vForward.y;
}
void CTicks::AntiWarp(CTFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove)
{
	AntiWarp(pLocal, flYaw, flForwardMove, flSideMove, GetTicks());
}
void CTicks::AntiWarp(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bAntiWarp)
		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	else
	{
		s_vVelocity = pLocal->m_vecVelocity();
		s_iMaxTicks = 0;
	}
}

bool CTicks::ValidWeapon(CTFWeaponBase* pWeapon)
{
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_PDA_SPY_BUILD:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_GRAPPLINGHOOK:
	case TF_WEAPON_JAR_MILK:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_BUFF_ITEM:
	case TF_WEAPON_ROCKETPACK:
	case TF_WEAPON_JAR_GAS:
	case TF_WEAPON_LASER_POINTER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_JAR:
		return false;
	}

	return true;
}

void CTicks::MoveFunc(float accumulated_extra_samples, bool bFinalTick)
{
	m_iShiftedTicks--;
	if (m_iWait > 0)
		m_iWait--;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	auto pWeapon = H::Entities.GetWeapon();
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		m_iWait = -1;

	m_bGoalReached = bFinalTick && m_iShiftedTicks == m_iShiftedGoal;

	static auto CL_Move = U::Hooks.m_mHooks["CL_Move"];
	CL_Move->Call<void>(accumulated_extra_samples, bFinalTick);
}

void CTicks::Move(float accumulated_extra_samples, bool bFinalTick)
{
	MoveManage();

	while (m_iShiftedTicks > m_iMaxShift)
		MoveFunc(accumulated_extra_samples, false);
	m_iShiftedTicks = std::max(m_iShiftedTicks, 0) + 1;

	if (m_bSpeedhack)
	{
		m_iShiftedTicks = Vars::Speedhack::Scale.Value;
		m_iShiftedGoal = 0;
	}

	m_iShiftedGoal = std::clamp(m_iShiftedGoal, 0, m_iMaxShift);
	if (m_iShiftedTicks > m_iShiftedGoal) // normal use/doubletap/teleport
	{
		m_iShiftStart = m_iShiftedTicks - 1;
		m_bShifted = false;

		while (m_iShiftedTicks > m_iShiftedGoal)
		{
			m_bShifting = m_bShifted |= m_iShiftedTicks - 1 != m_iShiftedGoal;
			MoveFunc(accumulated_extra_samples, m_iShiftedTicks - 1 == m_iShiftedGoal);
		}

		m_bShifting = m_bAntiWarp = m_bTimingUnsure = false;
		if (m_bWarp)
			m_iDeficit = 0;

		m_bDoubletap = m_bWarp = false;
	}
	else // else recharge, run once if we have any choked ticks
	{
		if (I::ClientState->chokedcommands)
			MoveFunc(accumulated_extra_samples, bFinalTick);
	}
}

void CTicks::MoveManage()
{
	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return;

	Recharge(pLocal);
	Warp();
	Speedhack();

	if (!m_bRecharge)
		m_iWait = std::max(m_iWait, 0);
	if (auto pWeapon = H::Entities.GetWeapon())
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_CANNON:
			if (!G::CanSecondaryAttack)
				m_iWait = Vars::Doubletap::TickLimit.Value;
			break;
		default:
			if (!ValidWeapon(pWeapon))
				m_iWait = -1;
			else if (G::Attacking || !G::CanPrimaryAttack && !G::Reloading)
				m_iWait = Vars::Doubletap::TickLimit.Value;
		}
	}
	else
		m_iWait = -1;

	static auto sv_maxusrcmdprocessticks = H::ConVars.FindVar("sv_maxusrcmdprocessticks");
	m_iMaxUsrCmdProcessTicks = sv_maxusrcmdprocessticks->GetInt();
	if (F::AntiCheatCompatibility.Active())
		m_iMaxUsrCmdProcessTicks = std::min(m_iMaxUsrCmdProcessTicks, 8);
	m_iMaxShift = m_iMaxUsrCmdProcessTicks - std::max(m_iMaxUsrCmdProcessTicks - Vars::Doubletap::RechargeLimit.Value, 0) - (F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0);
	m_iMaxShift = std::max(m_iMaxShift, 1);
}

void CTicks::CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, bool* pSendPacket)
{
	Doubletap(pLocal, pCmd);
	AntiWarp(pLocal, pCmd);
	ManagePacket(pCmd, pSendPacket);

	SaveShootPos(pLocal);
	SaveShootAngle(pCmd, *pSendPacket);

	if (m_bDoubletap && m_iShiftedTicks == m_iShiftStart && pWeapon && pWeapon->IsInReload())
		m_bTimingUnsure = true;
}

void CTicks::ManagePacket(CUserCmd* pCmd, bool* pSendPacket)
{
	if (!m_bDoubletap && !m_bWarp && !m_bSpeedhack)
	{
		static bool bWasSet = false;
		bool bCanChoke = CanChoke(true); // failsafe
		if (G::PSilentAngles && bCanChoke)
			*pSendPacket = false, bWasSet = true;
		else if (bWasSet || !bCanChoke)
			*pSendPacket = true, bWasSet = false;

		bool bShouldShift = m_iShiftedTicks && m_iShiftedTicks + I::ClientState->chokedcommands >= m_iMaxUsrCmdProcessTicks;
		if (!*pSendPacket && bShouldShift)
			m_iShiftedGoal = std::max(m_iShiftedGoal - 1, 0);
	}
	else
	{
		if ((m_bSpeedhack || m_bWarp) && G::Attacking == 1)
		{
			*pSendPacket = true;
			return;
		}

		*pSendPacket = m_iShiftedGoal == m_iShiftedTicks;
		if (I::ClientState->chokedcommands >= 21) // prevent overchoking
			*pSendPacket = true;
	}
}

void CTicks::Start(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	Vec2 vOriginalMove; int iOriginalButtons;
	if (m_bPredictAntiwarp = m_bAntiWarp || GetTicks(H::Entities.GetWeapon()) && Vars::Doubletap::AntiWarp.Value && pLocal->m_hGroundEntity())
	{
		vOriginalMove = { pCmd->forwardmove, pCmd->sidemove };
		iOriginalButtons = pCmd->buttons;

		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	}

	F::EnginePrediction.Start(pLocal, pCmd);

	if (m_bPredictAntiwarp)
	{
		pCmd->forwardmove = vOriginalMove.x, pCmd->sidemove = vOriginalMove.y;
		pCmd->buttons = iOriginalButtons;
	}
}

void CTicks::End(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bPredictAntiwarp && !m_bAntiWarp && !G::Attacking)
	{
		F::EnginePrediction.End(pLocal, pCmd);
		F::EnginePrediction.Start(pLocal, pCmd);
	}
}

bool CTicks::CanChoke(bool bCanShift, int iMaxTicks)
{
	bool bCanChoke = I::ClientState->chokedcommands < 21;
	if (bCanChoke && !bCanShift)
		bCanChoke = m_iShiftedTicks + I::ClientState->chokedcommands < iMaxTicks;
	return bCanChoke;
}
bool CTicks::CanChoke(bool bCanShift)
{
	return CanChoke(bCanShift, m_iMaxUsrCmdProcessTicks);
}

int CTicks::GetTicks(CTFWeaponBase* pWeapon)
{
	if (m_bDoubletap && m_iShiftedGoal < m_iShiftedTicks)
		return m_iShiftedTicks - m_iShiftedGoal;

	if (!Vars::Doubletap::Doubletap.Value
		|| m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack || F::AutoRocketJump.IsRunning())
		return 0;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		return 0;
	
	return std::min(Vars::Doubletap::TickLimit.Value - 1, m_iMaxShift);
}

int CTicks::GetShotsWithinPacket(CTFWeaponBase* pWeapon, int iTicks)
{
	iTicks = std::min(m_iMaxShift + 1, iTicks);

	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return 1 + (iTicks - iDelay) / std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL);
}

int CTicks::GetMinimumTicksNeeded(CTFWeaponBase* pWeapon)
{
	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return (GetShotsWithinPacket(pWeapon) - 1) * std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL) + iDelay;
}

void CTicks::SaveShootPos(CTFPlayer* pLocal)
{
	if (m_iShiftedTicks == m_iShiftStart)
		m_vShootPos = pLocal->GetShootPos();
}
Vec3 CTicks::GetShootPos()
{
	return m_vShootPos;
}

void CTicks::SaveShootAngle(CUserCmd* pCmd, bool bSendPacket)
{
	static auto sv_maxusrcmdprocessticks_holdaim = H::ConVars.FindVar("sv_maxusrcmdprocessticks_holdaim");

	if (bSendPacket)
		m_vShootAngle = std::nullopt;
	else if (!m_vShootAngle && G::Attacking == 1 && sv_maxusrcmdprocessticks_holdaim->GetBool())
		m_vShootAngle = pCmd->viewangles;
}
Vec3* CTicks::GetShootAngle()
{
	if (m_vShootAngle && I::ClientState->chokedcommands)
		return &m_vShootAngle.value();
	return nullptr;
}

bool CTicks::IsTimingUnsure()
{	// actually knowing when we'll shoot would be better than this, but this is fine for now
	return m_bTimingUnsure || m_bSpeedhack /*|| m_bWarp*/;
}

void CTicks::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Ticks) || !pLocal->IsAlive())
		return;

	const DragBox_t dtPos = Vars::Menu::TicksDisplay.Value;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);

	int x = dtPos.x, y = dtPos.y + 2;

	if (m_bSpeedhack)
	{
		IndicatorPanel p;
		p.Reset(fFont, "Ticks", ALIGN_TOP, MS_ICON_FA_ANGLES_RIGHT);
		p.Row(std::format("Speedhack x{}", Vars::Speedhack::Scale.Value).c_str(), MemeSenseGfx::White());
		p.Draw(x, y);
		F::Menu.DragOverlay(Vars::Menu::TicksDisplay, p.OriginX(x), y, p.Width(), p.Height());
		return;
	}

	const Color_t tAccent = MemeSenseGfx::Accent();
	const Color_t tWhite = MemeSenseGfx::White();
	const Color_t tBlack = { 0, 0, 0, 255 };

	const int iMax = std::max(m_iMaxShift, 1);
	const int iCharge = std::clamp(m_iShiftedTicks, 0, m_iMaxShift);
	const float flCharge = float(iCharge) / float(iMax);

	static float flSmoothed = flCharge;
	const float flSpeed = std::clamp(I::GlobalVars->frametime * 8.f, I::GlobalVars->interval_per_tick, 1.0f);
	flSmoothed = std::clamp(flSmoothed + (flCharge - flSmoothed) * flSpeed, 0.f, 1.f);
	if (fabsf(flSmoothed - flCharge) < 0.001f)
		flSmoothed = flCharge;

	Color_t tStatus = MemeSenseGfx::White();
	std::string sStatus;
	const bool bHardBlocked = !Vars::Doubletap::Doubletap.Value || m_iWait || F::AutoRocketJump.IsRunning();
	if (m_bRecharge)
		sStatus = "charging", tStatus = Vars::Colors::IndicatorTextMid.Value;
	else if (iCharge >= std::min(Vars::Doubletap::TickLimit.Value, iMax))
		sStatus = "ready", tStatus = Vars::Colors::IndicatorTextGood.Value;
	else if (bHardBlocked)
		sStatus = "not possible", tStatus = Vars::Colors::IndicatorTextBad.Value;
	else
		sStatus = "not ready", tStatus = MemeSenseGfx::ComboText();

	// size the panel to its contents so the text never escapes the box
	const std::string sCharge = std::format("{} / {} ({:.2f}s)", iCharge, iMax, iCharge * TICK_INTERVAL);
	const int iIconSize = fFont.m_nTall;
	const int iTextW = std::max(int(H::Draw.GetTextSize(sCharge.c_str(), fFont).x), int(H::Draw.GetTextSize(sStatus.c_str(), fFont).x));
	const int iPadding = H::Draw.Scale(5, Scale_Round);
	const int iWidth = iTextW + iIconSize + H::Draw.Scale(6, Scale_Round) + iPadding * 2;
	const int iPosX = dtPos.x - iWidth / 2;
	int iPosY = dtPos.y;

	// eased reveal / rise for the whole panel (Vars::Menu::Overlay)
	const uint32_t uPanelID = FNV1A::Hash32("Ticks DT");
	float flAlpha = 1.f, flSlide = 1.f;
	if (OverlayFx::Anim())
		OverlayFx::Reveal(uPanelID, 0.f, flAlpha, flSlide);
	if (flAlpha < 0.02f)
		return;
	iPosY += int((1.f - flSlide) * H::Draw.Scale(6, Scale_Round));

	// layout: line0 = icon + charge text, line1 = status (own row), bar below it
	const int iRowH = fFont.m_nTall + H::Draw.Scale(1, Scale_Round);
	const int iTextTop = H::Draw.Scale(5, Scale_Round);
	const int iLine0Y = iPosY + iTextTop;
	const int iLine1Y = iLine0Y + iRowH;
	const int iBarH = H::Draw.Scale(9, Scale_Round);
	const int iBarY = iLine1Y + iRowH + H::Draw.Scale(2, Scale_Round);
	const int iHeight = (iBarY + iBarH) - iPosY + H::Draw.Scale(4, Scale_Round);

	// aero glass chrome: glow / gradient body / accent border / rail / ember baseline
	OverlayFx::PanelSurface(iPosX, iPosY, iWidth, iHeight, tAccent, flAlpha);

	// charge bar: black when empty, glow color ramps to accent at full charge
	// (reverse glow: bright at the bar's outer edges, dim toward the core)
	const int bar_x = iPosX + iPadding;
	const int bar_y = iBarY;
	const int bar_total = iWidth - iPadding * 2;
	const int bar_h = iBarH;

	H::Draw.FillRect(bar_x, bar_y, bar_total, bar_h, tBlack.Alpha((unsigned char)(220.f * flAlpha)));
	const int bar_w = std::max(0, int(bar_total * flSmoothed));
	if (bar_w > 0)
	{
		const Color_t tFill = tBlack.Lerp(tAccent, flSmoothed, LerpEnum::All).Alpha((unsigned char)(255.f * flAlpha));
		const int hHalf = std::max(1, bar_h / 2);
		H::Draw.GradientRect(bar_x, bar_y, bar_w, hHalf, tFill, tFill.Alpha((unsigned char)(64.f * flAlpha)), false);
		H::Draw.GradientRect(bar_x, bar_y + bar_h - hHalf, bar_w, hHalf, tFill.Alpha((unsigned char)(64.f * flAlpha)), tFill, false);
		const int hMid = bar_h - hHalf * 2;
		if (hMid > 0)
			H::Draw.FillRect(bar_x, bar_y + hHalf, bar_w, hMid, tFill.Alpha((unsigned char)(48.f * flAlpha)));
		H::Draw.LineRect(bar_x, bar_y, bar_total, bar_h, tFill);
	}

	// double-chevron DT icon + charge text (line 0)
	const int iTextX = iPosX + iPadding + iIconSize + H::Draw.Scale(6, Scale_Round);
	SurfaceIcons::Draw(MS_ICON_FA_ANGLES_RIGHT, iPosX + iPadding, iLine0Y, iIconSize, tAccent.Alpha((unsigned char)(255.f * flAlpha)));
	H::Draw.StringOutlined(fFont, iTextX, iLine0Y, tWhite.Alpha((unsigned char)(255.f * flAlpha)), tBlack.Alpha((unsigned char)(255.f * flAlpha)), ALIGN_TOPLEFT, sCharge.c_str());

	// status on its own line above the bar, right-aligned like jvnkbin
	H::Draw.StringOutlined(fFont, iPosX + iWidth - iPadding, iLine1Y, tStatus.Alpha((unsigned char)(tStatus.a * flAlpha)), tBlack.Alpha((unsigned char)(255.f * flAlpha)), ALIGN_TOPRIGHT, sStatus.c_str());

	F::Menu.DragOverlay(Vars::Menu::TicksDisplay, iPosX, iPosY, iWidth, iHeight);
}