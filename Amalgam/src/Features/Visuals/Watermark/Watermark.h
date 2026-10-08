#pragma once
#include "../../../SDK/SDK.h"

class CWatermark
{
private:
	float m_flFPS = 0.f;
	float m_flFPSAccumulator = 0.f;
	int m_nFPSFrames = 0;

public:
	void Draw(CTFPlayer* pLocal);
};

ADD_FEATURE(CWatermark, Watermark);
