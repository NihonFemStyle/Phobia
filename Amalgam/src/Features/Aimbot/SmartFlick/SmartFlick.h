#pragma once
#include "../../../SDK/SDK.h"

class CSmartFlick
{
private:
	enum State
	{
		Idle,
		Sequence
	};

	State m_iState = State::Idle;
	bool m_bPrevAttacking = false;
	int m_iMethod = -1;
	float m_flStartTime = 0.f;
	float m_flDuration = 0.f;
	float m_flDistance = 0.f;
	Vec3 m_vOrigin = {};
	Vec3 m_vAim = {};
	Vec3 m_vPast = {};
	Vec3 m_vFlickPoint = {};

	static inline float EaseOut(float t)
	{
		return 1.f - (1.f - t) * (1.f - t);
	}

	static inline float EaseIn(float t)
	{
		return t * t;
	}

	bool Reset();

public:
	bool Run(CUserCmd* pCmd, Vec3& vAngles, int iMethod);
};

ADD_FEATURE(CSmartFlick, SmartFlick);