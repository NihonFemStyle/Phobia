#include "SmartFlick.h"

#include <random>

bool CSmartFlick::Reset()
{
	m_iState = State::Idle;
	m_iMethod = -1;
	m_flStartTime = 0.f;
	m_flDuration = 0.f;
	m_flDistance = 0.f;
	m_vOrigin = m_vAim = m_vPast = m_vFlickPoint = {};
	return false;
}

bool CSmartFlick::Run(CUserCmd* pCmd, Vec3& vAngles, int iMethod)
{
	const bool bPlain = iMethod == Vars::Aimbot::General::AimTypeEnum::Plain;
	const bool bSilent = iMethod == Vars::Aimbot::General::AimTypeEnum::Silent;
	const bool bSmooth = iMethod == Vars::Aimbot::General::AimTypeEnum::Smooth;
	if (!Vars::Aimbot::General::SmartFlick.Value || !bPlain && !bSilent && !bSmooth)
		return Reset();

	switch (m_iState)
	{
	case State::Idle:
	{
		if (G::Attacking == 1 && !m_bPrevAttacking)
		{
			m_iState = State::Sequence;
			m_iMethod = iMethod;
			m_bPrevAttacking = true;
			m_flStartTime = I::GlobalVars->curtime;

			static std::mt19937 rng(std::random_device{}());
			static std::uniform_real_distribution<float> dist(0.f, 1.f);

			const float flMinT = Vars::Aimbot::General::TimeMin.Value;
			const float flMaxT = std::max(flMinT, Vars::Aimbot::General::TimeMax.Value);
			const float flMinD = Vars::Aimbot::General::DistanceMin.Value;
			const float flMaxD = std::max(flMinD, Vars::Aimbot::General::DistanceMax.Value);
			m_flDuration = flMinT + dist(rng) * (flMaxT - flMinT);
			m_flDistance = flMinD + dist(rng) * (flMaxD - flMinD);
			m_vOrigin = I::EngineClient->GetViewAngles();
			m_vAim = vAngles;
			m_vPast = {};
			m_vFlickPoint = G::AimPoint.m_vOrigin;
		}
		else
			m_bPrevAttacking = G::Attacking == 1;
		return false;
	}
	case State::Sequence:
	{
		if (iMethod != m_iMethod)
			return Reset();

		const float flPhase = std::clamp((I::GlobalVars->curtime - m_flStartTime) / std::max(0.001f, m_flDuration / 1000.f), 0.f, 1.f);

		const float flAimEnd = bSilent ? 0.5f : 0.7f;
		const float flPastEnd = bSilent ? 0.6f : 1.f;

		if (flPhase < flAimEnd)
		{
			m_vAim = vAngles;
			m_bPrevAttacking = G::Attacking == 1;
			return false; // aimbot as intended, fire happens naturally
		}

		if (!m_vPast)
		{
			const Vec3 vDelta = m_vAim.DeltaAngle(m_vOrigin);
			const Vec3 vDir = vDelta.Length() > 0.01f ? vDelta.Normalized() : Vec3(0.f, 1.f, 0.f);
			m_vPast = m_vAim + vDir * (m_flDistance * (Vars::Aimbot::General::Overflick.Value ? 1.f : 0.25f));
			Math::ClampAngles(m_vPast);
		}

		Vec3 vCur;
		if (flPhase < flPastEnd)
			vCur = m_vAim.LerpAngle(m_vPast, EaseIn(std::clamp((flPhase - flAimEnd) / (flPastEnd - flAimEnd), 0.f, 1.f)));
		else if (bSilent)
			vCur = m_vPast.LerpAngle(m_vOrigin, EaseOut(std::clamp((flPhase - flPastEnd) / (1.f - flPastEnd), 0.f, 1.f)));
		else
			vCur = m_vPast;
		Math::ClampAngles(vCur);

		if (bSilent)
		{
			SDK::FixMovement(pCmd, vCur);
			pCmd->viewangles = vCur;
			G::PSilentAngles = true;
		}
		else
		{
			pCmd->viewangles = vCur;
			I::EngineClient->SetViewAngles(vCur);
		}
		vAngles = vCur;

		if (m_vFlickPoint)
			G::AimPoint = { m_vFlickPoint, I::GlobalVars->tickcount };

		if (flPhase >= 1.f)
		{
			m_bPrevAttacking = G::Attacking == 1;
			return Reset();
		}
		return true;
	}
	}

	return false;
}