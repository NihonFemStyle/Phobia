#pragma once
#include "../../SDK/SDK.h"

struct AngleHistory_t
{
	Vec3 m_vAngle;
	bool m_bAttacking;
};

struct PlayerInfo
{
	uint32_t m_uAccountID = 0;
	const char* m_sName = "";

	int m_iDetections = 0;

	struct PacketChoking_t
	{
		std::deque<int> m_vChokes = {}; // store last 3 choke counts
		bool m_bInfract = false; // infract the user for choking?
	} m_PacketChoking;

	struct AimFlicking_t
	{
		std::deque<AngleHistory_t> m_vAngles = {}; // store last 3 angles & if damage was dealt
	} m_AimFlicking;
					
	struct DuckSpeed_t
	{
		int m_iStartTick = 0;
	} m_DuckSpeed;

	struct Tickbase_t
	{
		int m_iLastSimulationTicks = 0;
		float m_flTickbaseInfractionWait = 0.f; // avoid flooding
		bool m_bIsAbusingTickbase = false;
	} m_Tickbase;

	struct Speedhack_t
	{
		Vec3 m_vLastOrigin = {};
		int m_iLastSimulationTicks = 0;
		int m_iViolations = 0;
		float m_flInfractionWait = 0.f; // 0.25s between flags
	} m_Speedhack;

	struct Ping_t
	{
		int m_iFrameCount = 0; // consecutive scans sustained
		bool m_bHard = false; // last offense was low ping
		bool m_bMarked = false; // already marked for this offense
	} m_Ping;
};

class CCheatDetection
{
private:
	bool ShouldScan();

	bool InvalidPitch(CTFPlayer* pEntity);
	bool IsChoking(CTFPlayer* pEntity);
	bool IsFlicking(CTFPlayer* pEntity);
	bool IsDuckSpeed(CTFPlayer* pEntity);
	bool IsAbusingTickbase(CTFPlayer* pEntity);
	bool IsSpeedhacking(CTFPlayer* pEntity);
	bool IsPingSpoofing(CTFPlayer* pEntity, int iPing, bool bIsSuspect, bool& bHard);

	void Infract(CTFPlayer* pEntity, const char* sReason, bool bHard = false);

	std::unordered_map<CTFPlayer*, PlayerInfo> mData = {};

public:
	void Run();

	void ReportChoke(CTFPlayer* pEntity, int iChoke);
	void ReportDamage(IGameEvent* pEvent);
	void Reset();
};

ADD_FEATURE(CCheatDetection, CheatDetection);