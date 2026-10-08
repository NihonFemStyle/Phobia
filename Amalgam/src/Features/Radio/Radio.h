#pragma once
#include "bass.h"
#include "../../SDK/SDK.h"

enum ERadioStation : int
{
	RADIO_DNB,
	RADIO_JUNGLE,
	RADIO_LIQUID_DNB,
	RADIO_HARDCORE,
	RADIO_TECHNO,
	RADIO_CLASSIC_RAP,
	RADIO_VAPORWAVE,
	RADIO_MAX,
};

class RadioManager
{
public:
	void RunRadioLoop();
	bool StartupRadio();

private:
	int m_current_channel = RADIO_DNB;
	bool m_need_reinit = false;
	ConVar* m_pVolume = nullptr;
};

ADD_FEATURE(RadioManager, Radio);