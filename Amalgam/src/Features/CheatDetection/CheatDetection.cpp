#include "CheatDetection.h"

#include "../Players/PlayerUtils.h"
#include "../Output/Output.h"

bool CCheatDetection::ShouldScan()
{
	if (!Vars::CheatDetection::Methods.Value /*|| I::EngineClient->IsPlayingDemo()*/)
		return false;

	static int iStaticTickcount = I::GlobalVars->tickcount;
	const int iLastTickcount = iStaticTickcount;
	const int iCurrTickcount = iStaticTickcount = I::GlobalVars->tickcount;
	if (iCurrTickcount != iLastTickcount + 1)
		return false;

	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (pNetChan && (pNetChan->GetTimeSinceLastReceived() > TICK_INTERVAL * 2 || pNetChan->IsTimingOut()))
		return false;

	return true;
}

bool CCheatDetection::InvalidPitch(CTFPlayer* pEntity)
{
	return Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::InvalidPitch && fabsf(pEntity->m_angEyeAnglesX()) == 90.f;
}

bool CCheatDetection::IsChoking(CTFPlayer* pEntity)
{
	bool bReturn = mData[pEntity].m_PacketChoking.m_bInfract;
	mData[pEntity].m_PacketChoking.m_bInfract = false;

	return Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::PacketChoking && bReturn;
}

bool CCheatDetection::IsFlicking(CTFPlayer* pEntity) // awful
{
	auto& vAngles = mData[pEntity].m_AimFlicking.m_vAngles;
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::AimFlicking))
	{
		vAngles.clear();
		return false;
	}

	vAngles.emplace_front(pEntity->GetEyeAngles(), false);
	if (vAngles.size() > 3)
		vAngles.pop_back();

	if (vAngles.size() != 3 || !vAngles[0].m_bAttacking && !vAngles[1].m_bAttacking && !vAngles[2].m_bAttacking
		|| Math::CalcFov(vAngles[0].m_vAngle, vAngles[1].m_vAngle) < Vars::CheatDetection::MinFlick.Value
		|| Math::CalcFov(vAngles[0].m_vAngle, vAngles[2].m_vAngle) > Vars::CheatDetection::MaxNoise.Value * (TICK_INTERVAL / 0.015f))
		return false;

	vAngles.clear();
	return true;
}

bool CCheatDetection::IsDuckSpeed(CTFPlayer* pEntity)
{
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::DuckSpeed)
		|| !pEntity->IsDucking() || !pEntity->IsOnGround()
		|| pEntity->m_vecVelocity().Length2D() < pEntity->m_flMaxspeed() * 0.5f)
	{
		mData[pEntity].m_DuckSpeed.m_iStartTick = 0;
		return false;
	}

	if (!mData[pEntity].m_DuckSpeed.m_iStartTick)
		mData[pEntity].m_DuckSpeed.m_iStartTick = I::GlobalVars->tickcount;

	if (I::GlobalVars->tickcount - mData[pEntity].m_DuckSpeed.m_iStartTick > TIME_TO_TICKS(1))
	{
		mData[pEntity].m_DuckSpeed.m_iStartTick = 0;
		return true;
	}

	return false;
}

bool CCheatDetection::IsAbusingTickbase(CTFPlayer* pEntity)
{
	auto& tTickbase = mData[pEntity].m_Tickbase;
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::TickbaseAbuse))
	{
		tTickbase = {};
		return false;
	}

	int iSimulationTicks = TIME_TO_TICKS(pEntity->m_flSimulationTime());
	int iServerTicks = I::ClientState->m_ClockDriftMgr.m_nServerTick;

	// Simulation time going backwards usually means a tickbase exploit
	if (iSimulationTicks < tTickbase.m_iLastSimulationTicks)
	{
		tTickbase = {};
		return true;
	}

	// A player that isn't being simulated for more than 2 seconds (while not lagging out) is abusing their tickbase
	if (tTickbase.m_iLastSimulationTicks && abs(iSimulationTicks - iServerTicks) > TIME_TO_TICKS(2))
	{
		if (I::GlobalVars->curtime > tTickbase.m_flTickbaseInfractionWait)
		{
			tTickbase.m_flTickbaseInfractionWait = I::GlobalVars->curtime + 0.25f;
			return true;
		}
	}

	tTickbase.m_bIsAbusingTickbase = tTickbase.m_iLastSimulationTicks && abs(iSimulationTicks - iServerTicks) > TIME_TO_TICKS(2);
	tTickbase.m_iLastSimulationTicks = iSimulationTicks;
	return false;
}

bool CCheatDetection::IsSpeedhacking(CTFPlayer* pEntity)
{
	auto& tSpeedhack = mData[pEntity].m_Speedhack;
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::Speedhack))
	{
		tSpeedhack = {};
		return false;
	}

	int iSimulationTicks = TIME_TO_TICKS(pEntity->m_flSimulationTime());
	int iServerTicks = I::ClientState->m_ClockDriftMgr.m_nServerTick;
	int iTickDiff = abs(iSimulationTicks - iServerTicks);
	bool bLagging = iTickDiff > 3 && iTickDiff <= TIME_TO_TICKS(1);

	// only works on players moving at normal (220) speed, and penalize laggers by the tick diff
	if (tSpeedhack.m_iLastSimulationTicks && pEntity->m_MoveType() == MOVETYPE_WALK && fabsf(pEntity->m_flMaxspeed() - 220.f) < 1.f)
	{
		// the distance a player should be able to travel in a tick, larger allowances for lagging players
		float flCost = bLagging ? 8.f + iTickDiff : 8.f;

		if (std::max((tSpeedhack.m_vLastOrigin - pEntity->m_vecOrigin()).Length2D() - flCost, 0.f) >= 1.f)
		{
			if (I::GlobalVars->curtime > tSpeedhack.m_flInfractionWait)
			{
				tSpeedhack.m_flInfractionWait = I::GlobalVars->curtime + 0.25f;
				tSpeedhack.m_iViolations++;
				if (tSpeedhack.m_iViolations >= 6)
				{
					tSpeedhack.m_iViolations = 0;
					tSpeedhack.m_vLastOrigin = pEntity->m_vecOrigin();
					tSpeedhack.m_iLastSimulationTicks = iSimulationTicks;
					return true;
				}
			}
		}
		else
			tSpeedhack.m_iViolations = 0;
	}

	tSpeedhack.m_vLastOrigin = pEntity->m_vecOrigin();
	tSpeedhack.m_iLastSimulationTicks = iSimulationTicks;
	return false;
}

bool CCheatDetection::IsPingSpoofing(CTFPlayer* pEntity, int iPing, bool bIsSuspect, bool& bHard)
{
	auto& tPing = mData[pEntity].m_Ping;
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::PingSpoofing))
	{
		tPing = {};
		return false;
	}

	const bool bLow = iPing < Vars::CheatDetection::PingThresholdLow.Value; // impossible for legit
	const bool bHigh = iPing > Vars::CheatDetection::PingThresholdHigh.Value; // suspicious latency

	if (bLow != bHigh)
	{
		const bool bOffenseHard = bLow;
		if (!tPing.m_bMarked || tPing.m_bHard != bOffenseHard)
		{
			tPing.m_iFrameCount++;
			if (tPing.m_iFrameCount >= TIME_TO_TICKS(3))
			{
				tPing.m_iFrameCount = 0;
				tPing.m_bMarked = true;
				tPing.m_bHard = bOffenseHard;
				if (bOffenseHard || !bIsSuspect)
				{
					bHard = tPing.m_bHard;
					return true;
				}
			}
		}
	}
	else
	{
		tPing.m_iFrameCount = 0;
		tPing.m_bMarked = false;
	}

	return false;
}

void CCheatDetection::Infract(CTFPlayer* pEntity, const char* sReason, bool bHard)
{
	const int iCheaterTag = F::PlayerUtils.TagToIndex(CHEATER_TAG);
	const int iSuspectTag = F::PlayerUtils.TagToIndex(SUSPECTED_CHEATER_TAG);

	bool bMark = false;
	if (Vars::CheatDetection::DetectionsRequired.Value)
	{
		mData[pEntity].m_iDetections++;
		bMark = mData[pEntity].m_iDetections >= Vars::CheatDetection::DetectionsRequired.Value;
	}

	F::Output.CheatDetection(mData[pEntity].m_sName, bMark ? "marked" : "infracted", sReason);
	if (bMark)
	{
		mData[pEntity].m_iDetections = 0;
		if (bHard)
		{
			if (F::PlayerUtils.HasTag(mData[pEntity].m_uAccountID, iSuspectTag))
				F::PlayerUtils.RemoveTag(mData[pEntity].m_uAccountID, iSuspectTag, true, mData[pEntity].m_sName);
			if (!F::PlayerUtils.HasTag(mData[pEntity].m_uAccountID, iCheaterTag))
				F::PlayerUtils.AddTag(mData[pEntity].m_uAccountID, iCheaterTag, true, mData[pEntity].m_sName);
		}
		else if (!F::PlayerUtils.HasTag(mData[pEntity].m_uAccountID, iCheaterTag)
			&& !F::PlayerUtils.HasTag(mData[pEntity].m_uAccountID, iSuspectTag))
			F::PlayerUtils.AddTag(mData[pEntity].m_uAccountID, iSuspectTag, true, mData[pEntity].m_sName);
	}
}

void CCheatDetection::Run()
{
	if (!ShouldScan() || !I::EngineClient->IsConnected() || I::EngineClient->IsPlayingDemo())
		return;

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return;

	for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		int iIndex = pPlayer->entindex();
		if (!H::Entities.GetDeltaTime(iIndex))
			continue;

		if (iIndex == I::EngineClient->GetLocalPlayer() || !pPlayer->IsAlive() || pPlayer->IsAGhost()
			|| pResource->IsFakePlayer(iIndex) || F::PlayerUtils.HasTag(iIndex, F::PlayerUtils.TagToIndex(CHEATER_TAG)))
		{
			mData[pPlayer].m_PacketChoking = {};
			mData[pPlayer].m_AimFlicking = {};
			mData[pPlayer].m_DuckSpeed = {};
			mData[pPlayer].m_Tickbase = {};
			mData[pPlayer].m_Speedhack = {};
			mData[pPlayer].m_Ping = {};
			continue;
		}

		mData[pPlayer].m_uAccountID = pResource->m_iAccountID(iIndex);
		mData[pPlayer].m_sName = F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex));

		const bool bIsSuspect = F::PlayerUtils.HasTag(mData[pPlayer].m_uAccountID, F::PlayerUtils.TagToIndex(SUSPECTED_CHEATER_TAG));

		if (InvalidPitch(pPlayer))
			Infract(pPlayer, "invalid pitch", true);
		if (IsChoking(pPlayer) && !bIsSuspect)
			Infract(pPlayer, "choking packets");
		if (IsFlicking(pPlayer) && !bIsSuspect)
			Infract(pPlayer, "flicking");
		if (IsDuckSpeed(pPlayer) && !bIsSuspect)
			Infract(pPlayer, "duck speed");
		if (IsAbusingTickbase(pPlayer) && !bIsSuspect)
			Infract(pPlayer, "tickbase abuse");
		if (IsSpeedhacking(pPlayer) && !bIsSuspect)
			Infract(pPlayer, "speedhack");

		bool bPingHard = false;
		if (IsPingSpoofing(pPlayer, pResource->m_iPing(iIndex), bIsSuspect, bPingHard))
			Infract(pPlayer, bPingHard ? "impossible ping" : "suspicious latency", bPingHard);
	}
}

void CCheatDetection::Reset()
{
	mData.clear();
}

void CCheatDetection::ReportChoke(CTFPlayer* pEntity, int iChoke)
{
	if (Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::PacketChoking)
	{
		mData[pEntity].m_PacketChoking.m_vChokes.push_back(iChoke);
		if (mData[pEntity].m_PacketChoking.m_vChokes.size() == 3)
		{
			mData[pEntity].m_PacketChoking.m_bInfract = true; // check for last 3 choke amounts
			for (auto& iChoke : mData[pEntity].m_PacketChoking.m_vChokes)
			{
				if (iChoke < Vars::CheatDetection::MinChoking.Value)
					mData[pEntity].m_PacketChoking.m_bInfract = false;
			}
			mData[pEntity].m_PacketChoking.m_vChokes.clear();
		}
	}
	else
		mData[pEntity].m_PacketChoking.m_vChokes.clear();
}

void CCheatDetection::ReportDamage(IGameEvent* pEvent)
{
	if (!(Vars::CheatDetection::Methods.Value & Vars::CheatDetection::MethodsEnum::AimFlicking))
		return;

	int iIndex = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
	if (iIndex == I::EngineClient->GetLocalPlayer())
		return;

	auto pEntity = I::ClientEntityList->GetClientEntity(iIndex)->As<CTFPlayer>();
	if (!pEntity || !pEntity->IsPlayer() || pEntity->IsDormant())
		return;

	auto pWeapon = pEntity->m_hActiveWeapon()->As<CTFWeaponBase>();
	switch (SDK::GetWeaponType(pWeapon))
	{
	case EWeaponType::UNKNOWN:
	case EWeaponType::PROJECTILE:
		return;
	}

	auto& vAngles = mData[pEntity].m_AimFlicking.m_vAngles;
	if (!vAngles.empty())
		vAngles.back().m_bAttacking = true;
}