#include "Aimbot.h"

#include "AimbotHitscan/AimbotHitscan.h"
#include "AimbotProjectile/AimbotProjectile.h"
#include "AimbotMelee/AimbotMelee.h"
#include "AutoDetonate/AutoDetonate.h"
#include "AutoAirblast/AutoAirblast.h"
#include "AutoHeal/AutoHeal.h"
#include "AutoRocketJump/AutoRocketJump.h"
#include "SmartFlick/SmartFlick.h"
#include "../Misc/Misc.h"
#include "../Visuals/Visuals.h"

bool CAimbot::ShouldRun(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (!pWeapon || !pLocal->CanAttack()
		|| !SDK::AttribHookValue(1, "mult_dmg", pWeapon)
		|| I::EngineVGui->IsGameUIVisible()
		|| pCmd->weaponselect)
		return false;

	return true;
}

void CAimbot::RunAimbot(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, bool bSecondaryType)
{
	m_bRunningSecondary = bSecondaryType;
	EWeaponType eWeaponType = !m_bRunningSecondary ? G::PrimaryWeaponType : G::SecondaryWeaponType;

	bool bOriginal;
	if (m_bRunningSecondary)
		bOriginal = G::CanPrimaryAttack, G::CanPrimaryAttack = G::CanSecondaryAttack;

	switch (eWeaponType)
	{
	case EWeaponType::HITSCAN: F::AimbotHitscan.Run(pLocal, pWeapon, pCmd); break;
	case EWeaponType::PROJECTILE: F::AimbotProjectile.Run(pLocal, pWeapon, pCmd); break;
	case EWeaponType::MELEE: F::AimbotMelee.Run(pLocal, pWeapon, pCmd); break;
	}

	if (m_bRunningSecondary)
		G::CanPrimaryAttack = bOriginal;
}

void CAimbot::RunMain(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (F::AimbotProjectile.m_iLastTickCancel)
	{
		pCmd->weaponselect = F::AimbotProjectile.m_iLastTickCancel;
		F::AimbotProjectile.m_iLastTickCancel = 0;
	}

	m_bRan = false;
	if (abs(G::AimTarget.m_iTickCount - I::GlobalVars->tickcount) > G::AimTarget.m_iDuration)
		G::AimTarget = {};
	if (abs(G::AimPoint.m_iTickCount - I::GlobalVars->tickcount) > G::AimPoint.m_iDuration)
		G::AimPoint = {};

	F::AutoRocketJump.Run(pLocal, pWeapon, pCmd);
	if (!ShouldRun(pLocal, pWeapon, pCmd))
		return;

	F::AutoDetonate.Run(pLocal, pCmd);
	F::AutoAirblast.Run(pLocal, pWeapon, pCmd);
	F::AutoHeal.Run(pLocal, pWeapon, pCmd);

	RunAimbot(pLocal, pWeapon, pCmd);
	RunAimbot(pLocal, pWeapon, pCmd, true);
}

void CAimbot::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	Store(false);

	RunMain(pLocal, pWeapon, pCmd);

	G::Attacking = SDK::IsAttacking(pLocal, pWeapon, pCmd, true);

	if (Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::SoftAim
		&& G::AimTarget.m_iEntIndex
		&& G::AimPoint.m_iTickCount)
	{
		if (m_iSoftAimEnt != G::AimTarget.m_iEntIndex)
		{
			m_iSoftAimEnt = G::AimTarget.m_iEntIndex;
			m_bSoftAimEngaged = false;
		}

		auto pTarget = I::ClientEntityList->GetClientEntity(G::AimTarget.m_iEntIndex);
		if (pTarget && !pTarget->IsDormant() && pLocal->IsAlive()
			&& I::GlobalVars->tickcount - G::AimTarget.m_iTickCount <= G::AimTarget.m_iDuration)
		{
			constexpr float flEngageRadius = 10.f;
			constexpr float flDragRadius = 10.f;
			const Vec3 vTargetAngle = Math::CalcAngle(pLocal->GetShootPos(), G::AimPoint.m_vOrigin);
			const Vec3 vDelta = pCmd->viewangles.DeltaAngle(vTargetAngle);
			const float flLen = vDelta.Length2D();
			if (m_bSoftAimEngaged || flLen <= flEngageRadius)
			{
				m_bSoftAimEngaged = true;
				if (flLen > flDragRadius)
				{
					pCmd->viewangles = vTargetAngle + vDelta / flLen * flDragRadius;
					Math::ClampAngles(pCmd->viewangles);
				}
			}
		}
		else if (m_bSoftAimEngaged)
		{
			m_bSoftAimEngaged = false;
			m_iSoftAimEnt = 0;
		}
	}
}

void CAimbot::Draw(CTFPlayer* pLocal)
{
	if (Vars::Aimbot::General::SmartFlick.Value
		&& (Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Plain
			|| Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Silent
			|| Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Smooth)
		&& Vars::Visuals::Prediction::SmartFlickCrosshair.Value
		&& Vars::Colors::SmartFlickCrosshair.Value.a
		&& pLocal->IsAlive()
		&& G::AimPoint.m_iTickCount)
	{
		Vec3 vScreen;
		if (SDK::W2S(G::AimPoint.m_vOrigin, vScreen))
		{
			const auto& tColor = Vars::Colors::SmartFlickCrosshair.Value;
			const int x = int(vScreen.x), y = int(vScreen.y);
			const int nSize = H::Draw.Scale(6), nLen = H::Draw.Scale(16);
			H::Draw.Line(x - nLen, y, x - nSize, y, tColor);
			H::Draw.Line(x + nSize, y, x + nLen, y, tColor);
			H::Draw.Line(x, y - nLen, x, y - nSize, tColor);
			H::Draw.Line(x, y + nSize, x, y + nLen, tColor);
		}
	}

	if (Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::SoftAim
		&& Vars::Visuals::Prediction::SoftAimCrosshair.Value
		&& Vars::Colors::SoftAimCrosshair.Value.a
		&& pLocal->IsAlive()
		&& G::AimPoint.m_iTickCount)
	{
		Vec3 vScreen;
		if (SDK::W2S(G::AimPoint.m_vOrigin, vScreen))
		{
			const auto& tColor = Vars::Colors::SoftAimCrosshair.Value;
			const int x = int(vScreen.x), y = int(vScreen.y);
			const int nSize = H::Draw.Scale(6), nLen = H::Draw.Scale(16);
			H::Draw.Line(x - nLen, y, x - nSize, y, tColor);
			H::Draw.Line(x + nSize, y, x + nLen, y, tColor);
			H::Draw.Line(x, y - nLen, x, y - nSize, tColor);
			H::Draw.Line(x, y + nSize, x, y + nLen, tColor);
		}
	}

	if (!Vars::Aimbot::General::FOVCircle.Value || !Vars::Colors::FOVCircle.Value.a || !pLocal->CanAttack(false))
		return;

	auto pWeapon = H::Entities.GetWeapon();
	if (pWeapon && !SDK::AttribHookValue(1, "mult_dmg", pWeapon))
		return;

	if (Vars::Aimbot::General::AimFOV.Value >= 90.f)
		return;

	float flRadius = tanf(Math::Deg2Rad(Vars::Aimbot::General::AimFOV.Value)) / tanf(Math::Deg2Rad(G::FOV) / 2) * float(H::Draw.m_nScreenW) * (4.f / 6.f) / (16.f / 9.f);
	H::Draw.LineCircle(H::Draw.m_nScreenW / 2, H::Draw.m_nScreenH / 2, flRadius, 68, Vars::Colors::FOVCircle.Value);
}

void CAimbot::Store(CBaseEntity* pEntity, size_t iSize)
{
	if (!Vars::Visuals::Prediction::RealPath.Value)
		return;

	if (!pEntity->IsPlayer())
		return;

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return;

	int iUserID = pResource->m_iUserID(pEntity->entindex());
	float flDuration = Vars::Visuals::Prediction::PlayerDrawDuration.Value ? Vars::Visuals::Prediction::PlayerDrawDuration.Value : 5.f;
	m_mRealPaths[iUserID] = {
		{ { pEntity->m_vecOrigin() }, I::GlobalVars->curtime + flDuration, Color_t(), Vars::Visuals::Prediction::RealPath.Value },
		iSize
	};
}

void CAimbot::Store(bool bFrameStageNotify)
{
	if (!Vars::Visuals::Prediction::RealPath.Value)
		return;

	int iLag = 1;
	if (bFrameStageNotify)
	{
		static int iStaticTickcout = I::GlobalVars->tickcount;
		iLag = I::GlobalVars->tickcount - iStaticTickcout;
		iStaticTickcout = I::GlobalVars->tickcount;
	}

	for (auto& [iUserID, tPath] : m_mRealPaths)
	{
		if (tPath.m_tPath.m_vPath.size() >= tPath.m_iSize || tPath.m_tPath.m_flTime < I::GlobalVars->curtime)
		{
			if (tPath.m_tPath.m_tColor = Vars::Colors::RealPath.Value, tPath.m_tPath.m_bZBuffer = true; tPath.m_tPath.m_tColor.a)
				G::PathStorage.push_back(tPath.m_tPath);
			if (tPath.m_tPath.m_tColor = Vars::Colors::RealPathIgnoreZ.Value, tPath.m_tPath.m_bZBuffer = false; tPath.m_tPath.m_tColor.a)
				G::PathStorage.push_back(tPath.m_tPath);
			m_mRealPaths.erase(iUserID);
			continue;
		}

		int iIndex = I::EngineClient->GetPlayerForUserID(iUserID);
		if (bFrameStageNotify ? iIndex == I::EngineClient->GetLocalPlayer() : iIndex != I::EngineClient->GetLocalPlayer())
			continue;

		auto pPlayer = I::ClientEntityList->GetClientEntity(iIndex)->As<CTFPlayer>();
		if (!pPlayer)
			continue;

		for (int i = 0; i < iLag; i++)
			tPath.m_tPath.m_vPath.push_back(pPlayer->m_vecOrigin());
	}
}